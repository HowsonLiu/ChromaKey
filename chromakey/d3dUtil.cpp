#include "stdafx.h"
#include "d3dUtil.h"
#include <D3DX11async.h>

//using namespace std::experimental;

HRESULT CreateShaderFromFile(
	const WCHAR* csoFileNameInOut,
	const WCHAR* hlslFileName,
	LPCSTR entryPoint,
	LPCSTR shaderModel,
	ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

// 	// 寻找是否有已经编译好的顶点着色器
// 	if (csoFileNameInOut)// && filesystem::exists(csoFileNameInOut))
// 	{
// 		return D3DReadFileToBlob(csoFileNameInOut, ppBlobOut);
// 	}
// 	else
// 	{
// 		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
// #ifdef _DEBUG
// 		// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，
// 		// 但仍然允许着色器进行优化操作
// 		dwShaderFlags |= D3DCOMPILE_DEBUG;
// 
// 		// 在Debug环境下禁用优化以避免出现一些不合理的情况
// 		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
// #endif
// 		ID3DBlob* errorBlob = nullptr;
// 		hr = D3DCompileFromFile(hlslFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
// 			dwShaderFlags, 0, ppBlobOut, &errorBlob);
// 		if (FAILED(hr))
// 		{
// 			if (errorBlob != nullptr)
// 			{
// 				OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
// 			}
// 			SAFE_RELEASE(errorBlob);
// 			return hr;
// 		}
// 
// 		// 若指定了输出文件名，则将着色器二进制信息输出
// 		if (csoFileNameInOut)
// 		{
// 			return D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut, FALSE);
// 		}
// 	}

	return hr;
}


//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT DX11CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

//     // find the file
//     WCHAR str[MAX_PATH];
//     V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, szFileName));

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
    if (FAILED(hr))
    {
        if (pErrorBlob != NULL)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        return hr;
    }
    SAFE_RELEASE(pErrorBlob);

    return S_OK;
}