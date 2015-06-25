#pragma once
/*
 * The D3D11Config class loads and sets up D3D11. It maintains configuration information
 * about the current state of D3D11 including contexts, devices etc
 *
 * We first load the info from the .cfg into a config file specific structure D3D11FileConfig
 * Then use that to create the best possible D3D11 setup we can. The settings loaded from the
 *.cfg file are available in a D3D11FileConfig structure present as a member of D3D11Config.
 * So the settings already present in D3D11FileConfig are not repeated as member variables in
 * D3D11Config. The combined info in D3D11FileConfig and D3D11Config is used by many classes.
 *
 * The D3D11Config class also enumerates and creates the D3D11 context and device.
 * WARN: Be careful of making changes here.
 */

//#define DEBUG
#define _CRT_SECURE_NO_DEPRECATE

// Orbiter
#include "..\include\OrbiterAPI.h"
#include "..\include\GraphicsAPI.h"
#include "..\include\Orbitersdk.h"

// Windows
#include <physicalmonitorenumerationapi.h>

// D3D11
#include <d3d11.h> // various
#include <DxErr.h> // for DXTraceA
#include <d3dx9math.h>   // D3DXCOLOR, D3DXMATRIX etc

// std
#include <vector>

// Local
#include "DxTypes.h"

/*
 * Anti-aliasing modes:
 * No AA
 * MSAA x2
 * MSAA x4
 * MSAA x8
 * CSAA x8     CFAA x8
 * CSAA x8Q    CFAA x8Q
 * CSAA x16    CFAA x16
 * CSAA x16Q   CFAA x16Q
 *
 * Video Card Vendor IDs ? :
 * Not recognized 0
 * Not NVidia         4318
 * Not AMD/ATI        1022
 * Not Intel          8086 :)
 */

/*
 *  The structure representing the config file and is a hint for the final
 *  D3D11 setup
 */
struct D3D11FileConfig {
    int
        Thread_mode,
        AA_count,
        AA_mode,
        Sketchpad_mode,
        MFD_filter,
        MFD_transparency,
        PP_effect_enable,

        Mesh_Texture_Mip_maps,
        Mesh_Texture_Filter,
        Mesh_Normal_maps,
        Mesh_Bump_maps,
        Mesh_Specular_maps,
        Mesh_Emissive_maps,
        Mesh_Base_LLights,
        Base_LLights,
        PStream_LLights,
        PlanetTile_LLights,

        Planet_Texture_filter,
        Planet_Tile_loading_Freq;
};

struct AA_MODE_DESC {
    std::string desc;
    DXGI_SAMPLE_DESC SampleDesc;
};

class D3D11Client;

/*
 * The D3D11Config class.
 * An object of this class is present as a member of D3D11Client.
 * When the client object is destroyed, then the dtor of this is called.
 * D3D11Config maintains many COM objects through smart pointers. It manages
 * access to many important DirectX objects through its accessor functions
 * (except those that are required in specific rendering classes like in
 * Scene and can be maintained there).
 */
class D3D11Config {

public:
    D3D11Config(const D3D11Client &gc);

    // Release COM object members
    ~D3D11Config();

    /*
     * Enumerate displays and pick the one chosen by Orbiter in the VIDEODATA
     * struct. Then create the D3D11 device. The swap chain and other states
     * are created later in Scene after device creation when the render window is
     * opened.
     */
    bool enumerateDisplaysCreateD3DDevice(oapi::GraphicsClient::VIDEODATA *vdata);

    /*
     * Finds the supported AA modes and fills them in m_vecAAModes
     * This is used during default init :
     * D3D11Client::clbkCreateRenderWindow()
     *     loadConfig()
     *        setDefault()
     */
    void findSupportedAAModes();

    /*
     * Called from D3D11Client::clbkCreateRenderWindow() and
     * VideoTab::InitConfigWindow()
     */
    bool loadConfig();
    void setDefault();
    void updateConfig();
    void saveConfig();
    void applyConfig();
    void dumpConfig() const;

    // Called from VideoTab::initConfigWindow()
    void copyFromFileConfig(D3D11FileConfig &fileConfig)
    {
        memcpy( &fileConfig, &m_FileConfig, sizeof(D3D11FileConfig) );
    }

    // Called from VideoTab::initConfigWindow()
    void copyToFileConfig(D3D11FileConfig &fileConfig)
    {
        memcpy( &m_FileConfig, &fileConfig, sizeof(D3D11FileConfig) );
    }


    /*
     * ---------- Accessors for ID3D11DeviceContextPtr m_d3dImmContext ---------------
     * The impl of these accessors are here because moving them below the
     * class declaration causes linker errors. We need these to be inline.
     */
    void ClearRenderTargetView(
        ID3D11RenderTargetViewPtr &pRenderTargetView,
        const FLOAT ColorRGBA[4])
    {
        m_d3dImmContext->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
    }

    void ClearDepthStencilView(
        ID3D11DepthStencilViewPtr &pDepthStencilView,
        UINT ClearFlags,
        FLOAT Depth,
        UINT8 Stencil)
    {
        m_d3dImmContext->ClearDepthStencilView(pDepthStencilView,
                                               ClearFlags,
                                               Depth,
                                               Stencil);
    }

    // Calls OMSetDepthStencilState() with m_DSSNoDepthNoStencil
    void OMSetDepthStencilStateNoDepthNoStencil(UINT StencilRef)
    {
       m_d3dImmContext->OMSetDepthStencilState(m_DSSNoDepthNoStencil,
                                               StencilRef);
    }

    void RSSetViewports(
        UINT NumViewports,
        const D3D11_VIEWPORT *pViewports)
    {
        m_d3dImmContext->RSSetViewports(NumViewports,
                                        pViewports);
    }

    /*
     * There is a HUGE gotcha!! with these _com_ptr_t based smart pointers
     *
     * http://stackoverflow.com/questions/21005223/how-to-get-the-address-of-the-pointer-in-a-com-ptr-t
     *
     * If the & operator is applied on them it will release the underlying COM
     * Interface object and set the pointer to it, that it holds, to NULL.
     * This is done as the more common case for taking the address of a _com_ptr_t
     * is to pass it to another function which will set it. Thus the old interface
     * that the _com_ptr_t holds, needs to be released. Else there would be a
     * COM leak. But in the case of:
     *
     * void OMSetRenderTargets(
     *    [in]  UINT NumViews,
     *    [in]  ID3D11RenderTargetView *const *ppRenderTargetViews,
     *    [in]  ID3D11DepthStencilView *pDepthStencilView
     *  );
     *
     *  The ID3D11RenderTargetView** is passed it as an [in] address and
     *  not something OMSetRenderTargets will fill. So we need to get the
     *  address of the underlying Interface somehow and thus we must use
     *  GetInterfacePtr() on the _com_ptr_t. Or we will get a NULL interface
     *  crash. Also we need to make sure we pass the _com_ptr_t BY REFERENCE
     *
     *  In fact this has to be applied on any [in] Interface** parameters.
     */
    void OMSetRenderTargets(
        UINT NumViews,
        ID3D11RenderTargetViewPtr &pRenderTargetViews,
        ID3D11DepthStencilViewPtr &pDepthStencilView)
    {
        m_d3dImmContext->OMSetRenderTargets(1,
                                            &pRenderTargetViews.GetInterfacePtr(),
                                            pDepthStencilView );
    }



    /*
     * -------------- Accessors for ID3D11DevicePtr m_d3dDevice ------------------
     */

    /*
     * Here we do take the address of ID3D11RenderTargetViewPtr directly as
     *  its passed as a [out] parameter
     */
    HRESULT CreateRenderTargetView(
        ID3D11Texture2DPtr& pResource,
        const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D11RenderTargetViewPtr &pRTView)
    {
        return m_d3dDevice->CreateRenderTargetView(pResource,
                                                   pDesc,
                                                   &pRTView);
    }

    /*
     * Here we do take the address of ID3D11Texture2DPtr directly as
     * its passed as a [out] parameter. Also we made sure pass the
     * by reference
     */
    HRESULT CreateTexture2D(
        const D3D11_TEXTURE2D_DESC *pDesc,
        const D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Texture2DPtr &pTexture2D)
    {
        return m_d3dDevice->CreateTexture2D(pDesc,
                                            pInitialData,
                                            &pTexture2D);
    }

    HRESULT CreateShaderResourceView(
        ID3D11Texture2DPtr &pResource,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D11ShaderResourceViewPtr &pSRView)
    {
        return m_d3dDevice->CreateShaderResourceView(pResource,
                                                     pDesc,
                                                     &pSRView);
    }

    HRESULT CreateDepthStencilView(
        ID3D11Texture2DPtr &pResource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D11DepthStencilViewPtr &pDepthStencilView)
    {
        return m_d3dDevice->CreateDepthStencilView(pResource,
                                                   pDesc,
                                                   &pDepthStencilView);
    }



    /*
     * -------------- Accessors for IDXGISwapChainPtr m_dxgiSwapChain -----------------
     */
    // The device and swapchain are already here, no need to pass.
    HRESULT CreateSwapChain(DXGI_SWAP_CHAIN_DESC *pDesc)
    {
        return m_dxgiFactory->CreateSwapChain(m_d3dDevice,
                                              pDesc,
                                              &m_dxgiSwapChain);
    }

    HRESULT GetBuffer(
            UINT Buffer,
            REFIID riid,
            ID3D11Texture2DPtr &pSurface)
        {
            return m_dxgiSwapChain->GetBuffer(Buffer,
                                  riid,
                                  reinterpret_cast<void**>(&pSurface.GetInterfacePtr()) );
        }

    void GetFullscreenState(BOOL &isFullscreen)
    {
        m_dxgiSwapChain->GetFullscreenState(&isFullscreen, nullptr);
    }
    // Called by D3D11Client::clbkPreOpenPopup()
    void SetFullscreenState(void)
    {
        m_dxgiSwapChain->SetFullscreenState(FALSE, nullptr);
    }

	void swapBuffers(oapi::GraphicsClient::VIDEODATA *pVideoData)
    {
        m_dxgiSwapChain->Present( pVideoData->novsync ? 0 : 1, 0  );
    }



    /*
     *  Public config variables accessed in many other places,
     *  technically members but no point in appending m_* to all
     */

    // Used by VideoTab::SelectOutput()
    std::vector<IDXGIOutputPtr> vecDXGIOutputs;
    IDXGIOutputPtr dxgiOutput;
    // Used by VideoTab::initVideoTab to init a UI control, so do not use
    // std::string. Each char* is dyn alloc in enumerateDisplaysCreateD3DDevice
    // so they must each be deallocated in the dtor
    std::vector<char*> vecDXGIAdapterOutputDesc;

    // Accessed from VideoTab::selectOutput()
    IDXGIAdapter1Ptr  dxgiAdapter;
    std::vector<IDXGIAdapter1Ptr> vecDXGIAdapters;

    // Used in Scene::init3D()
    DXGI_MODE_DESC dxgiMode;
    DXGI_SAMPLE_DESC dxgiCurrentAAMode;
    std::vector<AA_MODE_DESC> vecAAModes;

    D3D_DRIVER_TYPE driverType;
    D3D_FEATURE_LEVEL succeededFeatureLevel;

    DXGI_FORMAT dxgiRTFormat;

    /*
     *  The following config settings are extra settings not present in the .cfg
     *  file represented by D3D11FileConfig. They are set by this D3D11Config object.
     */
    int iEnablePPEffects;

    DWORD
        dwWidth,         // Window full / fullscreen
        dwHeight,        // Window full / fullscreen
        clientWidth,     // Window active/client area
        clientHeight,    // Window active/client area
        dwSketchPadMode, // TODO: there is only 1 mode ?
        dwTextureMipMapCount,  // See applyConfig()
        dwTileLoadingFrequency; // Unused currently

    bool
        bFullScreenWindow,        //full-screen mode
        bNormalMaps,              //use normal maps
        bBumpMaps,                //use bump maps
        bSpecularMaps,            //use specular maps
        bEmissiveMaps,            //use emissive maps
        bPreloadTiles;

    float
        fAspect,
        fShadowAlpha,
        fBumpAmplitude,
        fMFDTransparency;         // TODO: find out where

    D3D11_FILTER
        FILTER_MeshTexture,        //vessel texture filtration
        FILTER_MeshNormalMap,
        FILTER_MeshSpecularMap,
        FILTER_MeshEmissiveMap,
        FILTER_PlanetTexture;

    DWORD
        AF_FACTOR_MeshTexture,
        AF_FACTOR_MeshNormalMap,
        AF_FACTOR_MeshSpecularMap,
        AF_FACTOR_MeshEmissiveMap,
        AF_FACTOR_PlanetTexture;

private:
    D3D11_FILTER setFilter( int value, DWORD &afactor );

    D3D11FileConfig m_FileConfig;
    const D3D11Client &m_GraphicsClient;

    /*
     * D3D11 setup - the device is created in D3D11Config::enumerateDisplaysCreateD3DDevice()
     * But the swap chain and other D3D11 initialization is done only in Scene which
     * calls back functions here. The swap chain is needed to render a 3D scene so can be
     * inited later. Its better to keep changes to important DirectX objects in 1 place -
     * HERE, so the create function for the swap chain is impl here but called from Scene.
     */

    // The *Ptr types are smart pointer types based on _com_ptr_t
    // This is the chosen dxgiFactory in enumerateDisplaysCreateD3DDevice()
    IDXGIFactory1Ptr m_dxgiFactory;


    ID3D11DevicePtr m_d3dDevice;           //device
    ID3D11DeviceContextPtr m_d3dImmContext;   //immediate context
    IDXGISwapChainPtr m_dxgiSwapChain;

    // D3D11 Debugging - if used, this needs ReleaseCOM too
    ID3D11DebugPtr m_d3dDebug;

    ID3D11SamplerStatePtr
        m_SSPointWrap,
        m_SSLinearWrap,
        m_SSLinearClamp;

    ID3D11BufferPtr
        m_cbD3DXMATRIXx1,
        m_cbD3DXMATRIXx2,
        m_cbD3DXVECTOR4;

    ID3D11RasterizerStatePtr
        m_RSCullBackSolid,
        m_RSCullBackWire,
        m_RSCullFrontSolid,
        m_RSCullFront_Wire,
        m_RSCullNoneSolid,
        m_RSCullNoneWire;

    ID3D11BlendStatePtr
        m_BSSrcAlpha,
        m_BSInvSrcAlpha,
        m_BSNoBlend;

    ID3D11DepthStencilStatePtr
        m_DSSNoDepthNoStencil,
        m_DSSTestDepthOnlyNoStencil;

    ID3D11Texture2DPtr m_d3dTex2x2;
    ID3D11RenderTargetViewPtr  m_d3dRTVTex2x2;

    UINT m_iShaderCompileFlag;

    // TODO: Check where used, and DO NOT TRY to init these in ctor init list
    // Will lead to an ASSERT error in VC++ xtring include file
    std::string
        m_VertexShaderVersion,
        m_GeometryShaderVersion,
        m_PixelShaderVersion;
};


