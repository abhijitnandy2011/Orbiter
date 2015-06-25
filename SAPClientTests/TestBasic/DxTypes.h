/*
 * Contains global data structures and pointers related to DirectX only
 */

#pragma once

// system
#include<comdef.h>

/*
 * COM smart pointer types
 * _com_ptr_t is a templated smart pointer for COM objects:
 * http://stackoverflow.com/questions/10153873/how-do-i-use-com-ptr-t
 *
 * Its used via macro generated specializations for the required types. The
 * new smart pointer type will be IDXGIFactory1Ptr for IDXGIFactory1.
 * We will be using this whenever we create COM objects to prevent COM leaks
 * When adding a new type, scan the list to see if its there already.
 * Make sure the IID and interface match names and in ALPHABETICAL order
 */
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter1, IID_IDXGIAdapter1);
_COM_SMARTPTR_TYPEDEF(IDXGIFactory1, IID_IDXGIFactory1);
_COM_SMARTPTR_TYPEDEF(IDXGIOutput, IID_IDXGIOutput);
_COM_SMARTPTR_TYPEDEF(IDXGISurface1, IID_IDXGISurface1);
_COM_SMARTPTR_TYPEDEF(IDXGISwapChain, IID_IDXGISwapChain);

_COM_SMARTPTR_TYPEDEF(ID3DBlob, IID_ID3DBlob);

_COM_SMARTPTR_TYPEDEF(ID3D11BlendState, IID_ID3D11BlendState);
_COM_SMARTPTR_TYPEDEF(ID3D11Buffer, IID_ID3D11Buffer);
_COM_SMARTPTR_TYPEDEF(ID3D11ComputeShader, IID_ID3D11ComputeShader);
_COM_SMARTPTR_TYPEDEF(ID3D11Debug, IID_ID3D11Debug);
_COM_SMARTPTR_TYPEDEF(ID3D11DepthStencilState, IID_ID3D11DepthStencilState);
_COM_SMARTPTR_TYPEDEF(ID3D11DepthStencilView, IID_ID3D11DepthStencilView);
_COM_SMARTPTR_TYPEDEF(ID3D11Device, IID_ID3D11Device);
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceContext, IID_ID3D11DeviceContext);
_COM_SMARTPTR_TYPEDEF(ID3D11InputLayout, IID_ID3D11InputLayout);
_COM_SMARTPTR_TYPEDEF(ID3D11PixelShader, IID_ID3D11PixelShader);
_COM_SMARTPTR_TYPEDEF(ID3D11RasterizerState, IID_ID3D11RasterizerState);
_COM_SMARTPTR_TYPEDEF(ID3D11RenderTargetView, IID_ID3D11RenderTargetView);
_COM_SMARTPTR_TYPEDEF(ID3D11Resource, IID_ID3D11Resource);
_COM_SMARTPTR_TYPEDEF(ID3D11SamplerState, IID_ID3D11SamplerState);
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderResourceView, IID_ID3D11ShaderResourceView);
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, IID_ID3D11Texture2D);
_COM_SMARTPTR_TYPEDEF(ID3D11VertexShader, IID_ID3D11VertexShader);


// COM errors
// TODO: use exception related functions using _com_error etc
#if defined DEBUG
    #ifndef HR
    #define HR(x) {         \
        HRESULT hr = (x);   \
        if( FAILED(hr)) {   \
        DXTrace( __FILE__ , (DWORD) __LINE__, hr, #x, true );   } \
        }
    #endif
#else
    #ifndef HR
    #define HR(x) (x)
    #endif
#endif

#define ReleaseCOM(x) {    if( (x) ) { (x)->Release(); (x) = nullptr;  } }
#define HRFAIL( hr ) ( ((HRESULT)(hr)) < 0 )

