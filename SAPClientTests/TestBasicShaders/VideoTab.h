/*
 * The VideoTab class.
 * TODO: Check forward decl
 *
 * So here is what the VideoTab class does. It loads the data in the
 * D3D11Client.cfg file into its own D3D11FileConfig struct. This is in
 * VideoTab::initConfig(). It uses this to initialize the controls shown to
 * the user. After the user clicks 'OK' it copies it back to the main D3D11Config
 * object. This is in  VideoTab::ConfigProc(). There is just 1 instance of
 * D3D11Config in the entire plugin and its present in the D3D11Client object.
 * Its used in may places so it MUST appear back there.
 */

#pragma once

// std
#include <vector>

// D3D11
#include <DXGI.h>

// Orbiter
#include "..\..\include\GraphicsAPI.h"

// Local
#include "D3D11Config.h"


class D3D11Client;

// Global callbacks which redirect to corresponding callbacks in VideoTab
// TODO: Check if these can be removed and the VideoTab::*Proc() functions
// directly used.
LRESULT CALLBACK gclbkConfigProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
LRESULT CALLBACK gclbkGeneralProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
LRESULT CALLBACK gclbkVesselProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
LRESULT CALLBACK gclbkPlanetsProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

/*
 * This class has the callback for the launchpad Video tab of this client:
 * VideoTab::launchpadVideoWndProc()
 *
 * The 'D3D11 Client Configuration' button position in the tab is managed
 * in the WM_WINDOWPOSCHANGED msg
 *
 * It also manages the 'D3D11 Client Configuration' dialog generated by clicking
 * the button on the launchpad Video tab. When ok is pressed in the
 * config dialog, the config is copied to the D3D11Config object(m_Config)
 * for this client(See VideoTab::ConfigProc).
 * m_Config is also used to write a new config file with the changed settings
 * TODO: Test this !
 * Since the D3D11Config object is updated(there is only 1 for the ENTIRE client)
 * the settings are available later for the client when the render window is
 * setup in Scene.
 *
 */
class VideoTab {

public:
    VideoTab(D3D11Client &pGC, HWND _hTab, HINSTANCE _hDLL );
    ~VideoTab();

    // Refresh videodata.
    void updateConfig();

    // Main window messaging callback for the tab
    BOOL launchpadVideoWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

    // Sets values in the UI, called from D3D11Client::LaunchpadVideoWndProc()
    void initTextBoxes();

    // Callbacks for the other windows created on button clicks
    LRESULT configProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    LRESULT generalProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    LRESULT vesselProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    LRESULT planetProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

    HWND
        m_hConfig,
        m_hGeneral,
        m_hVessels,
        m_hPlanets;
private:
    // Called from ctor
    void initVideoTab();

    // These just read from the ui control in the dialog & set them in
    // the D3D11Config object
    void selectWidth();
    void selectHeight();
    void selectFullscreen( bool fullscreen );
    void selectOutput();
    void selectMode();
    void selectFullScreenWindow();
    void selectSoftwareDevice();


    void createSymbolicLinks();
    bool symbolicLinksCreated();
    bool createLink(const char * linkName, const char * destination);

    // Init the individual object windows
    void initConfigWindow();
    void initGeneralWindow();
    void initVesselWindow();
    void initPlanetWindow();

    void initConfig();
    void saveConfig();

    oapi::GraphicsClient::VIDEODATA *m_VideoData;

    DXGI_MODE_DESC *m_DXGIModeDesc;
    std::vector<int> m_vecModeDescIndices;

    HWND m_hTab;
    int m_iAspect, m_iAspectWFac[3], m_iAspectHFac[3];

    HINSTANCE m_hDLL;

    RECT m_InitTabRect;

    // Graphics Client, do not make const as
    // oapiGraphicsClient::GetVideoData() is non-const
    D3D11Client &m_GraphicsClient;

    // Config, the VideoTab has its own copy of D3D11FileConfig
    // separate from but inited by, the one in D3D11Config
    // m_Config is used to modify the single D3D11Config object from
    // this class, so do not make it const
    D3D11Config &m_Config;
    D3D11FileConfig m_FileConfig;
};
