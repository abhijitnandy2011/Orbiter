/*
 * Impl
 *
 */

// Self
#include "ShaderManager.h"

// std
#include <iostream>
#include <fstream>

// DirectX
#include <D3DX11.h>  // needed ?

template<> struct ShaderProfile<ID3D11VertexShader> { static const TCHAR * name; };
template<> struct ShaderProfile<ID3D11PixelShader> { static const TCHAR * name; };
template<> struct ShaderProfile<ID3D11GeometryShader> { static const TCHAR * name; };
template<> struct ShaderProfile<ID3D11ComputeShader> { static const TCHAR * name; };
template<> struct ShaderProfile<ID3D11HullShader> { static const TCHAR * name; };
template<> struct ShaderProfile<ID3D11DomainShader> { static const TCHAR * name; };

template<typename ShaderInterface> const TCHAR * ShaderProfile<ShaderInterface>::name = _T("UNKNOWN");

const TCHAR * ShaderProfile<ID3D11VertexShader>::name =		_T("vs_") SHADER_VERSION _T("_0");
const TCHAR * ShaderProfile<ID3D11PixelShader>::name =		_T("ps_") SHADER_VERSION _T("_0");
const TCHAR * ShaderProfile<ID3D11GeometryShader>::name =	_T("gs_") SHADER_VERSION _T("_0");
const TCHAR * ShaderProfile<ID3D11ComputeShader>::name =	_T("cs_") SHADER_VERSION _T("_0");
const TCHAR * ShaderProfile<ID3D11HullShader>::name =		_T("hs_") SHADER_VERSION _T("_0");
const TCHAR * ShaderProfile<ID3D11DomainShader>::name =		_T("ds_") SHADER_VERSION _T("_0");

void InitShaderProfiles(D3D_FEATURE_LEVEL fl)
{
	switch(fl)
	{
		case D3D_FEATURE_LEVEL_10_0:
			ShaderProfile<ID3D11VertexShader>::name =		_T("vs_4_0");
			ShaderProfile<ID3D11PixelShader>::name =		_T("ps_4_0");
			ShaderProfile<ID3D11GeometryShader>::name =		_T("gs_4_0");
			ShaderProfile<ID3D11ComputeShader>::name =		_T("cs_4_0");
			ShaderProfile<ID3D11HullShader>::name =			_T("hs_4_0");
			ShaderProfile<ID3D11DomainShader>::name =		_T("ds_4_0");
			break;
		case D3D_FEATURE_LEVEL_10_1:
			ShaderProfile<ID3D11VertexShader>::name =		_T("vs_4_1");
			ShaderProfile<ID3D11PixelShader>::name =		_T("ps_4_1");
			ShaderProfile<ID3D11GeometryShader>::name =		_T("gs_4_1");
			ShaderProfile<ID3D11ComputeShader>::name =		_T("cs_4_1");
			ShaderProfile<ID3D11HullShader>::name =			_T("hs_4_1");
			ShaderProfile<ID3D11DomainShader>::name =		_T("ds_4_1");
			break;
		case D3D_FEATURE_LEVEL_11_0:
			ShaderProfile<ID3D11VertexShader>::name =		_T("vs_5_0");
			ShaderProfile<ID3D11PixelShader>::name =		_T("ps_5_0");
			ShaderProfile<ID3D11GeometryShader>::name =		_T("gs_5_0");
			ShaderProfile<ID3D11ComputeShader>::name =		_T("cs_5_0");
			ShaderProfile<ID3D11HullShader>::name =			_T("hs_5_0");
			ShaderProfile<ID3D11DomainShader>::name =		_T("ds_5_0");
			break;
	}
}

using namespace Rendering;

// The location for the shader disk cache
const std::string ShaderManager::sDiskCacheDirectoryName = "D3D11Client_ShadersCache\\";

/* Init shader purpose vs file name map - make an entry here for any new shader purposes
 * Shader file names must include the path from the Orbiter directory as the exe is
 * launched with that set as the working directory.
 */
const std::map<std::string, std::string> ShaderManager::m_shaderPurposeVsFileNameMap = {
     { "VertexShader", "Modules\\D3D11Shaders\\Test\\TestVertexShader.fx" }
    ,{ "PixelShader",  "Modules\\D3D11Shaders\\Test\\TestPixelShader.fx" }
};

