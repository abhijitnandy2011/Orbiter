/*
 * Header file
 *
 *
 *
 */
#pragma once

// std

// D3D11
#include <d3dx11.h>

// Orbiter
#include "..\..\include\GraphicsAPI.h"

// Local
#include "D3D11Config.h"


//#include "D3D11Pad.h"
//#include "Scene.h"
//#include "Texture.h"
//#include "ShaderManager.h"

namespace Rendering {
	class Scene;
}

class D3D11Config;
class MeshManager;
class TextureMgr;
class GDIPad;
class Overlay;
class VideoTab;

#define GET_SHADER(gc,fname,entry,res) (gc->getShader(_T(fname), _T(entry), res))
#define GET_SHADER_DEFINES(d3d,fname,entry,res,defines) (d3d->getShader(_T(fname), _T(entry), res, defines))

class D3D11Client : public oapi::GraphicsClient {

public:
    D3D11Client( HINSTANCE hIn );
    ~D3D11Client();

    void ToggleHUD();

    //--------------------- Begin Orbiter Public Callbacks ---------------------------

    bool clbkUseLaunchpadVideoTab() const;
    void clbkRefreshVideoData();
    BOOL LaunchpadVideoWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

    // Render/windows.
    LRESULT RenderWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    void clbkGetViewportSize( DWORD *w, DWORD *h ) const;
    bool clbkFullscreenMode() const;    //    -> D3D11Client
    bool clbkInitialise();                //    -> D3D11Client
    HWND clbkCreateRenderWindow();        //    -> D3D11Client
 //   void clbkPostCreation();            //    -> D3D11Client
    bool clbkGetRenderParam( DWORD prm, DWORD *value ) const;// = 0


    // Popups.
 /*   void clbkPreOpenPopup();        //?
    bool RenderWithPopupWindows();    //?

    // GDI.
    oapi::Sketchpad* clbkGetSketchpad( SURFHANDLE surf );                //    -> GDIPad
    void clbkReleaseSketchpad( oapi::Sketchpad *Skpad );                //    -> GDIPad
    oapi::Font* clbkCreateFont( int height, bool prop, const char *face, oapi::Font::Style style, int orientation ) const;//    -> GDIPad
    void clbkReleaseFont( oapi::Font *font ) const;                        //    -> GDIPad
    oapi::Pen* clbkCreatePen( int style, int width, DWORD col ) const;    //    -> GDIPad
    void clbkReleasePen( oapi::Pen *pen ) const;                        //    -> GDIPad
    oapi::Brush* clbkCreateBrush( DWORD col ) const;                    //    -> GDIPad
    void clbkReleaseBrush( oapi::Brush *brush ) const;                    //    -> GDIPad

    //Textures and Surfaces.
    //SURFHANDLE clbkCreateSurface( HBITMAP hbmp );                            //    -> TextureManager
    SURFHANDLE clbkCreateSurface( DWORD w, DWORD h, SURFHANDLE tpl = NULL );//    -> TextureManager
    SURFHANDLE clbkCreateTexture( DWORD w, DWORD h );                        //    -> TextureManager
    virtual SURFHANDLE clbkCreateSurfaceEx (DWORD w, DWORD h, DWORD attrib);//  -> TextureManager
    void clbkIncrSurfaceRef( SURFHANDLE srf );                            //    -> TextureManager
    bool clbkReleaseSurface( SURFHANDLE srf );                        //    -> TextureManager
    void clbkReleaseTexture( SURFHANDLE tex );
    bool clbkGetSurfaceSize( SURFHANDLE srf, DWORD *w, DWORD *h );        //    -> TextureManager
    bool clbkSetSurfaceColourKey( SURFHANDLE srf, DWORD ckey );            //    -> TextureManager
    DWORD clbkGetDeviceColour( BYTE r, BYTE g, BYTE b );                //    -> TextureManager
    HDC clbkGetSurfaceDC( SURFHANDLE srf );                                //    -> TextureManager
    void clbkReleaseSurfaceDC( SURFHANDLE srf, HDC hdc );                //    -> TextureManager
    bool clbkBlt(    SURFHANDLE tgt, DWORD tgtx, DWORD tgty,
                    SURFHANDLE src, DWORD flag ) const;                    //    -> TextureManager
    bool clbkBlt(    SURFHANDLE tgt, DWORD tgtx, DWORD tgty,
                    SURFHANDLE src, DWORD srcx, DWORD srcy,
                    DWORD w, DWORD h, DWORD flag ) const;                //    -> TextureManager
    bool clbkScaleBlt(    SURFHANDLE tgt, DWORD tgtx, DWORD tgty, DWORD tgtw, DWORD tgth,
                        SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD srcw, DWORD srch,
                        DWORD flag ) const;                                //    -> TextureManager
    // bool clbkCopyBitmap( SURFHANDLE srf, HBITMAP hbm, int x, int y, int dx, int dy );//    -> TextureManager
    bool clbkFillSurface( SURFHANDLE srf, DWORD col ) const;            //    -> TextureManager
    bool clbkFillSurface( SURFHANDLE srf, DWORD tgtx, DWORD tgty, DWORD w, DWORD h, DWORD col ) const;//    -> TextureManager

    // Vessels.
    void clbkNewVessel( OBJHANDLE hVessel );
    int clbkVisEvent( OBJHANDLE hObj, VISHANDLE vis, DWORD msg, UINT context );
    void clbkDeleteVessel( OBJHANDLE hVessel );

    // Meshes.
    MESHHANDLE clbkGetMesh( VISHANDLE vis, UINT idx );
    void clbkStoreMeshPersistent( MESHHANDLE hMesh, const char *fname );
    int clbkEditMeshGroup( DEVMESHHANDLE hMesh, DWORD didx, GROUPEDITSPEC *ges );
    bool clbkSetMeshTexture( DEVMESHHANDLE hMesh, DWORD texidx, SURFHANDLE tex );
    int clbkSetMeshMaterial( DEVMESHHANDLE hMesh, DWORD matidx, const MATERIAL *mat );
    bool clbkSetMeshProperty( DEVMESHHANDLE hMesh, DWORD prop, DWORD value );

    // Particle streams.
    oapi::ParticleStream *clbkCreateParticleStream( PARTICLESTREAMSPEC *pss ) { return NULL; };
    oapi::ParticleStream *clbkCreateExhaustStream( PARTICLESTREAMSPEC *pss, OBJHANDLE obj, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir );
    oapi::ParticleStream *clbkCreateExhaustStream( PARTICLESTREAMSPEC *pss, OBJHANDLE obj, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir );
    oapi::ParticleStream *clbkCreateReentryStream( PARTICLESTREAMSPEC *pss,    OBJHANDLE obj );
    bool clbkParticleStreamExists( const oapi::ParticleStream *ps );
    */

    //--------------------------- End Orbiter Public Callbacks ---------------------------------

    // Handle to the Render Window(different from the dialog handles of VideoTab)
    HWND m_hWindow;

    // Return the config object.
    D3D11Config& getConfig() { return m_Config; }
    const D3D11Config& getConfig() const { return m_Config; }

    void progressString( const char *line, DWORD _color );

    /* Shader loading utilities
     * This is called via the GET_SHADER() macro, will need overloads
     * for various shader types when we start using shaders.
     */
    template <typename T>
    bool getShader(const std::string fileName,
                   const std::string entryPoint,
                   ID3D11PixelShader& result,
                   const D3D10_SHADER_MACRO* pDefines = nullptr)
    {
       // return mShaderManager->getShader<T>(fileName, entryPoint, result, pDefines);
    }

    template<typename T>
    bool getShaderBytecode(const ID3D11VertexShaderPtr& shaderObject,
                           ID3DBlobPtr& bytecode)
    {
      //  return mShaderManager->getShaderBytecode(shaderObject, bytecode);
    }

    template<typename T, size_t TLayoutSize>
    bool createInputLayout(D3D11_INPUT_ELEMENT_DESC (&layout)[TLayoutSize],
                           const ID3D11VertexShaderPtr& shaderObject,
                           ID3D11InputLayoutPtr& result)
    {
        ID3DBlobPtr bytecode;
        if (!getShaderBytecode(shaderObject, bytecode))
            return false;
        UINT numElements = TLayoutSize;
        auto hr = g_d3dDevice->CreateInputLayout(layout,
                                                 numElements,
                                                 bytecode->GetBufferPointer(),
                                                 bytecode->GetBufferSize(),
                                                 &result);
        return SUCCEEDED(hr);
    }

protected:
    //TextureMgr *mTexMgr;
    //std::unique_ptr<ShaderManager> m_pShaderManager;

    //---------------- Begin Orbiter Protected callbacks -------------------------

    // Surfaces
/*    virtual int clbkBeginBltGroup (SURFHANDLE tgt) { return 0; }
    virtual int clbkEndBltGroup () { return 0; }
    bool clbkCopyBitmap( SURFHANDLE srf, HBITMAP hbm, int x, int y, int dx, int dy );//    -> TextureManager
    SURFHANDLE clbkCreateSurface( HBITMAP hbmp );                            //    -> TextureManager
    SURFHANDLE clbkLoadTexture( const char *fname, DWORD flags );
    virtual SURFHANDLE clbkLoadSurface (const char *fname, DWORD attrib);*/

    // Render.
    void clbkUpdate( bool running );
    void clbkRenderScene();// = 0
/*    bool clbkDisplayFrame();
    void clbkRender2DPanel( SURFHANDLE *srf, MESHHANDLE hMesh, MATRIX3 *T, bool transparent );*/

    // Exit.
    void clbkCloseSession( bool fastclose );
    void clbkDestroyRenderWindow( bool fastclose );

    // Screen annotations
    oapi::ScreenAnnotation *clbkCreateAnnotation();

    //---------------- End Orbiter Protected callbacks -------------------------

private:
    // HUD
    bool m_bDisplayHUD;

    void InitSplashScreen();
    void ExitSplashScreen();

    // UI/VideoTab
    bool m_bVideoTab, m_bEnumerate, m_bPopup;

    // TODO: Windows modes
    bool m_bModeChanged;
    DEVMODE m_CurrentMode;

    /*
     * Config object. This is not dynamically allocated anymore as it *has* to
     * exist after the client plugin is loaded as it has important device enumeration
     * and D3D11 COM objects. No memory is saved by dynamic allocation.
     */
    D3D11Config m_Config;

    /*
     * There is only scene in the entire client. We allocate this dynamically
     * because this needs to be allocated only after client requests rendering.
     * We could have used std::shared_ptr here.
     */
    Rendering::Scene *m_pScene;
};

extern D3D11Client *g_GraphicsClient;

