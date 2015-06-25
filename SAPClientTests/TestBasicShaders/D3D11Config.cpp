/*
 * Impl
 */

// Self
#include "D3D11Config.h"

// std
#include <array>  // C++11
#include <string>

// Local
#include "D3D11Client.h"
#include "Globals.h"     // for SHOW_MESSAGE() macro

// Debugging - causes DEBUG flags to be added to createDevice()
//#define _DEBUG



D3D11Config::D3D11Config(const D3D11Client &gc)
    : dwWidth(0)
    , dwHeight(0)
    , clientWidth(0)
    , clientHeight(0)
    , dwSketchPadMode(1)
    , dwTextureMipMapCount(0)
    , bFullScreenWindow(false)
    , bNormalMaps(false)
    , bBumpMaps(false)
    , bPreloadTiles(false)
    , fMFDTransparency(0.9f)
    , FILTER_MeshTexture(D3D11_FILTER_MIN_MAG_MIP_LINEAR)
    , FILTER_PlanetTexture(D3D11_FILTER_MIN_MAG_MIP_LINEAR)
    , AF_FACTOR_MeshTexture(1)
    , AF_FACTOR_PlanetTexture(1)
    , dxgiRTFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    , dxgiOutput(0)
    , dxgiAdapter(0)
    , m_GraphicsClient(gc)
    , m_dxgiFactory(0)
    , m_d3dDevice(0)
    , m_d3dImmContext(0)
    , m_dxgiSwapChain(0)
    , iShaderCompileFlag(0)
{
    oapiWriteLogV("D3D11Config::D3D11Config");
    dxgiCurrentAAMode.Count = 1;
    dxgiCurrentAAMode.Quality = 0;

}

D3D11Config::~D3D11Config()
{
    oapiWriteLogV("D3D11Config::~D3D11Config");

    vecAAModes.clear();

    // D3D11 COM objects are managed through smart pointers, no need to release here
}

void
D3D11Config::findSupportedAAModes()
{
    oapiWriteLogV("D3D11Config::findSupportedAAModes");

    std::array<AA_MODE_DESC, 9> tmp;
    bool flag[9];
    flag[0] = true;

    tmp[0].desc = "No AA";
    tmp[0].SampleDesc.Count = 1;
    tmp[0].SampleDesc.Quality = 0;

    tmp[1].desc = "MSAA x2";
    tmp[1].SampleDesc.Count = 2;
    tmp[1].SampleDesc.Quality = 0;

    tmp[2].desc = "MSAA x4";
    tmp[2].SampleDesc.Count = 4;
    tmp[2].SampleDesc.Quality = 0;

    tmp[3].desc = "MSAA x6";
    tmp[3].SampleDesc.Count = 6;
    tmp[3].SampleDesc.Quality = 0;

    tmp[4].desc = "MSAA x8";
    tmp[4].SampleDesc.Count = 8;
    tmp[4].SampleDesc.Quality = 0;

    tmp[5].desc = "CSAA x8";
    tmp[5].SampleDesc.Count = 4;
    tmp[5].SampleDesc.Quality = 8;

    tmp[6].desc = "CSAA x8Q";
    tmp[6].SampleDesc.Count = 8;
    tmp[6].SampleDesc.Quality = 8;

    tmp[7].desc = "CSAA x16";
    tmp[7].SampleDesc.Count = 4;
    tmp[7].SampleDesc.Quality = 16;

    tmp[8].desc = "CSAA x16Q";
    tmp[8].SampleDesc.Count = 8;
    tmp[8].SampleDesc.Quality = 16;

    UINT j, num = 0;
    m_d3dDevice->CheckMultisampleQualityLevels( dxgiRTFormat, 2, &num );
    flag[1] = num ? true : false;

    m_d3dDevice->CheckMultisampleQualityLevels( dxgiRTFormat, 4, &num );
    flag[2] = num ? true : false;
    flag[5] = num >= 9 ? true : false;
    flag[7] = num >= 17 ? true : false;

    m_d3dDevice->CheckMultisampleQualityLevels( dxgiRTFormat, 6, &num );
    flag[3] = num ? true : false;

    m_d3dDevice->CheckMultisampleQualityLevels( dxgiRTFormat, 8, &num );
    flag[4] = num ? true : false;
    flag[6] = num >= 9 ? true : false;
    flag[8] = num >= 17 ? true : false;

    m_FileConfig.AA_mode = 0;
    m_FileConfig.AA_count = 0;
    for( j = 0; j < 9; j++ ) {
        if ( flag[j] ) {
            m_FileConfig.AA_count++;
        }
    }

    int ctr = 0;
    for( j = 0; j < 9; j++ ) {
        if( flag[j] ) {
            vecAAModes.push_back(tmp[j]);
            ctr++;
        }
    }
}

bool
D3D11Config::loadConfig()
{
    oapiWriteLogV("D3D11Config::loadConfig");

    // Check if the file is not there
    if (!std::ifstream("D3D11Client.cfg") ) {
        // Its not there so init this D3D11Config object and create file in saveConfig()
        setDefault();
        saveConfig();
        return true;
    }

    // It exists, read it
    FILEHANDLE hCfgFile = oapiOpenFile( "D3D11Client.cfg", FILE_IN, ROOT );

    // D3D11 options
    oapiReadItem_int( hCfgFile, "Supported AA modes", m_FileConfig.AA_count );
    oapiReadItem_int( hCfgFile, "Selected AA mode", m_FileConfig.AA_mode );

    // If we could not get AA_count, the file could not be opened
    // TODO:  Apparently AA_count HAS to be there ? Why ?
    if (m_FileConfig.AA_count < 1) {
        oapiWriteLog( "ERROR : Unable to load config file" );
        oapiCloseFile( hCfgFile, FILE_IN );
        setDefault();
        saveConfig();
        return true;
    }

    // Read the AA modes, need to use strcpy because of Orbiter functions
    vecAAModes.resize(m_FileConfig.AA_count);
    int tmp;
    char line[32], tag[64], temp[16];
    strcpy( line, "AA_mode" );
    for(int index = 0; index < m_FileConfig.AA_count; ++index ) {
        sprintf( tag, "%s_%d", line, index);

        oapiReadItem_string( hCfgFile, tag, temp);
        vecAAModes[index].desc = temp;

        sprintf( tag, "%s_%d_count", line, index);
        oapiReadItem_int( hCfgFile, tag, tmp );
        vecAAModes[index].SampleDesc.Count = tmp;

        sprintf( tag, "%s_%d_quality", line, index);
        oapiReadItem_int( hCfgFile, tag, tmp );
        vecAAModes[index].SampleDesc.Quality = tmp;
    }

    oapiReadItem_int( hCfgFile, "Threading", m_FileConfig.Thread_mode );
    oapiReadItem_int( hCfgFile, "Sketchpad mode", m_FileConfig.Sketchpad_mode );
    oapiReadItem_int( hCfgFile, "MFD filter", m_FileConfig.MFD_filter );
    oapiReadItem_int( hCfgFile, "MFD transparency", m_FileConfig.MFD_transparency );
    oapiReadItem_int( hCfgFile, "PP Effect Enabled", m_FileConfig.PP_effect_enable );

    // Read vessel related options
    oapiReadItem_int( hCfgFile, "Mesh mip maps", m_FileConfig.Mesh_Texture_Mip_maps );
    oapiReadItem_int( hCfgFile, "Mesh texture filter", m_FileConfig.Mesh_Texture_Filter );
    oapiReadItem_int( hCfgFile, "Enable mesh normal maps", m_FileConfig.Mesh_Normal_maps );
    oapiReadItem_int( hCfgFile, "Enable mesh bump maps", m_FileConfig.Mesh_Bump_maps );
    oapiReadItem_int( hCfgFile, "Enable mesh specular maps", m_FileConfig.Mesh_Specular_maps );
    oapiReadItem_int( hCfgFile, "Enable mesh emissive maps", m_FileConfig.Mesh_Emissive_maps );

//    oapiReadItem_int( hCfgFile, "Enable PS local lighting", m_FileConfig.PStream_LLights );
//    oapiReadItem_int( hCfgFile, "Enable base local lighting", m_FileConfig.Base_LLights );

    // Read planet related options
    oapiReadItem_int( hCfgFile, "Planet texture filter", m_FileConfig.Planet_Texture_filter );
    oapiReadItem_int( hCfgFile, "Planet tile loading frequency", m_FileConfig.Planet_Tile_loading_Freq );
    oapiCloseFile( hCfgFile, FILE_IN );

    // Check values
    m_FileConfig.Thread_mode = m_FileConfig.Thread_mode > 1 ? 0 : (m_FileConfig.Thread_mode < 0 ? 0 : m_FileConfig.Thread_mode);
    m_FileConfig.Sketchpad_mode = m_FileConfig.Sketchpad_mode > 2 ? 0 : (m_FileConfig.Sketchpad_mode < 0 ? 0 : m_FileConfig.Sketchpad_mode);
    m_FileConfig.Mesh_Texture_Mip_maps = m_FileConfig.Mesh_Texture_Mip_maps > 2 ? 0 : (m_FileConfig.Mesh_Texture_Mip_maps < 0 ? 0 : m_FileConfig.Mesh_Texture_Mip_maps);
    m_FileConfig.Mesh_Texture_Filter = m_FileConfig.Mesh_Texture_Filter > 4 ? 0 : (m_FileConfig.Mesh_Texture_Filter < 0 ? 0 : m_FileConfig.Mesh_Texture_Filter);
    m_FileConfig.Planet_Texture_filter = m_FileConfig.Planet_Texture_filter > 4 ? 0 : (m_FileConfig.Planet_Texture_filter < 0 ? 0 : m_FileConfig.Planet_Texture_filter);
    m_FileConfig.Planet_Tile_loading_Freq = m_FileConfig.Planet_Tile_loading_Freq > 200 ? 50 : (m_FileConfig.Planet_Tile_loading_Freq < 0 ? 50 : m_FileConfig.Planet_Tile_loading_Freq);
    return true;
}

void
D3D11Config::setDefault()
{
    oapiWriteLogV("D3D11Config::setDefault");

    m_FileConfig.Thread_mode = 0;
    findSupportedAAModes();
    m_FileConfig.Sketchpad_mode = 1;
    m_FileConfig.MFD_filter = 1;
    m_FileConfig.MFD_transparency = 90;

    m_FileConfig.Mesh_Texture_Mip_maps = 2;
    m_FileConfig.Mesh_Texture_Filter = 4;
    m_FileConfig.Mesh_Normal_maps = 1;
    m_FileConfig.Mesh_Bump_maps = 0;
    m_FileConfig.Mesh_Specular_maps = 0;
    m_FileConfig.Mesh_Emissive_maps = 0;
    m_FileConfig.PStream_LLights = 0;
    m_FileConfig.Base_LLights = 0;

    m_FileConfig.Planet_Texture_filter = 1;
    m_FileConfig.Planet_Tile_loading_Freq = 50;
}

void
D3D11Config::saveConfig()
{
    oapiWriteLogV("D3D11Config::saveConfig");

    // Clear out the file
    std::ofstream ofs;
    ofs.open("D3D11Client.cfg", std::ofstream::out | std::ofstream::trunc);
    ofs.close();

    FILEHANDLE hCfgFile = oapiOpenFile( "D3D11Client.cfg", FILE_OUT, ROOT );

    if (!hCfgFile ) {
        oapiWriteLog( "Unable to create config file" );
        return;
    }

    //GENERAL:
    oapiWriteItem_int( hCfgFile, "Threading", m_FileConfig.Thread_mode );
    oapiWriteItem_int( hCfgFile, "Supported AA modes", m_FileConfig.AA_count );
    oapiWriteItem_int( hCfgFile, "Selected AA mode", m_FileConfig.AA_mode );

    // Format and write the AA modes, need to use strcpy because of Orbiter functions
    char line[32], tag[64], temp[16];
    strcpy( line, "AA_mode" );
    for( WORD j = 0; j < m_FileConfig.AA_count; j++ ) {
        sprintf( tag, "%s_%d", line, j );
        oapiWriteItem_string( hCfgFile, tag, temp);
		vecAAModes[j].desc = temp;

        sprintf( tag, "%s_%d_count", line, j );
        oapiWriteItem_int( hCfgFile, tag, vecAAModes[j].SampleDesc.Count );

        sprintf( tag, "%s_%d_quality", line, j );
        oapiWriteItem_int( hCfgFile, tag, vecAAModes[j].SampleDesc.Quality );
    }

    oapiWriteItem_int( hCfgFile, "Sketchpad mode", m_FileConfig.Sketchpad_mode );
    oapiWriteItem_int( hCfgFile, "MFD filter", m_FileConfig.MFD_filter );
    oapiWriteItem_int( hCfgFile, "MFD transparency", m_FileConfig.MFD_transparency );
    oapiWriteItem_int( hCfgFile, "PP Effect Enabled", m_FileConfig.PP_effect_enable );

    //VESSELS:
    oapiWriteItem_int( hCfgFile, "Mesh mip maps", m_FileConfig.Mesh_Texture_Mip_maps );
    oapiWriteItem_int( hCfgFile, "Mesh texture filter", m_FileConfig.Mesh_Texture_Filter );
    oapiWriteItem_int( hCfgFile, "Enable mesh normal maps", m_FileConfig.Mesh_Normal_maps );
    oapiWriteItem_int( hCfgFile, "Enable mesh bump maps", m_FileConfig.Mesh_Bump_maps );
    oapiWriteItem_int( hCfgFile, "Enable mesh specular maps", m_FileConfig.Mesh_Specular_maps );
    oapiWriteItem_int( hCfgFile, "Enable mesh emissive maps", m_FileConfig.Mesh_Emissive_maps );
//    oapiWriteItem_int( hCfgFile, "Enable PS local lighting", m_FileConfig.PStream_LLights );
//    oapiWriteItem_int( hCfgFile, "Enable base local lighting", m_FileConfig.Base_LLights );

    //PLANETS:
    oapiWriteItem_int( hCfgFile, "Planet texture filter", m_FileConfig.Planet_Texture_filter );
    oapiWriteItem_int( hCfgFile, "Planet tile loading frequency",
                                       m_FileConfig.Planet_Tile_loading_Freq );
    oapiCloseFile( hCfgFile, FILE_OUT );
}

void
D3D11Config::applyConfig()
{
    oapiWriteLogV("D3D11Config::applyConfig");

    // Apply the settings read from the config file
	dxgiCurrentAAMode = vecAAModes[m_FileConfig.AA_mode].SampleDesc;
	dwSketchPadMode = m_FileConfig.Sketchpad_mode;
	fMFDTransparency = (float)m_FileConfig.MFD_transparency / 100.0f;

	switch (m_FileConfig.Mesh_Texture_Mip_maps) {
    case 0:
        dwTextureMipMapCount = 1;
        break;
    case 1:
        dwTextureMipMapCount = 4;
        break;
    case 2:
        dwTextureMipMapCount = 0;
        break;
    }

	FILTER_MeshTexture = setFilter(m_FileConfig.Mesh_Texture_Filter, AF_FACTOR_MeshTexture);

	bNormalMaps = m_FileConfig.Mesh_Normal_maps ? true : false;
	bBumpMaps = m_FileConfig.Mesh_Bump_maps ? true : false;
	bSpecularMaps = m_FileConfig.Mesh_Specular_maps ? true : false;
	bEmissiveMaps = m_FileConfig.Mesh_Emissive_maps ? true : false;

	FILTER_PlanetTexture = setFilter(m_FileConfig.Planet_Texture_filter, AF_FACTOR_PlanetTexture);

    bPreloadTiles = false;
	iEnablePPEffects = m_FileConfig.PP_effect_enable;
//    bPreloadTiles = CFG.P_preload_tiles ? true : false;
}

void
D3D11Config::dumpConfig() const
{
    // TODO dump file and this config's data to log
}

bool
D3D11Config::enumerateDisplaysCreateD3DDevice(oapi::GraphicsClient::VIDEODATA *vdata)
{
    oapiWriteLogV("D3D11Config::enumerateDisplaysCreateD3DDevice");
	
    DXGI_ADAPTER_DESC adapterDesc;
    DXGI_OUTPUT_DESC outputDesc;

    // First enumerate the monitors
    PHYSICAL_MONITOR monitor;
    UINT adapterCount = 0, outputCount = 0;
    char *line, cbuf[128];
    HR( CreateDXGIFactory1( __uuidof(IDXGIFactory1), (void**)(&m_dxgiFactory) ) );
    if (!m_dxgiFactory) {
        SHOW_MESSAGE("Initialization error", "CreateDXGIFactory1 failed");
        return false;
    }
    while ( m_dxgiFactory->EnumAdapters1( adapterCount, &dxgiAdapter ) == S_OK ) {
        dxgiAdapter->GetDesc( &adapterDesc );
        while ( dxgiAdapter->EnumOutputs( outputCount, &dxgiOutput ) == S_OK ) {
			dxgiOutput->GetDesc(&outputDesc);

            GetPhysicalMonitorsFromHMONITOR( outputDesc.Monitor, 1, &monitor );
            wcstombs( cbuf, monitor.szPhysicalMonitorDescription, 128 );
            DestroyPhysicalMonitors( 1, &monitor );

            line = new char [128];
            vecDXGIAdapterOutputDesc.push_back( line );
			wcstombs(vecDXGIAdapterOutputDesc[outputCount], adapterDesc.Description, 128);
			strcat(vecDXGIAdapterOutputDesc[outputCount], cbuf);

            vecDXGIOutputs.push_back( dxgiOutput );
            ++outputCount;
        }

        vecDXGIAdapters.push_back( dxgiAdapter );
        ++adapterCount;
    }
    UINT numOutputs = outputCount;

    if ( vdata->deviceidx < 0 || (UINT)vdata->deviceidx > outputCount ) {
        vdata->deviceidx = 0;
    }

    UINT numModes = 0;
    vecDXGIOutputs[vdata->deviceidx]->GetDisplayModeList( DXGI_FORMAT_B8G8R8A8_UNORM,
                                                          0, &numModes, NULL );
    DXGI_MODE_DESC *modes = new DXGI_MODE_DESC [numModes];
    vecDXGIOutputs[vdata->deviceidx]->GetDisplayModeList( DXGI_FORMAT_B8G8R8A8_UNORM,
                                                          0, &numModes, modes );

    dxgiAdapter = vecDXGIAdapters[vdata->deviceidx];
    dxgiOutput = vecDXGIOutputs[vdata->deviceidx];
    dxgiMode = modes[vdata->modeidx];

    //FullScreenWindow = vdata->pageflip;
    driverType = /*vdata->forceenum ? D3D_DRIVER_TYPE_REFERENCE : */D3D_DRIVER_TYPE_UNKNOWN;
    bFullScreenWindow = vdata->fullscreen;
    dwWidth = vdata->winw;
    dwHeight = vdata->winh;

    D3D_FEATURE_LEVEL featureLevels[ ] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT result = D3D11CreateDevice(
        dxgiAdapter,
        driverType,  // Not unknown
        0,                              // no software device
        createDeviceFlags,
        featureLevels,
        3,                              // Why 3 ? !!
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &(succeededFeatureLevel),
        &m_d3dImmContext);

    if (!m_d3dDevice) {
        char errMsg[200];
        sprintf(errMsg, "Device creation failed.\nError code: 0x%08p", result);
        MessageBoxA( m_GraphicsClient.m_hWindow, errMsg, "Critical error.",
                     MB_ICONERROR | MB_OK );
    }

    // D3D11 Debugging
  //  m_d3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_d3dDebug));

	return true;
}

D3D11_FILTER D3D11Config::setFilter(
    int value,
    DWORD &afactor)  // TODO: whats afactor !!
{
    oapiWriteLogV("D3D11Config::setFilter");

    if ( value < 0 || value > 4 ) {
        return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }

    D3D11_FILTER ret;
    switch( value ) {
    case 0:
        ret = D3D11_FILTER_MIN_MAG_MIP_POINT;
        afactor = 1;
        break;
    case 1:
        ret = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        afactor = 1;
        break;
    case 2:
        ret = D3D11_FILTER_ANISOTROPIC;
        afactor = 4;
        break;
    case 3:
        ret = D3D11_FILTER_ANISOTROPIC;
        afactor = 8;
        break;
    case 4:
        ret = D3D11_FILTER_ANISOTROPIC;
        afactor = 16;
        break;
    }

    return ret;
}

void
D3D11Config::initShaderLevels()
{
    D3D_FEATURE_LEVEL feature = m_d3dDevice->GetFeatureLevel();

    switch( feature ) {
    case D3D_FEATURE_LEVEL_9_1:
        strcpy( VSVersion, "vs_4_0_level_9_1" );
        strcpy( GSVersion, "gs_4_0_level_9_1" );
        strcpy( PSVersion, "ps_4_0_level_9_1" );
        break;
    case D3D_FEATURE_LEVEL_9_2:
        strcpy( VSVersion, "vs_4_0_level_9_1" );
        strcpy( GSVersion, "gs_4_0_level_9_1" );
        strcpy( PSVersion, "ps_4_0_level_9_1" );
        break;
    case D3D_FEATURE_LEVEL_9_3:
        strcpy( VSVersion, "vs_4_0_level_9_3" );
        strcpy( GSVersion, "gs_4_0_level_9_3" );
        strcpy( PSVersion, "ps_4_0_level_9_3" );
        break;
    case D3D_FEATURE_LEVEL_10_0:
        strcpy( VSVersion, "vs_4_0" );
        strcpy( GSVersion, "gs_4_0" );
        strcpy( PSVersion, "ps_4_0" );
        break;
    case D3D_FEATURE_LEVEL_10_1:
        strcpy( VSVersion, "vs_4_0" );
        strcpy( GSVersion, "gs_4_0" );
        strcpy( PSVersion, "ps_4_0" );
        break;
    case D3D_FEATURE_LEVEL_11_0:
        strcpy( VSVersion, "vs_5_0" );
        strcpy( GSVersion, "gs_5_0" );
        strcpy( PSVersion, "ps_5_0" );
        break;
    }

    iShaderCompileFlag =
        D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_OPTIMIZATION_LEVEL3;
}

