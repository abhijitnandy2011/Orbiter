#define STRICT
#define ORBITER_MODULE

// Local
#include "D3D11Client.h"
#include "VideoTab.h"
#include "Globals.h"
#include "Scene.h"
//#include "Version.h"

HINSTANCE hDLL;

// Single global client object
D3D11Client *g_GraphicsClient = NULL;

// Created here and also used in VideoTab.h
// TODO: We could have made this a member instead of keeping this global
VideoTab *g_VideoTab = NULL;

#define APP_TITLE "Test - built on "__DATE__

//#pragma region Initialization
D3D11Client::D3D11Client( HINSTANCE hIn )
    : oapi::GraphicsClient(hIn)
    , m_Config(*this)
    , m_pScene(0)
    , m_hWindow(0)
    , m_bDisplayHUD(true)
    , m_bVideoTab(false)
    , m_bEnumerate(false)
    , m_bPopup(false)
    , m_bModeChanged(false)
{
    oapiWriteLogV("D3D11Client::D3D11Client");
		
	// PerfCounter not used, we will try to use a member obj later
    //PCounter = new PerformanceCounter();
   // m_Config.setGraphicsClient(*this);

}

D3D11Client::~D3D11Client()
{
    oapiWriteLogV("D3D11Client::~D3D11Client");

    // The videotab may not exist if it was not clicked in the launchpad
    // or the user launched a scenario directly
    if (m_bVideoTab) {
        delete g_VideoTab;
    }

    // The Scene wont exist if the user never launched a scenario.
    if (m_pScene) {
        delete m_pScene;
    }
}

bool
D3D11Client::clbkInitialise()
{
    oapiWriteLogV("D3D11Client::clbkInitialise");
    if( !oapi::GraphicsClient::clbkInitialise() )        return false;
    auto vdata = g_GraphicsClient->GetVideoData();
    return true;
}

bool
D3D11Client::clbkUseLaunchpadVideoTab() const
{
    oapiWriteLogV("D3D11Client::clbkUseLaunchpadVideoTab");
    return true;
}

HWND
D3D11Client::clbkCreateRenderWindow()
{
    oapiWriteLogV("D3D11Client::clbkCreateRenderWindow");
    WLOG2( APP_TITLE" is starting up..." );

    // Must callback to base
    m_hWindow = oapi::GraphicsClient::clbkCreateRenderWindow();
    /*auto currStyle = GetWindowLong(m_hWindow, GWL_STYLE);
    auto currExStyle = GetWindowLong(m_hWindow, GWL_EXSTYLE);
    auto style = WS_POPUP;
    auto exStyle = WS_EX_TOPMOST;*/
    SetWindowTextA( m_hWindow, "- "APP_TITLE );

    // Get Windows display settings
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &m_CurrentMode);

    // Get the user preferences for video in Orbiter Launchpad
    auto data = GetVideoData();
    if (!data->fullscreen) {
        // Fullscreen not requested by Orbinaut, so we store whatever dimensions the current
        // window has.
        WINDOWINFO winfo;
        winfo.cbSize = sizeof( WINDOWINFO );
        GetWindowInfo( m_hWindow, &winfo );
        m_Config.clientWidth = abs( winfo.rcClient.right - winfo.rcClient.left );
        m_Config.clientHeight = abs( winfo.rcClient.bottom - winfo.rcClient.top );
    }
    else {
        // Fullscreen requested, apparently we do not do a true FullScreen
        // The window is simply enlarged according to the dimensions in 'data' to
        // cover the entire screen but is still movable. Hence the MoveWindow()

        // First a style setting, window should stay on top
        auto currExStyle = GetWindowLong(m_hWindow, GWL_EXSTYLE);
        currExStyle |= WS_EX_TOPMOST;
        SetWindowLong(m_hWindow, GWL_EXSTYLE, currExStyle);

        // Get window 'fullscreen' dims from data, here the dims in 'data' will
        // be the user set dims(Check). We retrieve the user preferred dims and
        // impose it on the window.
        m_Config.clientWidth = data->winw;
        m_Config.clientHeight = data->winh;
        if (m_CurrentMode.dmPelsWidth != m_Config.clientWidth ||
                m_CurrentMode.dmPelsHeight != m_Config.clientHeight) {

            DEVMODE mode;
            CopyMemory(&mode, &m_CurrentMode, sizeof(DEVMODE));
            mode.dmBitsPerPel = 32;
            mode.dmPelsWidth = m_Config.clientWidth;
            mode.dmPelsHeight = m_Config.clientHeight;
            mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            ChangeDisplaySettings(&mode, 0);
            m_bModeChanged = true;
        }
        MoveWindow(m_hWindow, 0, 0, m_Config.clientWidth, m_Config.clientHeight, true);
    }
	m_Config.fAspect = (float)m_Config.clientWidth / (float)m_Config.clientHeight;

	// Enumerate devices if not done already
    if( !m_bEnumerate ) {
    //    Device enumeration always fails when placed before clbkCreateRenderWindow).
        m_Config.enumerateDisplaysCreateD3DDevice( GetVideoData() );
        m_Config.loadConfig();
        m_bEnumerate = true;
    }

    // The config was changed above, some dependent properties may need updating
    m_Config.applyConfig();

    // Create the Scene & initialize it
    m_pScene = new Rendering::Scene(*this);
    m_pScene->init3D();
	
	// TODO: See how this works !
    progressString("Scene Allocated", 2);   

    return m_hWindow;
}


void
D3D11Client::clbkUpdate( bool running )
{
    oapiWriteLogV("D3D11Client::clbkUpdate");

    // TODO: Check if this is called before clbkRenderScene
    m_pScene->saveRenderParams();
    m_pScene->update();
}

void
D3D11Client::clbkRenderScene()
{
    oapiWriteLogV("D3D11Client::clbkRenderScene");

    // These apparently need to be set just the first time thru this function
    static const LONG WStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    static DWORD nPopup, j;
    static const HWND *PopupList;
    auto pVideoData = GetVideoData();

    // Scene rendering
    // TODO

    // Swap buffers, should this not be in config ?
    m_Config.swapBuffers(pVideoData);
}

bool
D3D11Client::clbkDisplayFrame()
{
    oapiWriteLogV("D3D11Client::clbkDisplayFrame");
    return true;
}

void D3D11Client::clbkRender2DPanel( SURFHANDLE *srf, MESHHANDLE hMesh, MATRIX3 *T, bool transparent )
{
    oapiWriteLogV("D3D11Client::clbkRender2DPanel");
}

void D3D11Client::ToggleHUD()
{
    oapiWriteLogV("D3D11Client::ToggleHUD");
    m_bDisplayHUD = !m_bDisplayHUD;
}

// Handle mouse messages, keyboard etc here
LRESULT D3D11Client::RenderWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    oapiWriteLogV("D3D11Client::RenderWndProc");
    if( msg == WM_KEYDOWN ) {
        if (wp == VK_HOME && GetKeyState(VK_CONTROL)) {
            g_GraphicsClient->ToggleHUD();
        }
    }

    return oapi::GraphicsClient::RenderWndProc (hwnd, msg, wp, lp);
}

void D3D11Client::clbkPostCreation()
{
    WLOG2( "D3D11Client::clbkPostCreation" );
}

/*
 * TODO: Check ! So we need to set full screen state using the swapchain
 * before opening a popup ? Why so ?
 */
void D3D11Client::clbkPreOpenPopup()
{
    oapiWriteLogV("D3D11Client::clbkPreOpenPopup");
    m_bPopup = true;
    auto data = GetVideoData();
    if (data->fullscreen) {
		// Uses the D3D11 swapchain to set the fullscreen state
		//m_Scene->setFullScreenState();
    }
}

void D3D11Client::clbkGetViewportSize( DWORD *w, DWORD *h ) const
{
    WLOG2( "D3D11Client::clbkGetViewportSize" );
    *w = m_Config.clientWidth;
    *h = m_Config.clientHeight;
}

bool D3D11Client::clbkFullscreenMode() const
{
    WLOG2( "D3D11Client::clbkFullscreenMode" );
    return m_Config.bFullScreenWindow;
}

bool D3D11Client::clbkGetRenderParam( DWORD param, DWORD *value ) const
{
    oapiWriteLogV("D3D11Client::clbkGetRenderParam");
    switch( param ) {
        case RP_COLOURDEPTH:
            *value = 32;
            return true;
        case RP_ZBUFFERDEPTH:
            *value = 24;
            return true;
        case RP_STENCILDEPTH:
            *value = 8;
            return true;
        case RP_MAXLIGHTS:
            *value = 12;
            return true;
        case RP_ISTLDEVICE:
            *value = TRUE;
            return true;
        case RP_REQUIRETEXPOW2:
            *value = FALSE;
            return true;
    }
    return false;
}

//LAUNCHPAD.
BOOL D3D11Client::LaunchpadVideoWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    oapiWriteLogV("D3D11Client::LaunchpadVideoWndProc");
    if( !m_bVideoTab ) {
        if( !m_bEnumerate ) {
            m_Config.enumerateDisplaysCreateD3DDevice( GetVideoData() );
            m_Config.loadConfig();
            m_Config.applyConfig();
            m_bEnumerate = true;
        }
        m_bVideoTab = true;
        g_VideoTab = new VideoTab( *this, oapi::GraphicsClient::LaunchpadVideoTab(), hDLL );
        g_VideoTab->initTextBoxes();
    }
    if (g_VideoTab)
    {
        return g_VideoTab->launchpadVideoWndProc( hwnd, msg, wp, lp );
    }
    else
    {
        return oapi::GraphicsClient::LaunchpadVideoWndProc(hwnd, msg, wp, lp);
    }
}

void D3D11Client::clbkRefreshVideoData()
{
	oapiWriteLogV("D3D11Client::clbkRefreshVideoData");
    if (g_VideoTab) {
	    g_VideoTab->updateConfig();
    }
}

void D3D11Client::clbkCloseSession( bool fastclose )
{
    WLOG2( "D3D11Client::clbkCloseSession" );
    oapi::GraphicsClient::clbkCloseSession( fastclose );
}

void D3D11Client::clbkDestroyRenderWindow( bool fastclose )
{
    WLOG2( "D3D11Client::clbkDestroyRenderWindow" );
//terminate rendering thread.
    if (m_bModeChanged)
    {
        ChangeDisplaySettings(&m_CurrentMode, 0);
    }

	oapi::GraphicsClient::clbkDestroyRenderWindow( fastclose );
}

#pragma region Mesh and Object Functions
//vessels.
int D3D11Client::clbkVisEvent( OBJHANDLE hObj, VISHANDLE vis, DWORD msg, UINT context )
{
    oapiWriteLogV("D3D11Client::clbkVisEvent");
    return 0;
}

void D3D11Client::clbkNewVessel( OBJHANDLE obj ) {
    oapiWriteLogV("D3D11Client::clbkNewVessel");
}

void D3D11Client::clbkDeleteVessel( OBJHANDLE hVessel ) {
    oapiWriteLogV("D3D11Client::clbkDeleteVessel");
}

MESHHANDLE D3D11Client::clbkGetMesh( VISHANDLE vis, UINT idx ) {
    oapiWriteLogV("D3D11Client::clbkGetMesh");
    return NULL;
}

int D3D11Client::clbkEditMeshGroup( DEVMESHHANDLE hMesh, DWORD grpidx, GROUPEDITSPEC *ges ) {
    oapiWriteLogV("D3D11Client::clbkEditMeshGroup");
    return 0;
}

bool D3D11Client::clbkSetMeshTexture( DEVMESHHANDLE hMesh, DWORD texidx, SURFHANDLE tex ) {
    oapiWriteLogV("D3D11Client::clbkSetMeshTexture");
	return true;
}

int D3D11Client::clbkSetMeshMaterial( DEVMESHHANDLE hMesh, DWORD matidx, const MATERIAL *mat ) {
    oapiWriteLogV("D3D11Client::clbkSetMeshMaterial");
    return 0;
}

bool D3D11Client::clbkSetMeshProperty( DEVMESHHANDLE hMesh, DWORD prop, DWORD value ) {
    oapiWriteLogV("D3D11Client::clbkSetMeshProperty");
    return 0;
}

void D3D11Client::clbkStoreMeshPersistent( MESHHANDLE hMesh, const char *fname ) {
    //no need to sync.
    oapiWriteLogV("D3D11Client::clbkStoreMeshPersistent");
    oapiWriteLogV("MESH loaded from file : %s", fname);
}
#pragma endregion
#pragma region TextureFunctions
SURFHANDLE D3D11Client::clbkLoadTexture( const char *fname, DWORD flags ) {
    oapiWriteLogV("D3D11Client::clbkLoadTexture");
    return NULL;
}

SURFHANDLE D3D11Client::clbkLoadSurface (const char *fname, DWORD attrib)
{
    oapiWriteLogV("D3D11Client::clbkLoadSurface");
    return NULL;
}

SURFHANDLE D3D11Client::clbkCreateTexture( DWORD w, DWORD h ) {
    oapiWriteLogV("D3D11Client::clbkCreateTexture");
    return NULL;
}

void D3D11Client::clbkReleaseTexture( SURFHANDLE tex ) {
    oapiWriteLogV("D3D11Client::clbkReleaseTexture");
}

HDC D3D11Client::clbkGetSurfaceDC( SURFHANDLE surf ) {
    oapiWriteLogV("D3D11Client::clbkGetSurfaceDC");
    return NULL;
}

void D3D11Client::clbkReleaseSurfaceDC( SURFHANDLE surf, HDC hdc ) {
    oapiWriteLogV("D3D11Client::clbkReleaseSurfaceDC");
}

bool D3D11Client::clbkReleaseSurface( SURFHANDLE surf ) {
    oapiWriteLogV("D3D11Client::clbkReleaseSurface");
    return true;
}

SURFHANDLE D3D11Client::clbkCreateSurface( DWORD w, DWORD h, SURFHANDLE hTempl ) {
    oapiWriteLogV("D3D11Client::clbkCreateSurface");
    return NULL;
}

SURFHANDLE D3D11Client::clbkCreateSurfaceEx (DWORD w, DWORD h, DWORD attrib)
{
    oapiWriteLogV("D3D11Client::clbkCreateSurfaceEx");
    return NULL;
}

SURFHANDLE D3D11Client::clbkCreateSurface( HBITMAP hbm ) {
    oapiWriteLogV("D3D11Client::clbkCreateSurface");
    return NULL;
}

bool D3D11Client::clbkGetSurfaceSize( SURFHANDLE surf, DWORD *w, DWORD *h ) {
    oapiWriteLogV("D3D11Client::clbkGetSurfaceSize");
    return true;
}

bool D3D11Client::clbkCopyBitmap( SURFHANDLE pdds, HBITMAP hbm, int x, int y, int dx, int dy ) {
    oapiWriteLogV("D3D11Client::clbkCopyBitmap");
    return true;
}

bool D3D11Client::clbkBlt( SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD flags ) const {
    oapiWriteLogV("D3D11Client::clbkBlt 1");
    return true;
}

bool D3D11Client::clbkBlt( SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD w, DWORD h, DWORD flags ) const {
    oapiWriteLogV("D3D11Client::clbkBlt 2");
    return true;
}

bool D3D11Client::clbkScaleBlt( SURFHANDLE tgt, DWORD tgtx, DWORD tgty, DWORD tgtw, DWORD tgth, SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD srcw, DWORD srch, DWORD flags ) const {
    oapiWriteLogV("D3D11Client::clbkScaleBlt");
    return true;
}

bool D3D11Client::clbkFillSurface(
    SURFHANDLE surf,
    DWORD col ) const
{
    oapiWriteLogV("D3D11Client::clbkFillSurface 1");
    return true;
}

bool D3D11Client::clbkFillSurface(
    SURFHANDLE surf,
    DWORD tgtx,
    DWORD tgty,
    DWORD w,
    DWORD h,
    DWORD col ) const
{
    oapiWriteLogV("D3D11Client::clbkFillSurface 2");
    return true;
}

DWORD D3D11Client::clbkGetDeviceColour( BYTE r, BYTE g, BYTE b )
{
    oapiWriteLogV("D3D11Client::clbkGetDeviceColour");
    DWORD value = r;
    value <<= 8;
    value |= g;
    value <<= 8;
    value |= b;
    return value;
}

bool D3D11Client::clbkSetSurfaceColourKey( SURFHANDLE surf, DWORD ckey )
{
    oapiWriteLogV("D3D11Client::clbkSetSurfaceColourKey");
    if( surf )
        return true;
    return false;
}

void D3D11Client::clbkIncrSurfaceRef( SURFHANDLE surf )
{
    oapiWriteLogV("D3D11Client::clbkIncrSurfaceRef");
}
#pragma endregion surface and texture functions
#pragma region Sketchpad/Fonts/Brushes/Pens
//GDI.
oapi::Sketchpad *D3D11Client::clbkGetSketchpad( SURFHANDLE surf )
{
    // Creates one of 2 possible versions of 2D Sketchpad
    oapiWriteLogV("D3D11Client::clbkGetSketchpad");
    // TODO
    return NULL;
}

void D3D11Client::clbkReleaseSketchpad( oapi::Sketchpad *Skpad )
{
    oapiWriteLogV("D3D11Client::clbkReleaseSketchpad");
    if( !Skpad )
        return;

    // Test code
    return;
}

oapi::Font *D3D11Client::clbkCreateFont(
    int height,
    bool prop,
    const char *face,
    oapi::Font::Style style,
    int orientation ) const
{
    oapiWriteLogV("D3D11Client::clbkCreateFont");
    return NULL;
}

void D3D11Client::clbkReleaseFont( oapi::Font *font ) const
{
    oapiWriteLogV("D3D11Client::clbkReleaseFont");
}

oapi::Pen *D3D11Client::clbkCreatePen( int style, int width, DWORD col ) const
{
    oapiWriteLogV("D3D11Client::clbkCreatePen");
    return NULL;
}

void D3D11Client::clbkReleasePen( oapi::Pen *pen ) const
{
    oapiWriteLogV("D3D11Client::clbkReleasePen");
}

oapi::Brush *D3D11Client::clbkCreateBrush( DWORD col ) const
{
    oapiWriteLogV("D3D11Client::clbkCreateBrush");
    return NULL;
}

void D3D11Client::clbkReleaseBrush( oapi::Brush *brush ) const
{
    oapiWriteLogV("D3D11Client::clbkReleaseBrush");
}

#pragma endregion GDI drawing functions
#pragma region PStreamFunctions
oapi::ParticleStream *D3D11Client::clbkCreateExhaustStream(
    PARTICLESTREAMSPEC *pss,
    OBJHANDLE obj,
    const double *lvl,
    const VECTOR3 *ref,
    const VECTOR3 *dir )
{
    oapiWriteLogV("D3D11Client::clbkCreateExhaustStream 1");
	return NULL;   
}

oapi::ParticleStream *D3D11Client::clbkCreateExhaustStream(
    PARTICLESTREAMSPEC *pss,
    OBJHANDLE obj,
    const double *lvl,
    const VECTOR3 &ref,
    const VECTOR3 &dir )
{
    oapiWriteLogV("D3D11Client::clbkCreateExhaustStream 2");
	return NULL;
}

oapi::ParticleStream *D3D11Client::clbkCreateReentryStream(
    PARTICLESTREAMSPEC *pss,
    OBJHANDLE obj )
{
    oapiWriteLogV("D3D11Client::clbkCreateReentryStream");
	return NULL;
}

bool D3D11Client::clbkParticleStreamExists( const oapi::ParticleStream *ps )
{
    oapiWriteLogV("D3D11Client::clbkParticleStreamExists");
    return false;
}
#pragma endregion Particle stream functions (temporarily void)

oapi::ScreenAnnotation *D3D11Client::clbkCreateAnnotation()
{
    oapiWriteLogV("D3D11Client::clbkCreateAnnotation");
    return oapi::GraphicsClient::clbkCreateAnnotation();
}

//module functions.
DLLCLBK void InitModule( HINSTANCE h )
{
    oapiWriteLogV("D3D11Client::InitModule");
    HMODULE hd3d11dll = ::LoadLibrary("d3d11.dll");
    if (!hd3d11dll)
    {
        ::MessageBox(0, "This module requires DirectX 11 to be installed.", "DirectX 11 is not installed", MB_OK | MB_ICONERROR);
        return;
    }
    ::FreeLibrary(hd3d11dll);
    hDLL = h;
    g_GraphicsClient = new D3D11Client( hDLL );
    if( !oapiRegisterGraphicsClient( g_GraphicsClient ) ) {
        delete g_GraphicsClient;
        g_GraphicsClient = NULL;
    }
}

DLLCLBK void ExitModule(HINSTANCE hDLL)
{
    oapiWriteLogV("D3D11Client::clbkScaleBlt");
    if( g_GraphicsClient ) {
        oapiUnregisterGraphicsClient( g_GraphicsClient );
        g_GraphicsClient = NULL;
    }
}

void D3D11Client::InitSplashScreen() {
}

void
D3D11Client::progressString( const char *line, DWORD color )
{

    oapiWriteLogV("ProgressString :  %s", line);

}

void
D3D11Client::ExitSplashScreen()
{

}
