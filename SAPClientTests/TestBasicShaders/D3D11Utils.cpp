/*
 * Only utility functions
 */

#pragma once

// Self
#include "D3D11Utils.h"

// Local
#include "DxTypes.h"


/* Compiler Note: Simply putting these under using namespace ClientUtils
 * will not work, their names must start with ClientUtils::
 */

//====================================================================
//						Math functions
//====================================================================

HRESULT
ClientUtils::D3DMAT_MatrixInvert (D3DXMATRIX *res, D3DXMATRIX *a)
{
    if( fabs(a->_44 - 1.0f) > .001f)
        return E_INVALIDARG;
    if( fabs(a->_14) > .001f || fabs(a->_24) > .001f || fabs(a->_34) > .001f )
        return E_INVALIDARG;

    FLOAT fDetInv = 1.0f / ( a->_11 * ( a->_22 * a->_33 - a->_23 * a->_32 ) -
                             a->_12 * ( a->_21 * a->_33 - a->_23 * a->_31 ) +
                             a->_13 * ( a->_21 * a->_32 - a->_22 * a->_31 ) );

    res->_11 =  fDetInv * ( a->_22 * a->_33 - a->_23 * a->_32 );
    res->_12 = -fDetInv * ( a->_12 * a->_33 - a->_13 * a->_32 );
    res->_13 =  fDetInv * ( a->_12 * a->_23 - a->_13 * a->_22 );
    res->_14 = 0.0f;

    res->_21 = -fDetInv * ( a->_21 * a->_33 - a->_23 * a->_31 );
    res->_22 =  fDetInv * ( a->_11 * a->_33 - a->_13 * a->_31 );
    res->_23 = -fDetInv * ( a->_11 * a->_23 - a->_13 * a->_21 );
    res->_24 = 0.0f;

    res->_31 =  fDetInv * ( a->_21 * a->_32 - a->_22 * a->_31 );
    res->_32 = -fDetInv * ( a->_11 * a->_32 - a->_12 * a->_31 );
    res->_33 =  fDetInv * ( a->_11 * a->_22 - a->_12 * a->_21 );
    res->_34 = 0.0f;

    res->_41 = -( a->_41 * res->_11 + a->_42 * res->_21 + a->_43 * res->_31 );
    res->_42 = -( a->_41 * res->_12 + a->_42 * res->_22 + a->_43 * res->_32 );
    res->_43 = -( a->_41 * res->_13 + a->_42 * res->_23 + a->_43 * res->_33 );
    res->_44 = 1.0f;

    return S_OK;
}

float
ClientUtils::D3DMAT_BSScaleFactor(const D3DXMATRIX *mat)
{
	float lx = mat->_11*mat->_11 + mat->_12*mat->_12 + mat->_13*mat->_13;
    float ly = mat->_21*mat->_21 + mat->_22*mat->_22 + mat->_23*mat->_23;
    float lz = mat->_31*mat->_31 + mat->_32*mat->_32 + mat->_33*mat->_33;
	return sqrt(max(max(lx,ly),lz));
}

void
ClientUtils::D3DMAT_RotY( D3DXMATRIX *mat, double r )
{
	double sinr = sin(r), cosr = cos(r);
	ZeroMemory (mat, sizeof (D3DXMATRIX));
	mat->_11 = mat->_33 = (FLOAT)cosr;
	mat->_31 = -(mat->_13 = (FLOAT)sinr);
	mat->_22 = mat->_44 = 1.0f;
}

void
ClientUtils::D3DMAT_FromAxisT( D3DXMATRIX *mat, const D3DVECTOR *x, const D3DVECTOR *y, const D3DVECTOR *z )
{
	mat->_11 = x->x;
	mat->_12 = x->y;
	mat->_13 = x->z;

	mat->_21 = y->x;
	mat->_22 = y->y;
	mat->_23 = y->z;

	mat->_31 = z->x;
	mat->_32 = z->y;
	mat->_33 = z->z;
}

//====================================================================
//                     Billboard functions
//====================================================================

void
ClientUtils::D3DMAT_CreateX_Billboard( const D3DXVECTOR3 *toCam, const D3DXVECTOR3 *pos, float size, D3DXMATRIX *pOut )
{
	float hz  = 1.0f/sqrt( toCam->x*toCam->x + toCam->z*toCam->z );

	pOut->_11 =  toCam->x;
	pOut->_12 =  toCam->y;
	pOut->_13 =  toCam->z;
	pOut->_31 = -toCam->z*hz;
	pOut->_32 =  0.0f;
	pOut->_33 =  toCam->x*hz;
	pOut->_21 = -pOut->_12*pOut->_33;
	pOut->_22 =  pOut->_33*pOut->_11 - pOut->_13*pOut->_31;
	pOut->_23 =  pOut->_31*pOut->_12;
	pOut->_41 =  pos->x;
	pOut->_42 =  pos->y;
	pOut->_43 =  pos->z;
	pOut->_14 = pOut->_24 = pOut->_34 = pOut->_44 = 0.0f;
	pOut->_11 *= size; pOut->_12 *= size; pOut->_13 *= size;
	pOut->_21 *= size; pOut->_22 *= size; pOut->_23 *= size;
	pOut->_31 *= size;					  pOut->_33 *= size;
	pOut->_44 = 1.0f;
}

void
ClientUtils::D3DMAT_CreateX_Billboard( const D3DXVECTOR3 *toCam, const D3DXVECTOR3 *pos, const D3DXVECTOR3 *dir, float size, float stretch, D3DXMATRIX *pOut )
{
	D3DXVECTOR3 q, w;
	D3DXVec3Normalize( &q, D3DXVec3Cross( &q, dir, toCam ) );
	D3DXVec3Normalize( &w, D3DXVec3Cross( &w, &q,  dir ) );

	pOut->_11 = w.x * size;
	pOut->_12 = w.y * size;
	pOut->_13 = w.z * size;

	pOut->_21 = q.x * size;
	pOut->_22 = q.y * size;
	pOut->_23 = q.z * size;

	pOut->_31 = dir->x * stretch;
	pOut->_32 = dir->y * stretch;
	pOut->_33 = dir->z * stretch;

	pOut->_41 = pos->x;
	pOut->_42 = pos->y;
	pOut->_43 = pos->z;

	pOut->_14 = pOut->_24 = pOut->_34 = pOut->_44 = 0.0f;
	pOut->_44 = 1.0f;
}

std::wstring
ClientUtils::string2wstring(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

std::string
ClientUtils::wstring2string(const std::wstring& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
    char* buf = new char[len];
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
    std::string r(buf);
    delete[] buf;
    return r;
}

/*====================================================================
 *                    Shader utility functions
 *====================================================================
 */

// Output to a error file but no message box immediately
void
ClientUtils::OutputShaderErrorMessage(ID3D10Blob* errorMessage, LPCSTR shaderFilename)
{
    char* compileErrors;
    unsigned long bufferSize, i;
    std::ofstream fout;


    // Get a pointer to the error message text buffer.
    compileErrors = (char*)(errorMessage->GetBufferPointer());

    // Get the length of the message.
    bufferSize = errorMessage->GetBufferSize();

    // Open a file to write the error message to.
    fout.open("shader-error.txt");

    // Write out the error message.
    for(i=0; i<bufferSize; i++)
    {
       fout << compileErrors[i];
    }

    // Close the file.
    fout.close();

    // Release the error message.
    ReleaseCOM(errorMessage);

    // Pop a message up on the screen to notify the user to check the text file for compile errors.
    MessageBox(0, "Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

    return;
}


// Creates message box with information about error but does not log in a file
void
ClientUtils::ShowShaderCompilationError( HRESULT hr, ID3DBlob *errorBlob )
{
    if( HRFAIL( hr ) )  {
        if( errorBlob ) {
            MessageBoxA( 0, (char*)errorBlob->GetBufferPointer(), 0, 0 );
            errorBlob->Release();
        }
        // Displays a message box, and passes the error code to DXGetErrorString9
        DXTraceA(__FILE__, (DWORD)__LINE__, hr, "D3DX11CompileFromFile", true);
    }
}

// The more detailed shader compiler interface
bool
ClientUtils::CompileShaderMaxOpts(
    LPCSTR pSrcFile,
    const D3D10_SHADER_MACRO* pDefines,
    LPD3D10INCLUDE pInclude,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    UINT shaderFlags,
    UINT effectsFlags,
    ID3DX11ThreadPump* pPump, // notifies progress to this thread - may be useful later
    ID3D10Blob** ppBlobOut)
{
    ID3DBlobPtr errorBlob;
#ifdef _DEBUG
    shaderFlags |= D3D10_SHADER_DEBUG;
#endif

    HRESULT result;
    D3DX11CompileFromFile(pSrcFile,
                          pDefines,
                          pInclude,
                          pFunctionName,
                          pProfile,
                          shaderFlags,
                          effectsFlags,
                          pPump,
                          ppBlobOut,
                          & (errorBlob.GetInterfacePtr()),
                          &result);

    if (FAILED(result)) {
        // If the shader failed to compile it should have written something to errorMessage.
        if(errorBlob) {
            OutputShaderErrorMessage(errorBlob, pSrcFile);
            ShowShaderCompilationError(result, errorBlob);
        }
        else {
            // If there was nothing in the error message then it simply could not find the file itself.
            MessageBox(0, pSrcFile, "Missing Shader File", MB_OK);
        }
        // Must release any compiled byte code in output
        ReleaseCOM(*ppBlobOut);
        return false;
    }

    return true;
}

// The easier shader compiler interface with less options
bool
ClientUtils::CompileShaderChkErrors(
    LPCSTR pSrcFile,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    UINT shaderFlags,
    //const D3D10_SHADER_MACRO * pDefines, - if needed add back later
    ID3DBlob** ppBlobOut)
{
    ID3DBlobPtr errorBlob;  // Was ID3D10Blob before which is abstract - revert if errors

    // Should implement classwise debug mode
#ifdef _DEBUG
    shaderFlags |= D3D10_SHADER_DEBUG;
#else

       // This is for compute shaders only - we will see about this later
    /*    if (pProfile[0] == 'c') {
           // for some reason compute shaders compilation fails on level 2 and 3
           shaderFlags |= D3D10_SHADER_OPTIMIZATION_LEVEL1;
       }
       else {
           shaderFlags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
       }*/
#endif
    HRESULT result;
    D3DX11CompileFromFile(pSrcFile,
                          NULL, //pDefines,
                          NULL,
                          pFunctionName,
                          pProfile,
                          shaderFlags,
                          0,
                          NULL,
                          ppBlobOut,
                          & (errorBlob.GetInterfacePtr()),
                          &result);

    if (FAILED(result)) {
        // If the shader failed to compile it should have written something to errorMessage.
        if(errorBlob) {
            OutputShaderErrorMessage(errorBlob, pSrcFile);
            ShowShaderCompilationError(result, errorBlob);
        }
        else {
            // If there was nothing in the error message then it simply could not find the file itself.
            MessageBox(0, pSrcFile, "Missing Shader File", MB_OK);
        }
        // Must release any compiled byte code in output
        ReleaseCOM(*ppBlobOut);
        return false;
    }

    return true;
}

