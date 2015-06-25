/*
 * VideoTab impl
 */

// Self
#include "VideoTab.h"

// Local
#include "D3D11Client.h"
#include "Globals.h"

// Declared in D3D11Client.cpp
extern VideoTab *g_VideoTab;

VideoTab::VideoTab(D3D11Client &gc, HWND hTab, HINSTANCE hDLL )
    : m_GraphicsClient(gc)
    , m_hTab(hTab)
    , m_hDLL(hDLL)
	, m_Config(m_GraphicsClient.getConfig())
{
	m_iAspect = 0;

	m_iAspectWFac[0] = 4;
	m_iAspectWFac[1] = 16;
	m_iAspectWFac[2] = 16;

	m_iAspectHFac[0] = 3;
	m_iAspectHFac[1] = 10;
	m_iAspectHFac[2] = 9;

	g_VideoTab = this;
	m_hConfig = m_hGeneral = m_hVessels = m_hPlanets = NULL;

	initVideoTab();
}

VideoTab::~VideoTab()
{
	delete [ ] m_DXGIModeDesc;
}

BOOL
VideoTab::launchpadVideoWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{

	static WINDOWPOS *wpos;
	static RECT TabRect;

	m_VideoData = m_GraphicsClient.GetVideoData();

	switch( msg ) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch( LOWORD(wp) ) {
		case VID_DEVICE://*
			if( HIWORD(wp) == CBN_SELCHANGE ) {
				selectOutput();
			}
			break;
		case VID_MODE:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				selectMode();
				return TRUE;
			}
			break;
		case VID_FULL:
			if( HIWORD(wp) == BN_CLICKED ) {
				selectFullscreen( true );
				return TRUE;
			}
			break;
		case VID_WINDOW:
			if( HIWORD(wp) == BN_CLICKED ) {
				selectFullscreen( false );
				return TRUE;
			}
			break;
		case VID_WIDTH:
			if( HIWORD(wp) == EN_CHANGE ) {
				selectWidth();
				return TRUE;
			}
			break;
		case VID_HEIGHT:
			if( HIWORD( wp ) == EN_CHANGE ) {
				selectHeight();
				return TRUE;
			}
			break;
		case VID_ASPECT:
			if( HIWORD( wp ) == BN_CLICKED ) {
				selectWidth();
				return TRUE;
			}
			break;
		case VID_PAGEFLIP://full screen window.
			if( HIWORD( wp ) == BN_CLICKED ) {
				selectFullScreenWindow();
				return TRUE;
			}
			break;
		case VID_ENUM://software device.
			if( HIWORD( wp ) == BN_CLICKED ) {
				selectSoftwareDevice();
				return TRUE;
			}
			break;
		case VID_4X3:
		case VID_16X10:
		case VID_16X9:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_iAspect = LOWORD( wp ) - VID_4X3;
				selectWidth();
				return TRUE;
			}
			break;
		case VID_VSYNC:
			if( HIWORD( wp ) == BN_CLICKED ) {
				return TRUE;
			}
			break;
		case VID_STENCIL:
			if( HIWORD( wp ) == BN_CLICKED ) {
				return TRUE;
			}
			break;
		case VID_D3D11CONFIG:
			if( HIWORD( wp ) == BN_CLICKED ) {
				if( m_hConfig )
					return TRUE;
				initConfigWindow();
				return TRUE;
			}
			break;
		}
		break;
	case WM_WINDOWPOSCHANGED:

		wpos = (WINDOWPOS*)lp;
		GetWindowRect( m_hTab, &TabRect );
		int
			tab_width = abs( TabRect.right - TabRect.left ),
			tab_height = abs( TabRect.bottom - TabRect.top );

		if( tab_width < 420 )			tab_width = 420;
		if( tab_height < 408 )			tab_height = 408;
		BOOL res = MoveWindow( GetDlgItem( m_hTab, VID_D3D11CONFIG ), 7 + (tab_width - 420)/2, 350 + (tab_height - 408)/2, 407, 50, true );
		return FALSE;
	}
	return FALSE;
}

void VideoTab::selectFullScreenWindow()
{
	if( m_Config.bFullScreenWindow ) {
		SendDlgItemMessage( m_hTab, VID_PAGEFLIP, BM_SETCHECK, 0, 0 );
		m_Config.bFullScreenWindow = m_VideoData->pageflip = false;
	}
	else {
		SendDlgItemMessage( m_hTab, VID_PAGEFLIP, BM_SETCHECK, 1, 0 );
		m_Config.bFullScreenWindow = m_VideoData->pageflip = true;
	}
}

void VideoTab::selectSoftwareDevice()
{
	static bool HardWare;

	HardWare = (m_Config.driverType == D3D_DRIVER_TYPE_UNKNOWN);
	m_VideoData->forceenum = !HardWare;
	if( HardWare ) {
		SendDlgItemMessage( m_hTab, VID_ENUM, BM_SETCHECK, 1, 0 );
		m_Config.driverType = D3D_DRIVER_TYPE_REFERENCE;
	}
	else {
		SendDlgItemMessage( m_hTab, VID_ENUM, BM_SETCHECK, 0, 0 );
		m_Config.driverType = D3D_DRIVER_TYPE_UNKNOWN;
	}
}

void VideoTab::selectWidth()
{
	if( SendDlgItemMessage( m_hTab, VID_ASPECT, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
		char cbuf[32];
		int w, h, wfac = m_iAspectWFac[m_iAspect], hfac = m_iAspectHFac[m_iAspect];
		GetWindowTextA( GetDlgItem( m_hTab, VID_WIDTH ), cbuf, 127 );
		w = atoi( cbuf );
		GetWindowTextA( GetDlgItem( m_hTab, VID_HEIGHT ), cbuf, 127 );
		h = atoi( cbuf );
		if( w != (wfac*h)/hfac ) {
			h = (hfac*w)/wfac;
			if( _itoa_s( h, cbuf, 32, 10 ) )
				return;
			SetWindowTextA( GetDlgItem( m_hTab, VID_HEIGHT ), cbuf );
			m_VideoData->winh = h;
		}
		m_Config.dwWidth = w;
		m_Config.dwHeight = h;
	}
}

void VideoTab::selectHeight()
{
	if( SendDlgItemMessage( m_hTab, VID_ASPECT, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
		char cbuf[32];
		int w, h, wfac = m_iAspectWFac[m_iAspect], hfac = m_iAspectHFac[m_iAspect];
		GetWindowTextA( GetDlgItem( m_hTab, VID_WIDTH ), cbuf, 127 );
		w = atoi( cbuf );
		GetWindowTextA( GetDlgItem( m_hTab, VID_HEIGHT ), cbuf, 127 );
		h = atoi( cbuf );
		if( h != (hfac*w)/wfac ) {
			w = (wfac*h)/hfac;
			if( _itoa_s( w, cbuf, 32, 10 ) )
				return;
			SetWindowTextA( GetDlgItem( m_hTab, VID_WIDTH ), cbuf );
			m_VideoData->winw = w;
		}
		m_Config.dwWidth = w;
		m_Config.dwHeight = h;
	}
}

void VideoTab::initTextBoxes()
{
	char cbuf[32];
	m_VideoData = m_GraphicsClient.GetVideoData();

	if( _itoa_s( m_VideoData->winw, cbuf, 32, 10 ) )		return;
	SetWindowTextA( GetDlgItem( m_hTab, VID_WIDTH ), cbuf );
//height.
	if( _itoa_s( m_VideoData->winh, cbuf, 32, 10 ) )		return;
	SetWindowTextA( GetDlgItem( m_hTab, VID_HEIGHT ), cbuf );
}

void VideoTab::selectFullscreen( bool fullscreen )
{
	m_Config.bFullScreenWindow = m_VideoData->fullscreen = fullscreen;
	if( fullscreen ) {
		SendDlgItemMessage( m_hTab, VID_FULL, BM_SETCHECK, 1, 0 );
		SendDlgItemMessage( m_hTab, VID_WINDOW, BM_SETCHECK, 0, 0 );

		EnableWindow( GetDlgItem( m_hTab, VID_ASPECT ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_WIDTH ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_HEIGHT ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_4X3 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_16X10 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_16X9 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_MODE ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_VSYNC ), true );
		//EnableWindow( GetDlgItem( m_hTab, VID_PAGEFLIP ), true );
	}
	else {
		SendDlgItemMessage( m_hTab, VID_FULL, BM_SETCHECK, 0, 0 );
		SendDlgItemMessage( m_hTab, VID_WINDOW, BM_SETCHECK, 1, 0 );

		EnableWindow( GetDlgItem( m_hTab, VID_ASPECT ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_WIDTH ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_HEIGHT ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_4X3 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_16X10 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_16X9 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_MODE ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_VSYNC ), false );
		//EnableWindow( GetDlgItem( m_hTab, VID_PAGEFLIP ), false );
	}
}

void VideoTab::selectOutput() 
{
	static char line[256];

	m_VideoData->deviceidx = SendDlgItemMessage( m_hTab, VID_DEVICE, CB_GETCURSEL, 0, 0 );
	m_Config.dxgiAdapter = m_Config.vecDXGIAdapters[m_VideoData->deviceidx];
	m_Config.dxgiOutput = m_Config.vecDXGIOutputs[m_VideoData->deviceidx];

	SendDlgItemMessage( m_hTab, VID_MODE, CB_RESETCONTENT, 0, 0 );
	delete [ ] m_DXGIModeDesc;
	UINT NumModes;
	m_Config.vecDXGIOutputs[m_VideoData->deviceidx]->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, NULL);
	m_DXGIModeDesc = new DXGI_MODE_DESC [NumModes];
	m_Config.vecDXGIOutputs[m_VideoData->deviceidx]->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, m_DXGIModeDesc);
	m_vecModeDescIndices.clear();
	for( DWORD j = 0; j < NumModes; j++ ) {
		if( m_DXGIModeDesc[j].Scaling )
			continue;
		if( m_DXGIModeDesc[j].ScanlineOrdering != 1 )
			continue;
		sprintf_s( line, "%u x %u %u Hz", m_DXGIModeDesc[j].Width, m_DXGIModeDesc[j].Height, (int)((float)m_DXGIModeDesc[j].RefreshRate.Numerator/(float)m_DXGIModeDesc[j].RefreshRate.Denominator + 0.3 ), m_DXGIModeDesc[j].Scaling, m_DXGIModeDesc[j].ScanlineOrdering );
		SendDlgItemMessageA( m_hTab, VID_MODE, CB_ADDSTRING, 0, (LPARAM)line );
		SendDlgItemMessageA( m_hTab, VID_MODE, CB_SETITEMDATA, j, (LPARAM)( m_DXGIModeDesc[j].Width<<16 | m_DXGIModeDesc[j].Height ) );
		m_vecModeDescIndices.push_back(j);
	}
	SendDlgItemMessage( m_hTab, VID_MODE, CB_SETCURSEL, 0, 0 );
}

void VideoTab::selectMode()
{
	auto idx = SendDlgItemMessage( m_hTab, VID_MODE, CB_GETCURSEL, 0, 0 );
	auto modeIndex = m_vecModeDescIndices[idx];
	m_VideoData->modeidx = modeIndex;
	m_Config.dwWidth = m_DXGIModeDesc[m_VideoData->modeidx].Width;
	m_Config.dwHeight = m_DXGIModeDesc[m_VideoData->modeidx].Height;
}

void VideoTab::updateConfig()
{
	char cbuf[32];
	m_VideoData = m_GraphicsClient.GetVideoData();

	m_VideoData->deviceidx  = SendDlgItemMessage( m_hTab, VID_DEVICE, CB_GETCURSEL, 0, 0 );
	auto idx = SendDlgItemMessage( m_hTab, VID_MODE, CB_GETCURSEL, 0, 0 );
	m_VideoData->modeidx = m_vecModeDescIndices[idx];
	//m_VideoData->modeidx    = SendDlgItemMessage( m_hTab, VID_MODE, CB_GETCURSEL, 0, 0 );
	m_VideoData->fullscreen = ( SendDlgItemMessage( m_hTab, VID_FULL, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
	m_VideoData->novsync    = ( SendDlgItemMessage( m_hTab, VID_VSYNC, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
	m_VideoData->pageflip   = ( SendDlgItemMessage( m_hTab, VID_PAGEFLIP, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
	m_VideoData->trystencil = ( SendDlgItemMessage( m_hTab, VID_STENCIL, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
	m_VideoData->forceenum  = ( SendDlgItemMessage( m_hTab, VID_ENUM, BM_GETCHECK, 0, 0 ) == BST_CHECKED );

	if (!m_VideoData->fullscreen)
	{
		GetWindowTextA(GetDlgItem(m_hTab, VID_WIDTH),  cbuf, 32);
		m_VideoData->winw = atoi(cbuf);
		GetWindowTextA(GetDlgItem(m_hTab, VID_HEIGHT), cbuf, 32);
		m_VideoData->winh = atoi(cbuf);
	}
	else
	{
		m_VideoData->winw = m_DXGIModeDesc[m_VideoData->modeidx].Width;
		m_VideoData->winh = m_DXGIModeDesc[m_VideoData->modeidx].Height;
	}
}

void VideoTab::initVideoTab()
{
	char line[256];
	DWORD aCounter = 0, oCounter = 0;
		
	m_VideoData = g_GraphicsClient->GetVideoData();
	SendDlgItemMessage( m_hTab, VID_DEVICE, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage( m_hTab, VID_MODE, CB_RESETCONTENT, 0, 0 );

	for (auto outputDesc : m_Config.vecDXGIAdapterOutputDesc) {
		SendDlgItemMessageA(m_hTab, VID_DEVICE, CB_ADDSTRING, 0, (LPARAM)outputDesc);
	}
	SendDlgItemMessageA( m_hTab, VID_DEVICE, CB_SETCURSEL, m_VideoData->deviceidx, 0 );
//display modes.
	UINT NumModes;
	m_Config.vecDXGIOutputs[m_VideoData->deviceidx]->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, NULL);
	m_DXGIModeDesc = new DXGI_MODE_DESC [NumModes];
	m_Config.vecDXGIOutputs[m_VideoData->deviceidx]->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &NumModes, m_DXGIModeDesc);
	m_vecModeDescIndices.clear();
	int currentModeIndex = 0;
	for( DWORD j = 0; j < NumModes; j++ ) {
		if( m_DXGIModeDesc[j].Scaling )
			continue;
		if( m_DXGIModeDesc[j].ScanlineOrdering != 1 )
			continue;
		sprintf_s( line, "%u x %u %u Hz", m_DXGIModeDesc[j].Width, m_DXGIModeDesc[j].Height, (int)((float)m_DXGIModeDesc[j].RefreshRate.Numerator/(float)m_DXGIModeDesc[j].RefreshRate.Denominator + 0.3 ), m_DXGIModeDesc[j].Scaling, m_DXGIModeDesc[j].ScanlineOrdering );
		SendDlgItemMessageA( m_hTab, VID_MODE, CB_ADDSTRING, 0, (LPARAM)line );
		SendDlgItemMessageA( m_hTab, VID_MODE, CB_SETITEMDATA, j, (LPARAM)( m_DXGIModeDesc[j].Width<<16 | m_DXGIModeDesc[j].Height ) );
		if (m_VideoData->modeidx == j)
		{
			currentModeIndex = m_vecModeDescIndices.size();
		}
		m_vecModeDescIndices.push_back(j);
	}
	SendDlgItemMessage( m_hTab, VID_MODE, CB_SETCURSEL, (WPARAM)currentModeIndex, 0 );
//software device.
	/*SendDlgItemMessageA( m_hTab, VID_ENUM, CB_RESETCONTENT, 0, 0 );
	SetWindowTextA( GetDlgItem( m_hTab, VID_ENUM ), "" );
	SendDlgItemMessageA( m_hTab, VID_ENUM, BM_SETCHECK, m_VideoData->forceenum, 0 );*/
	ShowWindow(GetDlgItem( m_hTab, VID_ENUM ), SW_HIDE);
//aspect.
	if( m_VideoData->winw == (4*m_VideoData->winh)/3 || m_VideoData->winh == (3*m_VideoData->winw)/4 )
		m_iAspect = 1;
	if( m_VideoData->winw == (16*m_VideoData->winh)/10 || m_VideoData->winh == (10*m_VideoData->winw)/16 )
		m_iAspect = 2;
	if( m_VideoData->winw == (16*m_VideoData->winh)/9 || m_VideoData->winh == (9*m_VideoData->winw)/16 )
		m_iAspect = 3;

	SendDlgItemMessageA( m_hTab, VID_ASPECT, BM_SETCHECK, m_iAspect ? 1 : 0, 0 );
	if( m_iAspect )
		m_iAspect--;
	SendDlgItemMessageA( m_hTab, VID_4X3+m_iAspect, BM_SETCHECK, 1, 0 );
//
	SendDlgItemMessageA( m_hTab, VID_STENCIL, BM_SETCHECK, 0, 0 );
	SendDlgItemMessageA( m_hTab, VID_VSYNC, BM_SETCHECK, 0, 0 );
//pass parameters to D3D11Config class. [TODO]
	/*SendDlgItemMessageA( m_hTab, VID_VSYNC, CB_RESETCONTENT, 0, 0 );
	SetWindowTextA( GetDlgItem( m_hTab, VID_VSYNC ), "" );*/
	SendDlgItemMessageA( m_hTab, VID_VSYNC, BM_SETCHECK, m_VideoData->novsync ? TRUE : FALSE, 0 );
//full screen window.
	/*SendDlgItemMessageA( m_hTab, VID_PAGEFLIP, CB_RESETCONTENT, 0, 0 );
	SetWindowTextA( GetDlgItem( m_hTab, VID_PAGEFLIP ), "" );
	SendDlgItemMessageA( m_hTab, VID_PAGEFLIP, BM_SETCHECK, m_VideoData->pageflip, 0 );*/
	ShowWindow(GetDlgItem( m_hTab, VID_PAGEFLIP ), SW_HIDE);
//stencil.
	/*SendDlgItemMessageA( m_hTab, VID_STENCIL, CB_RESETCONTENT, 0, 0 );
	SetWindowTextA( GetDlgItem( m_hTab, VID_STENCIL ), "" );
	SendDlgItemMessageA( m_hTab, VID_STENCIL, BM_SETCHECK, m_VideoData->trystencil, 0 );*/
	ShowWindow(GetDlgItem( m_hTab, VID_STENCIL ), SW_HIDE);
//sets 32bit into BPP combobox.
	EnableWindow( GetDlgItem( m_hTab, VID_BPP ), false );
	SendDlgItemMessageA( m_hTab, VID_BPP, CB_RESETCONTENT, 0, 0 );
	SendDlgItemMessageA( m_hTab, VID_BPP, CB_ADDSTRING, 0, (LPARAM)"32" );
	SendDlgItemMessageA( m_hTab, VID_BPP, CB_SETCURSEL, 0, 0 );

	//EnableWindow( GetDlgItem( m_hTab, VID_FULL ), false );
	EnableWindow( GetDlgItem( m_hTab, VID_STENCIL ), false );
	EnableWindow( GetDlgItem( m_hTab, VID_PAGEFLIP ), false );
	EnableWindow( GetDlgItem( m_hTab, VID_VSYNC ), false );
	EnableWindow( GetDlgItem( m_hTab, VID_ENUM ), false );
	
//fulscreen.
	SendDlgItemMessage( m_hTab, VID_FULL, BM_SETCHECK, m_VideoData->fullscreen, 0 );
	SendDlgItemMessage( m_hTab, VID_WINDOW, BM_SETCHECK, !m_VideoData->fullscreen, 0 );
	if( m_VideoData->fullscreen ) {
		EnableWindow( GetDlgItem( m_hTab, VID_ASPECT ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_WIDTH ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_HEIGHT ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_4X3 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_16X10 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_16X9 ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_MODE ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_VSYNC ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_PAGEFLIP ), true );
	}
	else {
		EnableWindow( GetDlgItem( m_hTab, VID_ASPECT ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_WIDTH ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_HEIGHT ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_4X3 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_16X10 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_16X9 ), true );
		EnableWindow( GetDlgItem( m_hTab, VID_MODE ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_VSYNC ), false );
		EnableWindow( GetDlgItem( m_hTab, VID_PAGEFLIP ), false );
	}

	CreateWindowA( "button", "D3D11Client Configuration", WS_CHILD | BS_PUSHBUTTON, 5, 310, 407, 50, m_hTab, (HMENU)VID_D3D11CONFIG, m_hDLL, 0 );
	HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	SendMessage( GetDlgItem( m_hTab, VID_D3D11CONFIG ), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
//	GetWindowRect( m_hTab, &mInitTabRect );

	ShowWindow( GetDlgItem( m_hTab, VID_D3D11CONFIG ), SW_SHOW );
	EnableWindow( GetDlgItem( m_hTab, VID_D3D11CONFIG ), true );
}

//================================================================
//			"D3D11Client configuration"
//================================================================

LRESULT CALLBACK gclbkConfigProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	return g_VideoTab->configProc( hwnd, msg, wp, lp );
}

LRESULT CALLBACK gclbkGeneralProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	return g_VideoTab->generalProc( hwnd, msg, wp, lp );
}

LRESULT CALLBACK gclbkVesselProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	return g_VideoTab->vesselProc( hwnd, msg, wp, lp );
}

LRESULT CALLBACK gclbkPlanetsProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	return g_VideoTab->planetProc( hwnd, msg, wp, lp );
}

LRESULT VideoTab::configProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ) {
	case WM_DESTROY:
		m_hConfig = NULL;
		return TRUE;

	case WM_COMMAND:

		switch( LOWORD( wp ) ) {
		case CTRL_OK:
			if( HIWORD( wp ) == BN_CLICKED ) {
			    m_Config.copyToFileConfig(m_FileConfig);
				m_Config.saveConfig();			//writes config to file
				m_Config.applyConfig();
				DestroyWindow( m_hConfig );	//destroys itself
				m_hConfig = NULL;
				return TRUE;
			}
			break;
		case CTRL_CANCEL:
			if( HIWORD( wp ) == BN_CLICKED ) {
				DestroyWindow( m_hConfig );
				m_hConfig = NULL;
				return TRUE;
			}
			break;
		case CTRL_GENERAL:
			if( HIWORD( wp ) == BN_CLICKED ) {
				EnableWindow( m_hGeneral, true );
				EnableWindow( m_hVessels, false );
				EnableWindow( m_hPlanets, false );
				ShowWindow( m_hGeneral, true );
				ShowWindow( m_hVessels, false );
				ShowWindow( m_hPlanets, false );
				SendDlgItemMessage( m_hConfig, CTRL_GENERAL, BM_SETCHECK, 1, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_VESSELS, BM_SETCHECK, 0, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_PLANETS, BM_SETCHECK, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_VESSELS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				EnableWindow( m_hGeneral, false );
				EnableWindow( m_hVessels, true );
				EnableWindow( m_hPlanets, false );
				ShowWindow( m_hGeneral, false );
				ShowWindow( m_hVessels, true );
				ShowWindow( m_hPlanets, false );
				SendDlgItemMessage( m_hConfig, CTRL_GENERAL, BM_SETCHECK, 0, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_VESSELS, BM_SETCHECK, 1, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_PLANETS, BM_SETCHECK, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_PLANETS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				EnableWindow( m_hGeneral, false );
				EnableWindow( m_hVessels, false );
				EnableWindow( m_hPlanets, true );
				ShowWindow( m_hGeneral, false );
				ShowWindow( m_hVessels, false );
				ShowWindow( m_hPlanets, true );
				SendDlgItemMessage( m_hConfig, CTRL_GENERAL, BM_SETCHECK, 0, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_VESSELS, BM_SETCHECK, 0, 0 );
				SendDlgItemMessage( m_hConfig, CTRL_PLANETS, BM_SETCHECK, 1, 0 );
				return TRUE;
			}
			break;
		}

	}
	return DefWindowProcA( hwnd, msg, wp, lp );
}

LRESULT VideoTab::generalProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	static char cbuf[32];
	static float transp;

	switch( msg ) {
	case WM_COMMAND:

		switch( LOWORD( wp ) ) {
		case CTRL_G_THREADS:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.Thread_mode = SendDlgItemMessage( m_hGeneral, CTRL_G_THREADS, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_G_AA:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.AA_mode = SendDlgItemMessage( m_hGeneral, CTRL_G_AA, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_G_SKPAD:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.Sketchpad_mode = SendDlgItemMessage( m_hGeneral, CTRL_G_SKPAD, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_G_MFDTEX:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.MFD_filter = SendDlgItemMessage( m_hGeneral, CTRL_G_MFDTEX, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_G_MFDTRF:
			if( HIWORD(wp) == EN_CHANGE ) {
				GetWindowTextA( GetDlgItem( m_hGeneral, CTRL_G_MFDTRF ), cbuf, 32 );
				sscanf_s( cbuf, "%f", &transp );
				m_FileConfig.MFD_transparency = (WORD)transp;
				return TRUE;
			}
		case CTRL_G_PP_EFFECTS:
			if( HIWORD(wp) == CBN_SELCHANGE ) 
			{
				//auto isChecked = IsDlgButtonChecked(m_hGeneral, CTRL_G_PP_EFFECTS);
				m_FileConfig.PP_effect_enable = SendDlgItemMessage( m_hGeneral, CTRL_G_PP_EFFECTS, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_G_THREADS_HELP:
			if( HIWORD( wp ) == BN_CLICKED ) {
				MessageBoxA( m_hGeneral, "In singlethreaded mode Orbiter waits while D3D11Client perform rendering of a scene and can proceed only after scene was rendered.\nThe 2-threaded mode allows Orbiter thread to perform its calculations on one CPU core, while rendering is being performed on another CPU core. Frequent usage of 2-D drawings can reduce benefit from 2-threaded mode.", "Help", MB_OK );
				return TRUE;
			}
			break;
		case CTRL_G_AA_HELP:
			if( HIWORD( wp ) == BN_CLICKED ) {
				MessageBoxA( m_hGeneral, "Antialising modes that current GPU supports. CSAA modes available for NVidia cards only.", "Help", MB_OK );
				return TRUE;
			}
			break;
		case CTRL_G_SKPAD_HELP:
			if( HIWORD( wp ) == BN_CLICKED ) {
				MessageBoxA( m_hGeneral, "GDI only - setting force usage of GDI even for back buffer.\n\nD3D11Pad only - force usage of hardware-accelerated sketchpad for any surface.(except direct GetDC calls).This setting allows maximum performance, but some old MFDs(which can directly use GDI DC) will not work correctly.\n\nD3D11Pad for back buffer only - alows GDI usage only for MFDs.\n\nDebug string, markers and HUD works through D3D11Pad regardless user's choice.", "Help", MB_OK );
				return TRUE;
			}
			break;
		case CTRL_G_CREATE_SYM:
			if( HIWORD( wp ) == BN_CLICKED ) 
			{
				createSymbolicLinks();
			}
			break;
		}
	}
	return DefWindowProcA( hwnd, msg, wp, lp );
}

LRESULT VideoTab::vesselProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg ) {
	case WM_COMMAND:
		
		switch( LOWORD( wp ) ) {
		case CTRL_V_MIPMAP:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.Mesh_Texture_Mip_maps = SendDlgItemMessage( m_hVessels, CTRL_V_MIPMAP, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_MIPMAP_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Mip maps allows a texture appear better from distance. Full mip map chain usege is recommended.", "Help", MB_OK );
			break;
		case CTRL_V_TEXFILTER:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.Mesh_Texture_Filter = SendDlgItemMessage( m_hVessels, CTRL_V_TEXFILTER, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_TEXFILTER_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Texture filtration of vessel textures and normal/bump/specular/emissive maps. Recommended setting - Anisotropic x8 or x16.", "Help", MB_OK );
			break;
		case CTRL_V_NORM:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.Mesh_Normal_maps = SendDlgItemMessage( m_hVessels, CTRL_V_NORM, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_NORM, BM_SETCHECK, m_FileConfig.Mesh_Normal_maps, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_NORM_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Normal maps store normal vectors in RGB image. They should be used for creation of bumpy surfaces without addition of vertices.\nSupported formats: DXT1/3/5, R8G8B8, R8G8B8A8 .dds files. Alpha channel not used. ", "Help", MB_OK );
			break;
		case CTRL_V_BUMP:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.Mesh_Bump_maps = SendDlgItemMessage( m_hVessels, CTRL_V_BUMP, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_BUMP, BM_SETCHECK, m_FileConfig.Mesh_Bump_maps, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_BUMP_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Bump maps are heightmaps of a surface. Lower(inner) parts are darker, higher(outer) parts are lighter.\nSupported formats: DXT1/3/5, L8, R8G8B8, R8G8B8A8 and other .dds files. Only first color channel is used.", "Help", MB_OK );
			break;
		case CTRL_V_SPEC:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.Mesh_Specular_maps = SendDlgItemMessage( m_hVessels, CTRL_V_SPEC, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_SPEC, BM_SETCHECK, m_FileConfig.Mesh_Specular_maps, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_SPEC_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Specular maps store specular color in RGB components and specular power in alpha channel. Specular maps should be used for surfaces that has different specular color and/or specular power.\nSupported formats: DXT5, R8G8B8A8 .dds files. Alpha channel required.", "Help", MB_OK );
			break;
		case CTRL_V_EMIS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.Mesh_Emissive_maps = SendDlgItemMessage( m_hVessels, CTRL_V_EMIS, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_EMIS, BM_SETCHECK, m_FileConfig.Mesh_Emissive_maps, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_EMIS_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Emissive maps store emissive color and should be used for hot or light emitting parts of vessel surface.\nSupported formats: DXT 1/3/5, R8G8B8 and R8G8B8A8 .dds files. DXT1 recommended.", "Help", MB_OK );
			break;
		case CTRL_V_PSLLIGHTS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.PStream_LLights = SendDlgItemMessage( m_hVessels, CTRL_V_PSLLIGHTS, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_PSLLIGHTS, BM_SETCHECK, m_FileConfig.PStream_LLights, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_PSLLIGHTS_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Enables lighting of diffuse particle streams by vessel local lights", "Help", MB_OK );
			break;
		case CTRL_V_BASELLIGHTS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.Base_LLights = SendDlgItemMessage( m_hVessels, CTRL_V_BASELLIGHTS, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_BASELLIGHTS, BM_SETCHECK, m_FileConfig.Base_LLights, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_BASELLIGHTS_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Enables lighting of base tiles and meshes by vessel local lights", "Help", MB_OK );
			break;
		case CTRL_V_TILELLIGHTS:
			if( HIWORD( wp ) == BN_CLICKED ) {
				m_FileConfig.PlanetTile_LLights = SendDlgItemMessage( m_hVessels, CTRL_V_TILELLIGHTS, BM_GETCHECK, 0, 0 ) ? 0 : 1;
				SendDlgItemMessage( m_hVessels, CTRL_V_TILELLIGHTS, BM_SETCHECK, m_FileConfig.PlanetTile_LLights, 0 );
				return TRUE;
			}
			break;
		case CTRL_V_TILELLIGHTS_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hVessels, "Enables lighting of planet tiles by vessel local lights.", "Help", MB_OK );
			break;
		}
	}
	return DefWindowProcA( hwnd, msg, wp, lp );
}

LRESULT VideoTab::planetProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	static char cbuf[32];
	static float lfreq;

	switch( msg ) {
	case WM_COMMAND:
		
		switch( LOWORD( wp ) ) {
		case CTRL_P_TEXFILTER:
			if( HIWORD( wp ) == CBN_SELCHANGE ) {
				m_FileConfig.Planet_Texture_filter = SendDlgItemMessage( m_hPlanets, CTRL_P_TEXFILTER, CB_GETCURSEL, 0, 0 );
				return TRUE;
			}
			break;
		case CTRL_P_TEXFILTER_HLP:
			if( HIWORD( wp ) == BN_CLICKED )
				MessageBoxA( m_hPlanets, "Texture filtration mode for planet textures.", "Help", MB_OK );
			break;
		case CTRL_P_LFREQ:
			if( HIWORD(wp) == EN_CHANGE ) {
				GetWindowTextA( GetDlgItem( m_hPlanets, CTRL_P_LFREQ ), cbuf, 32 );
				sscanf_s( cbuf, "%f", &lfreq );
				m_FileConfig.Planet_Tile_loading_Freq = (WORD)lfreq;
				return TRUE;
			}
			break;
		}
	}
	return DefWindowProcA( hwnd, msg, wp, lp );
}

void VideoTab::initConfigWindow()
{
	HFONT hFont;
	HWND hw;
	WNDCLASSA wc;

	m_Config.loadConfig();
	m_Config.applyConfig();

	wc.lpszClassName = "Config";
	wc.lpszMenuName = NULL;
	wc.hInstance = m_hDLL;
	wc.hCursor = LoadCursor( 0, IDC_ARROW );
	wc.hIcon = LoadIcon( 0, IDI_APPLICATION );
	wc.lpfnWndProc = gclbkConfigProc;               // The dialog callback. TODO: Why not set the member function as callback directly ?
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hbrBackground = CreateSolidBrush( RGB( 245, 245, 245 ) );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassA( &wc );

	// This is the window
	m_hConfig = CreateWindowExA( WS_EX_CLIENTEDGE, "Config", "D3D11Client Configuration",
		WS_MINIMIZEBOX | WS_VISIBLE | WS_CAPTION | WS_POPUPWINDOW, 400, 100, 370, 550, m_hTab, 0, m_hDLL, NULL );

	// Set the buttons for the D3D11Client Configuration dialog

	hw = CreateWindowA( "button", "OK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 110, 483, 115, 30, m_hConfig, (HMENU)CTRL_OK, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 235, 483, 115, 30, m_hConfig, (HMENU)CTRL_CANCEL, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	// Pre-create the windows triggered from the D3D11Client Configuration dialog

	hw = CreateWindowA( "button", "General", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_PUSHLIKE , 5, 5, 100, 30, m_hConfig, (HMENU)CTRL_GENERAL, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "Vessel", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_PUSHLIKE, 5, 40, 100, 30, m_hConfig, (HMENU)CTRL_VESSELS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "Planets", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_PUSHLIKE, 5, 75, 100, 30, m_hConfig, (HMENU)CTRL_PLANETS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
	
	// Init the window  controls too so they are ready when needed
	initGeneralWindow();
	initVesselWindow();
	initPlanetWindow();

	ShowWindow( m_hGeneral, true );
	EnableWindow( m_hGeneral, true );
	SendDlgItemMessage( m_hConfig, CTRL_GENERAL, BM_SETCHECK, 1, 0 );
	SendDlgItemMessage( m_hConfig, CTRL_VESSELS, BM_SETCHECK, 0, 0 );
	SendDlgItemMessage( m_hConfig, CTRL_PLANETS, BM_SETCHECK, 0, 0 );

	// Init the config window with whatever was read in from the .cfg file in the D3D11Config object
	m_Config.copyFromFileConfig(m_FileConfig);
	
	initConfig();
}

void VideoTab::initConfig()
{
	char cbuf[32];

	for (auto AAMode : m_Config.vecAAModes)
		SendDlgItemMessageA(m_hGeneral, CTRL_G_AA, CB_ADDSTRING, 0, (LPARAM)AAMode.desc.data());

	LRESULT res = SendDlgItemMessageA( m_hGeneral, CTRL_G_THREADS, CB_SETCURSEL, m_FileConfig.Thread_mode, 0 );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_AA, CB_SETCURSEL, m_FileConfig.AA_mode, 0 );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_SKPAD, CB_SETCURSEL, m_FileConfig.Sketchpad_mode, 0 );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_MFDTEX, CB_SETCURSEL, m_FileConfig.MFD_filter, 0 );
	sprintf_s( cbuf, "%d", m_FileConfig.MFD_transparency );
	SetWindowTextA( GetDlgItem( m_hGeneral, CTRL_G_MFDTRF ), cbuf );
	
	SendDlgItemMessageA( m_hGeneral, CTRL_G_PP_EFFECTS, CB_SETCURSEL, m_FileConfig.PP_effect_enable, 0 );
	//CheckDlgButton(m_hGeneral, CTRL_G_PP_EFFECTS, m_FileConfig.PP_effect_enable == 1 ? BST_CHECKED : BST_UNCHECKED);

	SendDlgItemMessageA( m_hVessels, CTRL_V_MIPMAP, CB_SETCURSEL, m_FileConfig.Mesh_Texture_Mip_maps, 0 );
	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_SETCURSEL, m_FileConfig.Mesh_Texture_Filter, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_NORM, BM_SETCHECK, m_FileConfig.Mesh_Normal_maps, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_BUMP, BM_SETCHECK, m_FileConfig.Mesh_Bump_maps, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_SPEC, BM_SETCHECK, m_FileConfig.Mesh_Specular_maps, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_EMIS, BM_SETCHECK, m_FileConfig.Mesh_Emissive_maps, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_PSLLIGHTS, BM_SETCHECK, m_FileConfig.PStream_LLights, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_BASELLIGHTS, BM_SETCHECK, m_FileConfig.Base_LLights, 0 );
	SendDlgItemMessage( m_hVessels, CTRL_V_TILELLIGHTS, BM_SETCHECK, m_FileConfig.PlanetTile_LLights, 0 );

	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_SETCURSEL, m_FileConfig.Planet_Texture_filter, 0 );
	sprintf_s( cbuf, "%d", m_FileConfig.Planet_Tile_loading_Freq );
	SetWindowTextA( GetDlgItem( m_hPlanets, CTRL_P_LFREQ ), cbuf );
}

void VideoTab::initGeneralWindow()
{
	WNDCLASSA wc;
	HWND hw;
	HFONT hFont;

	wc.lpszClassName = "General_cfg";
	wc.lpszMenuName = NULL;
	wc.hInstance = m_hDLL;
	wc.hCursor = LoadCursor( 0, IDC_ARROW );
	wc.hIcon = LoadIcon( 0, IDI_APPLICATION );
	wc.lpfnWndProc = gclbkGeneralProc;
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hbrBackground = CreateSolidBrush( RGB( 240, 240, 240 ) );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassA( &wc );

	m_hGeneral = CreateWindowExA( WS_EX_CLIENTEDGE, "General_cfg", "Planets",
		WS_CHILD | WS_BORDER , 110, 5 , 245, 473, m_hConfig, 0, m_hDLL, NULL );

	//===============================================================

	hw = CreateWindowA( "button", "Threading", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5, 220, 50, m_hGeneral, (HMENU)9102, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "render type", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 27, 177, 40, m_hGeneral, (HMENU)CTRL_G_THREADS, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hGeneral, CTRL_G_THREADS, CB_ADDSTRING, 0, (LPARAM)"Single - Threaded" );
//	SendDlgItemMessageA( m_hGeneral, CTRL_G_THREADS, CB_ADDSTRING, 0, (LPARAM)"Separated rendering thread" );

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 27, 30, 20, m_hGeneral, (HMENU)CTRL_G_THREADS_HELP, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//===============================================================

	hw = CreateWindowA( "button", "Anti-Aliasing", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 55, 220, 50, m_hGeneral, (HMENU)9104, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "render type", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 77, 177, 40, m_hGeneral, (HMENU)CTRL_G_AA, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 77, 30, 20, m_hGeneral, (HMENU)CTRL_G_AA_HELP, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		
	//===============================================================

	hw = CreateWindowA( "button", "Sketchpad mode", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 110, 220, 50, m_hGeneral, (HMENU)9106, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "render type", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 132, 177, 40, m_hGeneral, (HMENU)CTRL_G_SKPAD, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hGeneral, CTRL_G_SKPAD, CB_ADDSTRING, 0, (LPARAM)"GDI only" );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_SKPAD, CB_ADDSTRING, 0, (LPARAM)"D3D11Pad for back buffer only" );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_SKPAD, CB_ADDSTRING, 0, (LPARAM)"D3D11Pad only" );

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 132, 30, 20, m_hGeneral, (HMENU)CTRL_G_SKPAD_HELP, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//===============================================================

	hw = CreateWindowA( "button", "MFD texture filter", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 165, 190, 50, m_hGeneral, (HMENU)9108, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "render type", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 187, 180, 40, m_hGeneral, (HMENU)CTRL_G_MFDTEX, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hGeneral, CTRL_G_MFDTEX, CB_ADDSTRING, 0, (LPARAM)"Point filtration" );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_MFDTEX, CB_ADDSTRING, 0, (LPARAM)"Linear filtration" );

	hw = CreateWindowA( "button", "MFD transparency (%)", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 220, 190, 50, m_hGeneral, (HMENU)9112, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "edit", "90", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 10, 242, 50, 22, m_hGeneral, (HMENU)CTRL_G_MFDTRF, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//enable PP effects checkbox
	hw = CreateWindowA( "button", "Post-Processing Engine", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 269, 190, 50, m_hGeneral, (HMENU)CTRL_G_PP_EFFECTS + 1, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "post-processing engine", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 5, 291, 180, 26, m_hGeneral, (HMENU)CTRL_G_PP_EFFECTS, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
	SendDlgItemMessageA( m_hGeneral, CTRL_G_PP_EFFECTS, CB_ADDSTRING, 0, (LPARAM)"Pixel Shaders" );
	SendDlgItemMessageA( m_hGeneral, CTRL_G_PP_EFFECTS, CB_ADDSTRING, 0, (LPARAM)"Compute Shaders" );
#ifdef _DEBUG
	SendDlgItemMessageA( m_hGeneral, CTRL_G_PP_EFFECTS, CB_ADDSTRING, 0, (LPARAM)"None" );
#endif

	hw = CreateWindowA( "button", "Create Symbolic Links", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 319, 190, 26, m_hGeneral, (HMENU)CTRL_G_CREATE_SYM, m_hDLL, NULL );
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	if (symbolicLinksCreated())
	{
		auto wnd = GetDlgItem(m_hGeneral, CTRL_G_CREATE_SYM);
		SetWindowText(wnd, "Symbolic Links Already Exist");
		EnableWindow(wnd, FALSE);
	}
}

void VideoTab::initVesselWindow()
{
	WNDCLASSA wc;
	HWND hw;;
	HFONT hFont;

	wc.lpszClassName = "Vessel_cfg";
	wc.lpszMenuName = NULL;
	wc.hInstance = m_hDLL;
	wc.hCursor = LoadCursor( 0, IDC_ARROW );
	wc.hIcon = LoadIcon( 0, IDI_APPLICATION );
	wc.lpfnWndProc = gclbkVesselProc;
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hbrBackground = CreateSolidBrush( RGB( 240, 240, 240 ) );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassA( &wc );

	m_hVessels = CreateWindowExA( WS_EX_CLIENTEDGE, "Vessel_cfg", "Vessels",
		WS_CHILD | WS_BORDER , 110, 5, 245, 473, m_hConfig, 0, m_hDLL, NULL );

	hw = CreateWindowA( "button", "General texture settings", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5, 230, 120, m_hVessels, (HMENU)9202, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//mip mapping
	hw = CreateWindowA( "button", "Mip mapping", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 20, 220, 50, m_hVessels, (HMENU)9202, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "mipmaps", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 15, 42, 177, 40, m_hVessels, (HMENU)CTRL_V_MIPMAP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hVessels, CTRL_V_MIPMAP, CB_ADDSTRING, 0, (LPARAM)"No mip maps" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_MIPMAP, CB_ADDSTRING, 0, (LPARAM)"4 mip maps" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_MIPMAP, CB_ADDSTRING, 0, (LPARAM)"Full mip map chain" );

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 195, 42, 30, 20, m_hVessels, (HMENU)CTRL_V_MIPMAP_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//filter
	hw = CreateWindowA( "button", "Texture filtration", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, 70, 220, 50, m_hVessels, (HMENU)9204, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "texfilter", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 15, 92, 177, 40, m_hVessels, (HMENU)CTRL_V_TEXFILTER, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"None (point)" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Linear" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x4" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x8" );
	SendDlgItemMessageA( m_hVessels, CTRL_V_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x16" );

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 195, 92, 30, 20, m_hVessels, (HMENU)CTRL_V_TEXFILTER_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "Extended vessel texture mapping", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 130, 230, 137, m_hVessels, (HMENU)9202, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//normal maps
	hw = CreateWindowA( "button", "Enable normal maps (_norm.dds)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 145, 190, 30, m_hVessels, (HMENU)CTRL_V_NORM, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 150, 30, 20, m_hVessels, (HMENU)CTRL_V_NORM_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//bump maps
	hw = CreateWindowA( "button", "Enable bump maps (_bump.dds)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 175, 190, 30, m_hVessels, (HMENU)CTRL_V_BUMP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 180, 30, 20, m_hVessels, (HMENU)CTRL_V_BUMP_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//specular maps
	hw = CreateWindowA( "button", "Enable specular maps (_spec.dds)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 205, 190, 30, m_hVessels, (HMENU)CTRL_V_SPEC, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 210, 30, 20, m_hVessels, (HMENU)CTRL_V_SPEC_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//emissive maps
	hw = CreateWindowA( "button", "Enable emissive maps (_ems.dds)", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 235, 190, 30, m_hVessels, (HMENU)CTRL_V_EMIS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 240, 30, 20, m_hVessels, (HMENU)CTRL_V_EMIS_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
/*
	hw = CreateWindowA( "button", "Extended vessel local lights", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 270, 230, 107, m_hVessels, (HMENU)9202, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//particle streams
	hw = CreateWindowA( "button", "Enable particle stream lighting", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 285, 190, 30, m_hVessels, (HMENU)CTRL_V_PSLLIGHTS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 290, 30, 20, m_hVessels, (HMENU)CTRL_V_PSLLIGHTS_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//base lights
	hw = CreateWindowA( "button", "Enable base lighting", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 315, 190, 30, m_hVessels, (HMENU)CTRL_V_BASELLIGHTS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 320, 30, 20, m_hVessels, (HMENU)CTRL_V_BASELLIGHTS_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	//planet tile
	hw = CreateWindowA( "button", "Enable planet tile lighting", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 345, 190, 30, m_hVessels, (HMENU)CTRL_V_TILELLIGHTS, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 350, 30, 20, m_hVessels, (HMENU)CTRL_V_TILELLIGHTS_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
*/
}

void VideoTab::initPlanetWindow()
{
	WNDCLASSA wc;
	HWND hw;;
	HFONT hFont;

	wc.lpszClassName = "Planet_cfg";
	wc.lpszMenuName = NULL;
	wc.hInstance = m_hDLL;
	wc.hCursor = LoadCursor( 0, IDC_ARROW );
	wc.hIcon = LoadIcon( 0, IDI_APPLICATION );
	wc.lpfnWndProc = gclbkPlanetsProc;
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hbrBackground = CreateSolidBrush( RGB( 240, 240, 240 ) );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassA( &wc );

	m_hPlanets = CreateWindowExA( WS_EX_CLIENTEDGE, "Planet_cfg", "Planets",
		WS_CHILD | WS_BORDER , 110, 5, 245, 473, m_hConfig, 0, m_hDLL, NULL );

	hw = CreateWindowA( "button", "Planet Texture filtration", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5, 220, 50, m_hPlanets, (HMENU)9204, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "combobox", "Ptexfilter", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 27, 177, 40, m_hPlanets, (HMENU)CTRL_P_TEXFILTER, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"None (point)" );
	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Linear" );
	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x4" );
	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x8" );
	SendDlgItemMessageA( m_hPlanets, CTRL_P_TEXFILTER, CB_ADDSTRING, 0, (LPARAM)"Anisotropic x16" );

	hw = CreateWindowA( "button", "?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 190, 27, 30, 20, m_hPlanets, (HMENU)CTRL_P_TEXFILTER_HLP, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "button", "Tile loading frequency (10-200 Hz)", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 55, 190, 50, m_hPlanets, (HMENU)9304, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

	hw = CreateWindowA( "edit", "50", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 10, 77, 50, 22, m_hPlanets, (HMENU)CTRL_P_LFREQ, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
/*
	hw = CreateWindowA( "button", "Preload tiles at start", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 110, 190, 30, m_hPlanets, (HMENU)CTRL_P_LMODE, m_hDLL, NULL );
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage( hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
*/
}


void VideoTab::createSymbolicLinks()
{
	DWORD soundAttrib = GetFileAttributes("Modules/Server/Sound");
	bool success = true;
	if (soundAttrib == INVALID_FILE_ATTRIBUTES)
	{
		char dir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dir);
		sprintf_s(dir, "%s\\Sound", dir);
		success &= createLink("Modules\\Server\\Sound", dir);
	}
	DWORD configAttrib = GetFileAttributes("Modules/Server/Config");
	if (configAttrib == INVALID_FILE_ATTRIBUTES)
	{
		char dir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dir);
		sprintf_s(dir, "%s\\Config", dir);
		success &= createLink("Modules\\Server\\Config", dir);
	}
	if (success)
	{
		auto wnd = GetDlgItem(m_hGeneral, CTRL_G_CREATE_SYM);
		SetWindowText(wnd, "Symbolic Links Already Exist");
		EnableWindow(wnd, FALSE);
		MessageBox(0, "Symbolic links have been successfully created!", "Success!", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		SHOW_MESSAGE(nullptr, "Unable to create symbolic links");
	}
}

bool VideoTab::createLink(
    const char * linkName,
    const char * destination)
{
	auto result = CreateSymbolicLink(linkName, destination, SYMBOLIC_LINK_FLAG_DIRECTORY);
	if (!result)
	{
		if (GetLastError()==0x522)
		{
			MessageBox(0, "Administrator privileges required","Error", MB_OK);
			return false;
		}
	}
	return true;
}

bool VideoTab::symbolicLinksCreated()
{
	DWORD soundAttrib = GetFileAttributes("Modules/Server/Sound");
	DWORD configAttrib = GetFileAttributes("Modules/Server/Config");
	return (soundAttrib != INVALID_FILE_ATTRIBUTES) && (configAttrib != INVALID_FILE_ATTRIBUTES);
}
