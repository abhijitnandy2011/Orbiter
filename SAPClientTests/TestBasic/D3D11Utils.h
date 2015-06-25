/*
 * Utility functions
 */

// std
#include <fstream>

// D3D11
#include <D3DX11core.h>  // for ID3DX11ThreadPump used with CompileFromFile()
#include <d3d11.h>       // various
#include <d3dx9math.h>   // D3DXMATRIX etc
#include <DxErr.h>       // for DXTraceA

// Orbiter
#include "..\include\OrbiterAPI.h"  // For COLOUR4 - british


namespace ClientUtils {

//====================================================================
//						Math functions
//====================================================================

HRESULT D3DMAT_MatrixInvert (D3DXMATRIX *res, D3DXMATRIX *a);

float D3DMAT_BSScaleFactor(const D3DXMATRIX *mat);

void D3DMAT_RotY( D3DXMATRIX *mat, double r );

void D3DMAT_FromAxisT(
    D3DXMATRIX *mat,
    const D3DVECTOR *x,
    const D3DVECTOR *y,
    const D3DVECTOR *z );


//====================================================================
//                     Billboard functions
//====================================================================

void D3DMAT_CreateX_Billboard( const D3DXVECTOR3 *toCam, const D3DXVECTOR3 *pos, float size, D3DXMATRIX *pOut );
void D3DMAT_CreateX_Billboard( const D3DXVECTOR3 *toCam, const D3DXVECTOR3 *pos, const D3DXVECTOR3 *dir, float size, float stretch, D3DXMATRIX *pOut );

//====================================================================
//                      Type conversion functions
//====================================================================

struct PTVertex {
    float px, py;
    float tu, tv;
};

inline PTVertex _vPT( float px, float py, float tu, float tv ) {
    PTVertex V;
    V.px = px;  V.py = py;
    V.tu = tu;  V.tv = tv;
    return V;
}

inline D3DXCOLOR C4ToD4( COLOUR4 colour ) {
    D3DXCOLOR col;
    col.r = colour.r;
    col.g = colour.g;
    col.b = colour.b;
    col.a = colour.a;
    return col;
}

inline void M3ToDM( D3DXMATRIX *out, MATRIX3 *in ) {
    D3DXMatrixIdentity( out );
    out->_11 = (float)in->m11;  out->_12 = (float)in->m12;  out->_13 = (float)in->m13;
    out->_21 = (float)in->m21;  out->_22 = (float)in->m22;  out->_23 = (float)in->m23;
    out->_31 = (float)in->m31;  out->_32 = (float)in->m32;  out->_33 = (float)in->m33;
}

inline void V3toD3( D3DXVECTOR3 *out, VECTOR3 *in ) {
    out->x = (float)in->x;
    out->y = (float)in->y;
    out->z = (float)in->z;
}

inline short mod( short a, short b )
{
    if (a<0) return b-1;
    if (a>=b) return 0;
    return a;
}

inline float saturate( float x ) {
    if( x > 1 ) return 1;
    if( x < 0 ) return 0;
    return x;
}

//====================================================================
//                      Shader utility functions
//====================================================================


void ShowShaderCompilationError( HRESULT hr, ID3DBlob *EBlob );

bool CompileShaderFromFile(
    LPCSTR szFileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    ID3DBlob** ppBlobOut,
    const D3D10_SHADER_MACRO * defines );

//====================================================================
//                      String functions
//====================================================================

std::wstring string2wstring(const std::string& s);

std::string wstring2string(const std::wstring& s);

} // namespace ClientUtils

