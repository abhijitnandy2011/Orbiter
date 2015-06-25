/*
 * Impl
 */

// Self
#include "CelSphere.h"

// Local
#include "D3D11Client.h"
#include "DxTypes.h"
#include "D3D11Utils.h" // For shader compiles
#include "Scene.h"

using namespace Rendering;

CelSphere::CelSphere(D3D11Client &gc)
    : m_GraphicsClient(gc)
    , m_d3dDevice(gc.getConfig().getD3DDevice())
    , m_d3dImmContext(gc.getConfig().getD3DDeviceContext())
    , m_Config(gc.getConfig())  // so we dont have to get it everytime
    , m_bufConstellationLines(nullptr)  // this is valid C++11
    , m_bufLatGrid(nullptr)
    , m_bufLngGrid(nullptr)
    , m_fCelSphereRadius(1e3f)  // From D3D9 :
                                // the actual render distance for the celestial sphere
                                // is irrelevant, since it is rendered without z-buffer,
                                // but it must be within the frustum limits - check this
                                // in case the near and far planes are dynamically changed!
{
    oapiWriteLogV("CelSphere::CelSphere");
    initCelSphereEffect();

	loadStars();
	loadConstellationLines();
	createGrid();
}

CelSphere::~CelSphere()
{
    // Release D3D objects used for rendering
    ReleaseCOM(cb_VS_Star_Line);
    ReleaseCOM(cb_PS_Line);
    ReleaseCOM(m_ILCVertex);
    ReleaseCOM(m_VSStarLine);
    ReleaseCOM(m_PSStar);
    ReleaseCOM(m_PSLine);

    // Release the star ID3D11Buffers that were allocated during loadStars
    for ( auto starBuffer : m_vecStarBuffers ) {
        ReleaseCOM( starBuffer );
    }
    // The vector can be cleared out now
    m_vecStarBuffers.clear();

    ReleaseCOM(m_bufConstellationLines);
    ReleaseCOM(m_bufLatGrid);
    ReleaseCOM(m_bufLngGrid);
}

void CelSphere::initCelSphereEffect()  // fix this
{
    //shaders:
    ID3DBlob *shaderBlob = NULL;

    // Create VS for stars and (constellation)lines
    D3D11_INPUT_ELEMENT_DESC ilDesc[ ] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 } };

	if ( ClientUtils::CompileShaderChkErrors(
	        "Modules\\D3D11Shaders\\CelSphere.fx",
	        "VS_Star_Line",
	        m_Config.VSVersion,
	        m_Config.iShaderCompileFlag,
	        &shaderBlob) ) {
	    // Why this IL ? TODO
        HR( m_d3dDevice->CreateInputLayout(ilDesc, 2, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(),
			                               &m_ILCVertex) );
        HR( m_d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL,
                                            &m_VSStarLine) );
	}
    ReleaseCOM( shaderBlob );

    // Create separate PS for stars
    if (ClientUtils::CompileShaderChkErrors(
            "Modules\\D3D11Shaders\\CelSphere.fx",
            "PS_Star",
            m_Config.PSVersion,
            m_Config.iShaderCompileFlag,
            &shaderBlob) ) {
        // Input layout needed only for VS
        HR( m_d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL,
                                           &m_PSStar) );
    }
	ReleaseCOM(shaderBlob);

    // And another for constellation lines
    if (ClientUtils::CompileShaderChkErrors(
            "Modules\\D3D11Shaders\\CelSphere.fx",
            "PS_Line",
            m_Config.PSVersion,
            m_Config.iShaderCompileFlag,
            &shaderBlob) ) {
        HR( m_d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL,
                                           &m_PSLine) );
    }
	ReleaseCOM(shaderBlob);

    // Const buffer for VS contains the world-view-projection matrix float4 * 16 = 64 bytes
    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.ByteWidth = 64;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    HR( m_d3dDevice->CreateBuffer( &desc, NULL, &cb_VS_Star_Line ) );

    // Const buffer for constellation line PS containing only colors - float4 * 4 = 16 bytes
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.ByteWidth = 16;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    HR( m_d3dDevice->CreateBuffer( &desc, NULL, &cb_PS_Line ) );
}

void CelSphere::loadStars()
{
	const DWORD bsize = 8;
	double a, b, xz;

	// Do not load stars lower than prm->mag_lo, so set prm->mag_lo first
	StarRenderPrm *prm = (StarRenderPrm*)m_GraphicsClient.GetConfigParam( CFGPRM_STARRENDERPRM );

	if( prm->mag_lo > prm->mag_hi ) {
		if( prm->map_log )
			a = -log( prm->brt_min )/(prm->mag_lo - prm->mag_hi);
		else {
			a = (1.0 - prm->brt_min)/(prm->mag_hi - prm->mag_lo);
			b = prm->brt_min - prm->mag_lo*a;
		}
	}
	else {
		oapiWriteLog( "Inconsistent magnitude limits for background stars" );
	}

    // Star brightness levels
	int lvl, plvl = 256;
	StarRec *data = new StarRec [MAXSTARVERTEX];
	STARVERTEX *idata = new STARVERTEX [MAXSTARVERTEX];
	FILE *file;
	DWORD index, k, numVertices, idx = 0;
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA sdata;

	// The stellar database
	if( !(file = fopen( "Star.bin", "rb" )) ) {
		return;
	}

	m_dwNumStarVertices = 0;
	// Stars are read from file in decreasing magnitude order
	while( numVertices = fread( data, sizeof(StarRec), MAXSTARVERTEX, file ) ) {

	    // Out of the total stars read, not all may be bright enough to be shown
	    // Find out how many are - since the stars are in decreasing order of magnitude
	    // a simple linear search from the beginning will get us to the last star
	    // that can be shown. Perfect candidate for a std::for_each to quickly
	    // get to the cutoff - maybe try lambda in such cases ?
	    for ( index = 0; index < numVertices; ++index ) {
			if( data[index].mag > prm->mag_lo ) {
				numVertices = index;
				break;
			}
	    }

	    // numVertices now has the number of stars satisfying mag < = prm->mag_lo
		if( numVertices ) {
		    // We have some valid stars to show in this iteration/bunch

			for ( index = 0; index < numVertices; ++index ) {
				StarRec &rec = data[index];
				STARVERTEX &v = idata[index];

				xz = m_fCelSphereRadius*cos(rec.lat);
				v.pos.x = (float)(xz*cos(rec.lng));
				v.pos.y = (float)(xz*sin(rec.lng));
				v.pos.z = (float)(m_fCelSphereRadius*sin(rec.lat));

				// TODO: What is going on here ? m_lvlid ?
				if( prm->map_log )	v.col = (float)min( 1.0, max( prm->brt_min, exp( -(rec.mag - prm->mag_hi)*a ) ) );
				else				v.col = (float)min( 1.0, max( prm->brt_min, a*rec.mag + b ) );

				lvl = (int)(v.col*256.0*0.5);
				lvl = (lvl > 255 ? 255 : lvl);

				for( k = lvl; k < (DWORD)plvl; k++ )
					m_lvlid[k] = idx;
				plvl = lvl;
				idx++;
			}

			ZeroMemory( &desc, sizeof(desc) );
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.ByteWidth = 16*numVertices;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			ZeroMemory( &sdata, sizeof(sdata) );
			sdata.pSysMem = idata;

			ID3D11Buffer *newStarBuffer;
			HR( m_d3dDevice->CreateBuffer( &desc, &sdata, &newStarBuffer ) );
			m_vecStarBuffers.push_back(newStarBuffer);

			m_dwNumStarVertices += numVertices;
		}
		if( numVertices < MAXSTARVERTEX )
			break;
	}
	fclose( file );

	delete [ ] data;
	delete [ ] idata;

	// TODO: What ??
	for( index = 0; index < (DWORD)plvl; ++index )
		m_lvlid[index] = idx;
}

void CelSphere::loadConstellationLines()
{
	const DWORD maxline = 1000;	// plenty for default data base, but check with custom data bases!

	oapi::GraphicsClient::ConstRec *cline = new oapi::GraphicsClient::ConstRec [maxline];
	m_dwNumConstellationLines = m_GraphicsClient.LoadConstellationLines( maxline, cline ); // this calls Orbiter func - do not change
	if( m_dwNumConstellationLines ) {
		D3DXVECTOR4 *data = new D3DXVECTOR4 [m_dwNumConstellationLines*2];
		DWORD j;
		double xz;

		// Setup the line end points
		for( j = 0; j < m_dwNumConstellationLines; j++ ) {
			oapi::GraphicsClient::ConstRec *rec = cline+j;
			xz = m_fCelSphereRadius*cos( rec->lat1 );
			data[j*2].x = (float)(xz*cos( rec->lng1 ));
			data[j*2].z = (float)(xz*sin( rec->lng1 ));
			data[j*2].y = (float)(m_fCelSphereRadius*sin( rec->lat1 ));
			data[j*2].w = 1.0f;
			xz = m_fCelSphereRadius*cos( rec->lat2 );
			data[j*2+1].x = (float)(xz*cos( rec->lng2 ));
			data[j*2+1].z = (float)(xz*sin( rec->lng2 ));
			data[j*2+1].y = (float)(m_fCelSphereRadius*sin( rec->lat2 ));
			data[j*2+1].w = 1.0f;
		}

		D3D11_BUFFER_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = m_dwNumConstellationLines*2*16;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA sdata;
		ZeroMemory( &sdata, sizeof(sdata) );
		sdata.pSysMem = data;

		HR( m_d3dDevice->CreateBuffer( &desc, &sdata, &m_bufConstellationLines ) );

		delete [ ] data;
	}
	delete [ ] cline;
}

void CelSphere::createGrid()
{
	DWORD j, i, idx;
	double lat, lng, xz, y;

	// NSEG = 64, so 64 points along each of the 11 latitude circles, got by varying the
	// lng angle
	// each circle parallel to eq & 15 deg apart each
	// TODO: Check why 12 used here
	D3DXVECTOR4 *buf = new D3DXVECTOR4 [12*(NSEG+1)];
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA sdata;

	// Create CelSphere latitude grid, start from lat = -45 & upto lat = +75 TODO WHY ??
	// 11 circles, j=5 is equator
	for( j = idx = 0; j <= 10; ++j ) {
		lat = (j-5)*15.0*RAD;
		// This is the circle radius at y ht from sphere center parallel to eq. plane
		xz = m_fCelSphereRadius*cos( lat );
		y = m_fCelSphereRadius*sin( lat );

		// Get NSEG points on this lat circle using xz as rad - we are accumulating NSEG points for
		// the lat circle by varying the lng angle before calc each point
		for( i = 0; i <= NSEG; ++i ) {
			lng = PI2*(double)i/(double)NSEG;
			buf[idx].x = (float)(xz*cos( lng ));
			buf[idx].z = (float)(xz*sin( lng ));
			buf[idx].y = (float)y;
			buf[idx].w = 1.0f;
			++idx;
		}
	}

	ZeroMemory( &desc, sizeof(desc) );
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = 16*(NSEG+1)*11;//idx;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	
	ZeroMemory( &sdata, sizeof(sdata) );
	sdata.pSysMem = buf;

	// TODO: Should the buffer name be changed to lat buffer ?
	HR( m_d3dDevice->CreateBuffer( &desc, &sdata, &m_bufLngGrid ) );

	// Here we get 64 lat points along each of the 11 lng circles
	// So finally lng & lat lines *both* have 64 segments & its not 64 for lng & 11
	// for lat or vice versa
	for( j = idx = 0; j < 12; j++ ) {
		lng = j*15.0*RAD;
		for( i = 0; i <= NSEG; i++ ) {
			lat = PI2*(double)i/(double)NSEG;
			xz = m_fCelSphereRadius*cos( lat );
			y = m_fCelSphereRadius*sin( lat );
			buf[idx].x = (float)(xz*cos( lng ));
			buf[idx].z = (float)(xz*sin( lng ));
			buf[idx].y = (float)y;
			buf[idx].w = 1.0f;
			idx++;
		}
	}

	ZeroMemory( &desc, sizeof(desc) );
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = 16*(NSEG+1)*12;//idx;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	
	ZeroMemory( &sdata, sizeof(sdata) );
	sdata.pSysMem = buf;

	HR( m_d3dDevice->CreateBuffer( &desc, &sdata, &m_bufLatGrid ) );

	delete [ ] buf;
}

void CelSphere::renderCelestialSphere( D3DXMATRIX *VP, float skybrt, const D3DXVECTOR3 *bgcol )
{

    // Renders stars, constellations, labels.

//	static const D3DXCOLOR EGColor = D3DXCOLOR( 0.0f, 0.0f, 0.4f, 1.0f ), EColor = D3DXCOLOR( 0.0f, 0.0f, 0.8f, 1.0f );

	m_d3dImmContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP );
	// This does not set the value, only indicates the array of const buffers to use, only 1 in this case for WVP matrix
	m_d3dImmContext->VSSetConstantBuffers( 0, 1, &cb_VS_Star_Line );
	m_d3dImmContext->IASetInputLayout( m_ILCVertex );
	m_d3dImmContext->VSSetShader( m_VSStarLine, NULL, NULL );
	
	Rendering::Scene *pScene = m_GraphicsClient.getScene();
	DWORD planetariumMode = pScene->getPlanetariumFlag();
	if( planetariumMode & PLN_ENABLE ) {
		float linebrt = 1.0f - skybrt;

		// Must update the VS const buffer with the WVP matrix
		m_d3dImmContext->UpdateSubresource( cb_VS_Star_Line, 0, NULL, pScene->getViewProjection(), 0, 0 );

		// Use m_PSLine which can read the color from cb_PS_Line that is set here
		// For planetarium mode we need colors to be set from plugin
		m_d3dImmContext->PSSetShader( m_PSLine, NULL, NULL );
		m_d3dImmContext->PSSetConstantBuffers( 1, 1, &cb_PS_Line );

		// Ecliptic grid - this is aligned with the orbital plane of the solar system
		if( planetariumMode & PLN_EGRID ) {
			D3DXVECTOR4 vColor( 0.0f, 0.0f, 0.4f*linebrt, 1.0f );
			m_d3dImmContext->UpdateSubresource( cb_PS_Line, 0, NULL, &vColor, 0, 0 );
			renderGrid( !(planetariumMode & PLN_ECL) );
		}
		// Line of ecliptic
		if( planetariumMode & PLN_ECL ) {
			D3DXVECTOR4 vColor( 0.0f, 0.0f, 0.8f*linebrt, 1.0f );
			m_d3dImmContext->UpdateSubresource( cb_PS_Line, 0, NULL, &vColor, 0, 0 );
			renderGreatCircle();
		}

		// Celestial grid & celestial equator-use same points but use the angle of obliquity
		// to tilt frame before rendering - the celestial grid is titled wrt ecliptic grid & aligned with Earth's
		// equatorial plane, Earth being itself titled wrt orbital plane of solar system/ecliptic
		// http://en.wikipedia.org/wiki/File:AxialTiltObliquity.png
		if( planetariumMode & (PLN_CGRID | PLN_EQU) ) {
			static const double obliquity = 0.4092797095927;
			double coso = cos(obliquity),	sino = sin(obliquity);
			D3DXMATRIX rot(		1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, (float)coso, (float)sino, 0.0f,
								0.0f, -(float)sino, (float)coso, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f );

			D3DXMatrixMultiply( &rot, &rot, pScene->getViewProjection() );
			m_d3dImmContext->UpdateSubresource( cb_VS_Star_Line, 0, NULL, &rot, 0, 0 );

			if( planetariumMode & PLN_CGRID ) {
				D3DXVECTOR4 vColor( 0.35f*linebrt, 0.0f, 0.35f*linebrt, 1.0f );
				m_d3dImmContext->UpdateSubresource( cb_PS_Line, 0, NULL, &vColor, 0, 0 );
				renderGrid( !(planetariumMode & PLN_EQU) );
			}
			if( planetariumMode & PLN_EQU ) {
				D3DXVECTOR4 vColor( 0.7f*linebrt, 0.0f, 0.7f*linebrt, 1.0f );
				m_d3dImmContext->UpdateSubresource( cb_PS_Line, 0, NULL, &vColor, 0, 0 );
				renderGreatCircle();
			}
		}

		// Constellation lines
		m_d3dImmContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
		if( planetariumMode & PLN_CONST ) {
			D3DXVECTOR4 vColor( 0.4f*linebrt, 0.3f*linebrt, 0.2f*linebrt, 1.0f );
			m_d3dImmContext->UpdateSubresource( cb_PS_Line, 0, NULL, &vColor, 0, 0 );
			renderConstellations();
		}
	}

	// Stars - use m_PSStar which uses the color passed from m_VSStarLine
	m_d3dImmContext->UpdateSubresource( cb_VS_Star_Line, 0, NULL, pScene->getViewProjection(), 0, 0 );
	m_d3dImmContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
	m_d3dImmContext->PSSetShader( m_PSStar, NULL, NULL );
	renderStars( bgcol );

	const oapi::GraphicsClient::LABELLIST *cmlist;
	DWORD x = m_GraphicsClient.GetCelestialMarkers( &cmlist );
	x = x;
	renderCelestialMarkers( VP );
}

void CelSphere::renderGrid( bool eqline )
{
    const UINT CVertexStride = 16, VBOffset = 0;
    DWORD j;

    // Draw latitude lines using the longitude buffer, TODO check - comment out one of these and chk what gets drawn
    // 11 latitude lines, so that there can be a middle line and equal on each side
    m_d3dImmContext->IASetVertexBuffers( 0, 1, &m_bufLngGrid, &CVertexStride, &VBOffset );
    for( j = 0; j <= 10; j++ )
        if( eqline || j != 5 ) // if eqline = false then skip j = 5 line, i.e. do not draw equator, else do draw
            m_d3dImmContext->Draw( NSEG+1, j*(NSEG+1) ); // offset to proper latitude circle(after every NSEG+1 segments)

    // Draw longitude lines using the latitude buffer, TODO check
    // 12 longitude lines
    m_d3dImmContext->IASetVertexBuffers( 0, 1, &m_bufLatGrid, &CVertexStride, &VBOffset );
    for( j = 0; j < 12; j++ )
        m_d3dImmContext->Draw( NSEG+1, j*(NSEG+1) ); // offset to proper longitude circle perp to eq plane
}

void CelSphere::renderCelestialMarkers( D3DXMATRIX *VP )
{
/*	todo	*/
}

void CelSphere::renderStars( const D3DXVECTOR3 *bgcol )
{
	//nmax = -1;
	DWORD j, i, nmax = m_dwNumStarVertices;
	const UINT CVertexStride = 16, Offset = 0;
	
	// TODO: Mysterious stuff going on here !!
	if( bgcol ) {
		int bglvl = min( 255, (int)((min( bgcol->x, 1.0f ) + min( bgcol->y, 1.0f ) + min( bgcol->z, 1.0f ))*128.0f));
		nmax = min( nmax, (DWORD)m_lvlid[bglvl] );
	}

	// Render each buffer allocated in loadStars()
	for( i = j = 0; i < nmax; i += MAXSTARVERTEX, ++j ) {
		m_d3dImmContext->IASetVertexBuffers( 0, 1, &m_vecStarBuffers[j], &CVertexStride, &Offset );
		m_d3dImmContext->Draw( min( nmax-i, MAXSTARVERTEX ), 0 );
	}
}

void CelSphere::renderConstellations()
{
	const UINT CVertexStride = 16, VBOffset = 0;

	m_d3dImmContext->IASetVertexBuffers( 0, 1, &m_bufConstellationLines, &CVertexStride, &VBOffset );
	m_d3dImmContext->Draw( m_dwNumConstellationLines*2, 0 );
}



void CelSphere::renderGreatCircle()
{
	const UINT CVertexStride = 16, VBOffset = 0;

	// TODO: Check if this renders the celestial equator
	m_d3dImmContext->IASetVertexBuffers( 0, 1, &m_bufLngGrid, &CVertexStride, &VBOffset );
	m_d3dImmContext->Draw( (NSEG+1), 5*(NSEG+1) );
}
