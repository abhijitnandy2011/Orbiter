/*
 * The Scene class - creates the D3D11 device
 *
 * Calls upon various objects to render themselves in the render loop
 * Called from D3D11Client::clbkRenderScene()
 *
 */

#pragma once

// D3D11
#include <d3d11.h>       // various
#include <d3dx9math.h>   // D3DXMATRIX etc

// Orbiter
#include "..\include\OrbiterAPI.h"  // For VECTOR3, OBJHANDLE etc

// Local - always include this after Dx includes
#include "DxTypes.h"

class D3D11Client;
class D3D11Config;

namespace Rendering {

struct DISPLAY_TEXTURE_VERTEX
{
    float x, y, z, w;
    float tu, tv;
};

class Scene {

public:
    Scene(D3D11Client &gc);
    ~Scene();

    /* Init, init3D called from clbkCreateRenderWindow()
     * This MUST be called before rendering, as well as the rest of the inits or
     * important dx members like swap chain, backbuffer, render target
     * will never be created !
     */
    void init3D();
    void initStatics();
    void initObjects();
    void exit3D();

    // Matrices:
    D3DXMATRIX *getViewProjection()  {    return &m_d3dmatViewProj;    }
    D3DXMATRIX *getView()            {    return &m_d3dmatView;        }
    D3DXMATRIX *getProjection()      {    return &m_d3dmatProj;        }

    // Camera
    double getCamAperture() const    {    return m_dCamAperture;    }
    float getAspect() const          {    return m_fAspect;        }
    OBJHANDLE getProxyBody() const   {    return m_objProxy;        }
    VECTOR3 getCamPos() const        {    return m_vec3CamPositionNew;    }
    float getFarPlane() const        {    return m_fFarPlane;    }

    // Textures and colors
    DWORD getBgColor() const         {    return m_dwSkyColorRGBA;   }
    D3DXVECTOR3 *getAmbientColor()   {    return &m_d3dvec3SkyColor;    }
    DWORD getPlanetariumFlag() const {    return m_dwPlanetariumFlag;    }
    float getLuminance() const       {    return 0.2f;   }

    /*
     *  Rendering order :
     *  1. Save render parameters  which can change per frame
     *  1. Update the parameters
     *  2. Render the Celestial background as textures on sphere with radius
     *     1e3 and no dpeth testing.
     *  3. Render the star as pixels with no depth testing
     *  4. ...
     */
    void saveRenderParams();
    void update();
    void renderStarsAndPlanets();
    void renderRest();


    /* View frustum culling based on bounding sphere
     * Primarily the bounding sphere passed to isVisibleInCamera(center, radius) is
     * used to check if its within the frustum planes.
     */

    /* m_vh = tan(m_dCamAperture), m_vw = m_vh*m_Config.fAspect
     * view height and width
     * tan = opp/adj, tan = opp = ht or width when adj = 1, tan < 1 if ht < width
     */
    float m_vh, m_vw;

    /*
     * From :
     * http://www.lighthouse3d.com/tutorials/view-frustum-culling/radar-approach-testing-spheres/
     *
     * They need to be recalculated whenever the perspective changes(as it depends
     * on fov). Used for accurate sphere testing during frustum culling. The Z check is easy
     * but for Y and X we need these factors.
     *
     * m_vhf = view ht factor = sphereFactorY, m_vwf = view width factor = sphereFactorX
     */
    float m_vhf, m_vwf;

    // Camera direction in global frame
    D3DXVECTOR3 m_d3dvec3CameraX;
    D3DXVECTOR3 m_d3dvec3CameraY;
    D3DXVECTOR3 m_d3dvec3CameraZ;
    bool isVisibleInCamera( D3DXVECTOR3 *vec, float rad );

    /*
     * This is a simple dot product of vec and the camera_z
     * If > 0 , then in front of camera
     */
    bool isVisiblePoint( D3DXVECTOR3 *vec );

    void setRenderTarget();

private:
    // Managers
    //CelBackground *mCelBkgrnd;

    // Pausing & are we inside cockpit ? - unused currently
    bool m_bCockpit, m_bVC, m_bPause, m_bPauseNew;

    // DirectX stuff
    DWORD m_dwBackBufferDrawCount;

    // Texture buffers:
    IDXGISurface1Ptr
        m_dxgisurfPixelBuffer;
    ID3D11Texture2DPtr
        m_d3dtex2DBackBuffer,
        m_d3dtex2DRenderTarget,
        m_d3dtex2DPixelBuffer;
    ID3D11RenderTargetViewPtr
        m_d3dRTVBackBuffer,     // TODO: why each tex 2d needs a rtv ?
        m_d3dRTVRenderTarget,
        m_d3dRTVPixelBuffer;
    ID3D11ShaderResourceViewPtr
        m_d3dSRVRenderTarget,
        m_d3dSRVPixelBuffer;
    ID3D11DepthStencilViewPtr
        m_d3dDSVRenderTarget;

    /*
     * Rendering parameters. The *New parameters are for the new frame
     * (the one about to be rendered). The other one is for the old frame
     * which has already been rendered. Keeping both around may be useful.
     */
    WORD m_wPixelBuffersCounter;    //not needed ?
    OBJHANDLE
        m_objProxy,        //proxy GBody [current frame]
        m_objProxyNew;    //proxy GBody [new frame]
    float
        m_fAspect,        //width/height of window
        m_fNearPlane,    //camera's near plane [1.0 def]
        m_fFarPlane;    //camera's far plane [1e6 def]
    double
        m_dCamAperture,
        m_dCamApertureNew;            //obtained through oapiCameraAperture()

    D3DXMATRIX
        m_d3dmatViewProjVirtCkpit,
        m_d3dmatViewProjVirtCkpitNew,
        m_d3dmatView,
        m_d3dmatViewNew,
        m_d3dmatProj,
        m_d3dmatProjNew,
        m_d3dmatViewProj,
        m_d3dmatViewProjNew;

    VECTOR3
        m_vec3CamPosition,        // [current frame]
        m_vec3CamPositionNew;     // [next frame]

    D3DXVECTOR3    m_d3dvec3SkyColor;//sky color float4
    DWORD m_dwSkyColorRGBA;            //sky color DWORD
    double m_dSkyBrightness;        //sky brightness
    int m_iSkyBkgrndLevel;                //background level
    DWORD m_dwPlanetariumFlag;    //current planetarium mode



    // 2D Sketchpad - needed for text
    COLORREF m_LabelColor[6];
    oapi::Pen
        *m_LabelPen[6],
        *m_NullPen;
    oapi::Font
        *m_LabelFont,
        *m_DebugFont;
    oapi::Brush
        *m_DebugBrush;

    /*
     * Debug string - this is not for debugging this plugin. This is for
     * printing the debug string that plugins may use. Its got from
     * oapiDebugString() & displayed in lower left corner. This will probably
     * require a sketchpad that can render directX text.
     */
    char m_DebugString[256];
    int m_iDebugLength;

    // Do not make const - config accessed through this will
    // be const but its changed here.
    D3D11Client &m_GraphicsClient;
    D3D11Config &m_Config;
};

} // namespace Rendering

