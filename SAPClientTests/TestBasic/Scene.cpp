/*
 * Scene impl
 */

// Self
#include "Scene.h"

// Local
#include "D3D11Client.h"
#include "D3D11Utils.h"
#include "D3D11Config.h"

//#include "TextureMgr.h"
//#include "CelBackground.h"

using namespace Rendering;

Scene::Scene(D3D11Client &gc)
    : m_GraphicsClient(gc)
    , m_Config(gc.getConfig())  // so we dont have to get it everytime
    , m_objProxy(0)
    , m_objProxyNew(0)
{
    oapiWriteLogV("Scene::Scene");

}

Scene::~Scene()
{
    oapiWriteLogV("Scene::~Scene");

    // These may be needed later when they are allocated
    //delete m_SketchPad;
    //delete mCelBkgrnd;


}

//=================================================
//            Update.
//=================================================

// The camera co-ord system is probably left handed and the frustum points down +ve Z
bool Scene::isVisibleInCamera( D3DXVECTOR3 *pCnt, float radius )
{
    // Used by tilemanger, cloudmanager ? TODO: CHECK !!
	float z = m_d3dvec3CameraZ.x*pCnt->x + m_d3dvec3CameraZ.y*pCnt->y + m_d3dvec3CameraZ.z*pCnt->z;
    if( z < (-radius) ) return false; // TODO: not sure
    if( z < 0) z=-z; // z is now the projection for P's z along camera Z

    // Its correct, see
    // http://www.lighthouse3d.com/tutorials/view-frustum-culling/radar-approach-testing-spheres/
    float y = m_d3dvec3CameraY.x*pCnt->x + m_d3dvec3CameraY.y*pCnt->y + m_d3dvec3CameraY.z*pCnt->z;
    if( y < 0) y=-y; // must compare only +ves, y is now projection of P's y along camera Y
    // ( y > d + (m_vh*z) ) => return false; d = r * 1/cos α = radius * m_vhf
    if( y - (radius*m_vhf) > (m_vh*z) ) return false;

    // Similar to Y axis
    float x = m_d3dvec3CameraX.x*pCnt->x + m_d3dvec3CameraX.y*pCnt->y + m_d3dvec3CameraX.z*pCnt->z;
    if( x < 0 ) x=-x;
    if( x - (radius*m_vwf) > (m_vw*z) ) return false;

    // All good
    return true;
}

void Scene::saveRenderParams()
{
    oapiWriteLogV("Scene::saveRenderParams");
    MATRIX3 cameraRot;

    // Get the FoV/2 - see API doc
    m_dCamApertureNew = oapiCameraAperture();

	// TODO: check !, we get the aspect here as the window may have been dragged by user changing its dims ?
    m_Config.fAspect = m_fAspect = (float)m_Config.clientWidth/(float)m_Config.clientHeight;

    // This is the matrix for transforming the camera in the global frame into the current
    // viewing orientation - check how this is used
    oapiCameraRotationMatrix( &cameraRot );
    ClientUtils::M3ToDM( &m_d3dmatViewNew, &cameraRot );

    // Global camera position
    oapiCameraGlobalPos( &m_vec3CamPositionNew );

    // Find proxy planet/sun - body closest to camera
    m_objProxyNew = oapiCameraProxyGbody();

    //mCelBkgrnd->SaveParams( 8, BgLvl );

    // Current planetarium mode config
    m_dwPlanetariumFlag = *(DWORD*)m_GraphicsClient.GetConfigParam( CFGPRM_PLANETARIUMFLAG );

    // Get debug string - some plugin might have put in something there and this needs to be shown
    char *line = oapiDebugString();
    if ( line ) {
        strcpy_s( m_DebugString, line );
        m_iDebugLength = strlen( m_DebugString );
    }
    else {
        m_iDebugLength = 0;
    }

    // Virtual cockpit projection matrix, m_dCamApertureNew is half the FoV, so double it
    D3DXMatrixPerspectiveFovLH( &m_d3dmatProjNew, (float)(m_dCamApertureNew*2.0), m_Config.fAspect, 0.15f, 1e6 );
    m_d3dmatViewProjVirtCkpitNew = m_d3dmatViewNew*m_d3dmatProjNew;

    // The main orbiter external projection matrix built using a D3D function
    D3DXMatrixPerspectiveFovLH( &m_d3dmatProjNew, (float)(m_dCamApertureNew*2.0), m_Config.fAspect, m_fNearPlane, m_fFarPlane );
    m_d3dmatProjNew = m_d3dmatViewNew*m_d3dmatProjNew;
}

void Scene::update()
{
    oapiWriteLogV("Scene::update");

    // Required during rendering
	m_d3dmatView = m_d3dmatViewNew;
    m_d3dmatProj = m_d3dmatProjNew;
    m_d3dmatViewProj = m_d3dmatViewProjNew;            //viewport matrix for exterior views
    m_d3dmatViewProjVirtCkpit = m_d3dmatViewProjVirtCkpitNew;        //viewport matrix for VC
    m_vec3CamPosition = m_vec3CamPositionNew;
    m_dCamAperture = m_dCamApertureNew;

    // View height/width, ht/width factor etc for frustum culling
    m_vh   = (float)tan(m_dCamAperture);
    m_vw   = m_vh*m_Config.fAspect;
    m_vhf  = (float)(1.0f/cos(m_dCamAperture));
    m_vwf  = m_vhf*m_Config.fAspect;

    // Required for frustum culling
    m_d3dvec3CameraX = D3DXVECTOR3(m_d3dmatView._11, m_d3dmatView._21, m_d3dmatView._31);
    m_d3dvec3CameraY = D3DXVECTOR3(m_d3dmatView._12, m_d3dmatView._22, m_d3dmatView._32);
    m_d3dvec3CameraZ = D3DXVECTOR3(m_d3dmatView._13, m_d3dmatView._23, m_d3dmatView._33);

    // Maybe

    m_objProxy = m_objProxyNew;
}

// Simple dot product of camera dir and point's position vector in global frame
bool Scene::isVisiblePoint( D3DXVECTOR3 *vec )
{
    return (D3DXVec3Dot( vec, &m_d3dvec3CameraZ ) > 0.0f ? false : true);
}


//=================================================
//            Rendering.
//=================================================

void Scene::renderStarsAndPlanets()
{
    oapiWriteLogV("Scene::renderStarsAndPlanets");
    int distcomp( const void *arg1, const void *arg2 );    //compares object by their distance from camera

	D3DXCOLOR clearColor(m_d3dvec3SkyColor.x, m_d3dvec3SkyColor.y, m_d3dvec3SkyColor.z, 0.0f);
    
    m_Config.ClearRenderTargetView(m_d3dRTVRenderTarget, clearColor );
    
	m_Config.ClearDepthStencilView(m_d3dDSVRenderTarget, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    setRenderTarget();

//  iCtx->ClearRenderTargetView( RTarget_RTV, ClearColor );
    m_Config.OMSetDepthStencilStateNoDepthNoStencil(0);

    D3D11_VIEWPORT viewport;
    viewport.Width = (FLOAT)m_Config.clientWidth;
    viewport.Height = (FLOAT)m_Config.clientHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_Config.RSSetViewports( 1, &viewport );

    //================================================
    //                        Stars - soon !!
    //================================================
    //background
    //mCelBkgrnd->Render();


}

void Scene::renderRest()
{

    //==========================================================================================================
    //        Vessels: Shadows -> Meshes -> Beacons -> Exhaust -> Particle Streams -> Markers
    //==========================================================================================================

}

void Scene::setRenderTarget()
{
    m_Config.OMSetRenderTargets( 1, m_d3dRTVRenderTarget, m_d3dDSVRenderTarget );
}

/*===========================================================
 *                    INIT
 *===========================================================
 */
#pragma region init
void Scene::init3D()
{
    oapiWriteLogV("Scene::init3D");

    // Create swap chain with chosen AA setting:
    DXGI_RATIONAL RR;
    RR.Numerator = 60;
    RR.Denominator = 1;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory( &swapChainDesc, sizeof( swapChainDesc ) );
    swapChainDesc.BufferCount = 1;
    if (!m_Config.bFullScreenWindow)
    {
        swapChainDesc.BufferDesc.Width = m_Config.clientWidth;
        swapChainDesc.BufferDesc.Height = m_Config.clientHeight;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    }
    else
    {
        swapChainDesc.BufferDesc = m_Config.dxgiMode;
    }
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags = 0;
#ifdef _DEBUG
    //swapChainDesc.Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    swapChainDesc.OutputWindow = m_GraphicsClient.m_hWindow;
    swapChainDesc.SampleDesc = m_Config.dxgiCurrentAAMode;


    swapChainDesc.Windowed = m_Config.bFullScreenWindow ? FALSE : TRUE;

    HR( m_Config.CreateSwapChain(&swapChainDesc) );

    // Back buffer and depth/stencil buffer:
    HR( m_Config.GetBuffer( 0,
                            __uuidof(ID3D11Texture2D),
                            m_d3dtex2DBackBuffer) );

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    ZeroMemory( &renderTargetViewDesc, sizeof(renderTargetViewDesc) );
    renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
	HR(m_Config.CreateRenderTargetView(m_d3dtex2DBackBuffer,
	                                   &renderTargetViewDesc,
	                                   m_d3dRTVBackBuffer));

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    ZeroMemory( &depthStencilDesc, sizeof( D3D11_TEXTURE2D_DESC ) );
    depthStencilDesc.Width = m_Config.clientWidth;
    depthStencilDesc.Height = m_Config.clientHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc = m_Config.dxgiCurrentAAMode;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.MiscFlags = 0;

    ID3D11Texture2DPtr depthStencil;
    HR( m_Config.CreateTexture2D( &depthStencilDesc, NULL, depthStencil ) );

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory( &depthStencilViewDesc, sizeof( depthStencilViewDesc ) );
    depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension =
	        m_Config.dxgiCurrentAAMode.Count != 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS :
	                                                D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

	HR(m_Config.CreateDepthStencilView(depthStencil,
	                                   &depthStencilViewDesc,
	                                   m_d3dDSVRenderTarget));
    

    // Set correct viewport
    D3D11_VIEWPORT viewport;
    viewport.Width = (float)m_Config.clientWidth;
    viewport.Height = (float)m_Config.clientHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    m_Config.RSSetViewports( 1, &viewport );

    // GDI-compatible buffer, TODO: Since we may not support GDI anymore, so do we need this ?
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory( &textureDesc, sizeof(textureDesc) );
    textureDesc.Width = m_Config.clientWidth;
    textureDesc.Height = m_Config.clientHeight;
    textureDesc.MipLevels = 1;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.ArraySize = 1;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    HR( m_Config.CreateTexture2D( &textureDesc,
                                  NULL,
                                  m_d3dtex2DPixelBuffer ) );

    // Shader Resource View
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewdesc;
    ZeroMemory( &shaderResourceViewdesc, sizeof(shaderResourceViewdesc));
    shaderResourceViewdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    shaderResourceViewdesc.Texture2D.MipLevels = 1;
    shaderResourceViewdesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    HR( m_Config.CreateShaderResourceView( m_d3dtex2DPixelBuffer,
                                           &shaderResourceViewdesc,
                                           m_d3dSRVPixelBuffer ) );

    // Render target view
    ZeroMemory( &renderTargetViewDesc, sizeof(renderTargetViewDesc) );
    renderTargetViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    HR(  m_Config.CreateRenderTargetView( m_d3dtex2DPixelBuffer,
                                          &renderTargetViewDesc,
                                          m_d3dRTVPixelBuffer ) );

    // No additional buffers needed. TODO: What does this mean here ?!
	m_d3dRTVRenderTarget = m_d3dRTVBackBuffer;
	m_d3dSRVRenderTarget = NULL;
	m_d3dtex2DRenderTarget = m_d3dtex2DBackBuffer;

	// TODO: Why do we need this ?
    HR( m_d3dtex2DPixelBuffer->QueryInterface(__uuidof(IDXGISurface1),
                                              reinterpret_cast<void**>(&m_dxgisurfPixelBuffer) ) );

    // Clipping planes
    m_fNearPlane = 1.0f;
    m_fFarPlane = 1e6;

    // Projection matrix
    D3DXMatrixIdentity( &m_d3dmatProj );
    // Camera global position
    m_vec3CamPosition.x = m_vec3CamPosition.y = m_vec3CamPosition.z = 0.0;

}

void Scene::initStatics()
{
    // Unused
}

void Scene::initObjects()
{
    // Unused
    // All bases should be created in their planets.
}

#pragma endregion

void
Scene::exit3D()
{
    oapiWriteLogV("Scene::exit3D");
    BOOL isFullscreen = FALSE;
	m_Config.GetFullscreenState(isFullscreen);
    if (isFullscreen)
    {
		m_Config.SetFullscreenState();
    }
  
   // CelBackground::ExitCelBackgroundEffect();
    
}
