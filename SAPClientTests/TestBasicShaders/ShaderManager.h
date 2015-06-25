/*
 * The ShaderManager class loads and compiles HLSL shaders and supports upto Shader Model 5.
 * It maintains a list of shaders mapped to their purpose and is the only point where shaders
 * enter the client. This list is read to find the shader to compile from the passed name.
 * It also maintains a shader cache in memory mapping shader names to ID3DBlobs of compiled shaders.
 * This prevents re-compilation of already loaded shaders. Uploading to GPU RAM from main memory
 * and vice versa should be managed here in the future.
 * It has direct access to the D3D device. Its mostly a header only class.
 * This class should have no dependence on Orbiter headers - should be resusable in other Dx apps
 *
 * It uses the ShaderDiskCache class to manage compiled shaders stored in disk.
 * A shader is requested through getShader("ShaderName",...)
 * e.g. m_GraphicsClient->getShader(client,
 *                                  "Planet Irradiance",   <-------- the purpose
 *                                  "PS_IrradianceN",
 *                                  PS_IrradianceN);
 *
 * Shader file names must include the path from the Orbiter directory as the exe is launched with that set as
 * the working directory.
 *
 * Shaders are searched as follows:
 * 1. Look in m_shaderMemoryCache with key "ShaderName", if found return ShaderInfo::shaderObject which is the shader ptr
 *    This prevents shaders already loaded in main memory from being read from disk cache or compiled
 *    again.
 * 2. Else look in the disk cache using m_pShaderDiskCacheMgr for the compiled shader code. This prevents
 *    recompilation unless the original shader source file was modified. If it was modified it will be
 *    recompiled as searchForEntry() will return false treating it as non-existent in cache
 */

#pragma once

// system
#include <tchar.h> // for _T

// std
#include <map>
#include <string>
#include <tuple>
#include <memory>

// d3d
#include <d3d11.h>
#include <D3Dcompiler.h>

// local
#include "ShaderDiskCache.h"
#include "DxTypes.h"

#define SHADER_VERSION _T("5")

template<typename ShaderInterface> struct ShaderProfile { static const TCHAR * name; };

void InitShaderProfiles(D3D_FEATURE_LEVEL fl);

namespace Rendering {

class ShaderManager
{

public:
    ShaderManager(ID3D11Device * device,
                  bool keepAllBytecode = false)
       : m_d3dDevice(device)
       , m_bKeepAllBytecode(keepAllBytecode)
    {

        TCHAR path[MAX_PATH + 1];
        GetTempPath(MAX_PATH, path);
        std::string tempFolderPath = path;
        tempFolderPath += _T(sDiskCacheDirectoryName);

        auto attrs = ::GetFileAttributes(tempFolderPath.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            // TODO: Check if this happens: directory does not exist, create it
            ::CreateDirectory(tempFolderPath.c_str(), nullptr);
        }
        m_pShaderDiskCacheMgr.reset(new ShaderDiskCache(tempFolderPath));
    }
    ~ShaderManager()
    {
        for(auto it = m_shaderMemoryCache.begin(); it != m_shaderMemoryCache.end(); it++)
        {
            // Smart pointer inside will release the COM shader object inside this ShaderInfo
            delete (it->second);
        }
        m_shaderMemoryCache.clear();
    }

    // The location for the shader disk cache - different from source file location
    static const std::string sDiskCacheDirectoryName;
    // Shader purpose vs filename mapping
    static const std::map<std::string, std::string> m_shaderPurposeVsFileNameMap;

    /*
     * The primary API for loading a shader into main memory or retrieving one already loaded
     * Template parameter T is for the shader type (ID3D11VertexShader,.. etc)
     * The rest are needed for the final call to D3DX11CompileFromFile
     */
    template <typename T>
    bool getShader(const std::string& purpose,
                   const std::string& entryPoint,
                   T &result,
                   const D3D10_SHADER_MACRO* pDefines = nullptr)
    {
        std::string shaderProfile = ShaderProfile<T>::name;

        /*std::string key = fileName + "__EP__" + entryPoint + "__SP__" + shaderProfile;
        std::string definesStr;
        createDefinesString(pDefines, definesStr);
        key += "__D__" + definesStr;*/

        // Look up the shader file name using for the required purpose
        std::string shaderFileName;
        if (m_shaderPurposeVsFileNameMap.count(purpose) > 0) {
            shaderFileName = m_shaderMemoryCache[purpose];
        }

        // Now search for the shader in memory cache
        if (m_shaderMemoryCache.count(purpose) > 0) {
            // Found !! no need to compile, simplest case ! return shader object.
            auto val = dynamic_cast<ShaderInfo<T>*>(m_shaderMemoryCache[purpose]);
            result = val->shaderObject;
            return true;
        }
        else {
            // Not in memory, prepare a blob to receive it from disk
            ID3DBlobPtr data;
            CacheEntry entry;
            entry.keyLength = (USHORT)purpose.length();
            entry.key = purpose.c_str();

            // Look in disk cache with shader filename
            if (m_pShaderDiskCacheMgr->searchForEntry(entry, shaderFileName)) {
                // Compiled shader found in disk cache, so just load it
                // must be the latest compiled stuff. entry will contain the shader
                // bytecode after the call to searchForEntry()
                D3DCreateBlob(entry.bytecodeLength, &data);

                // Copy into data for creating the shader object below - last step
                CopyMemory(data->GetBufferPointer(),
                           entry.bytecode,
                           entry.bytecodeLength);
            }
            else {
                // The shader is not in the disk cache - recompile from file
                if (CompileShaderFromFile(shaderFileName.c_str(),
                                          entryPoint.c_str(),
                                          shaderProfile.c_str(),
                                          &data, pDefines)) {
                    // compiled bytecode in data, copy to entry to create entry
                    // to memory cache
                    entry.bytecodeLength = (UINT)data->GetBufferSize();
                    entry.bytecode = data->GetBufferPointer();
                    // add to disk with shader filename
                    m_pShaderDiskCacheMgr->addCacheEntry(entry, shaderFileName);
                }
                else {
                    return false;
                }
            }

            // Prepare for an entry into the shader memory cache
            auto shaderInfoObj = new ShaderInfo<T>();

            // Create the shader object
            if (createShaderObject(key, data, shaderInfoObj)) {
                m_shaderMemoryCache.insert(std::make_pair(purpose, shaderInfoObj));
                result = shaderInfoObj->shaderObject;
                return true;
            }
            delete shaderInfoObj;
        }
        return false;
    }

    template<typename T>
    bool getShaderBytecode(T &shaderObject, ID3DBlobPtr &bytecode)
    {
        for(auto it = m_shaderMemoryCache.begin(); it != m_shaderMemoryCache.end(); it++)
        {
            auto shaderInfoObj = dynamic_cast<ShaderInfo<T>*>(it->second);
            if (shaderInfoObj && shaderInfoObj->shaderObject == shaderObject)
            {
                bytecode = shaderInfoObj->shaderCode;
                return true;
            }
        }
        return false;
    }
private:

    /*
     * This nested interface class exists to allow storage of different shader types in 1 map
     * for the memory cache (See m_shaderMemoryCache below). There is no higher level common
     * parent that we have found so far for ID3D11VertexShaderPtr etc
     */
    class IGenericShaderInfo
    {
    public:
        IGenericShaderInfo() {}
        virtual ~IGenericShaderInfo() {}
    };
    // The above interface is specialized for different shader object types
    template<typename T> class ShaderInfo : public IGenericShaderInfo
    {
    public:
        ShaderInfo() { }
        virtual ~ShaderInfo() { }
        T shaderObject;
        ID3DBlobPtr shaderCode;
    };
    // The memory cache for shader objects which have already been loaded once
    // It maps shader purpose vs shader object
    std::map<std::string, IGenericShaderInfo*> m_shaderMemoryCache;

    template<typename T>
    bool createShaderObject(const std::string& key, ID3DBlobPtr &data, ShaderInfo<T>* result)
    {
        return false;
    }
    template<> bool createShaderObject(const std::string& key, ID3DBlobPtr &data, ShaderInfo<ID3D11VertexShaderPtr>* result)
    {
        auto res = SUCCEEDED(m_d3dDevice->CreateVertexShader(data->GetBufferPointer(),
                                                             data->GetBufferSize(),
                                                             nullptr,
                                                             &result->shaderObject));
        result->shaderCode = data;
        return res;
    }
    template<> bool createShaderObject(const std::string& key, ID3DBlobPtr &data, ShaderInfo<ID3D11PixelShaderPtr>* result)
    {
        auto res = SUCCEEDED(m_d3dDevice->CreatePixelShader(data->GetBufferPointer(),
                                                            data->GetBufferSize(),
                                                            nullptr,
                                                            &result->shaderObject));
        if (m_bKeepAllBytecode)
        {
            result->shaderCode = data;
        }
        return res;
    }

    ID3D11Device *m_d3dDevice;
    bool m_bKeepAllBytecode;

    // The manager for shader compiled code on disk
    std::unique_ptr<ShaderDiskCache> m_pShaderDiskCacheMgr;

};

} // namespace Rendering

