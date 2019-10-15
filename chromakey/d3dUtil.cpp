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

// 	// Ѱ���Ƿ����Ѿ�����õĶ�����ɫ��
// 	if (csoFileNameInOut)// && filesystem::exists(csoFileNameInOut))
// 	{
// 		return D3DReadFileToBlob(csoFileNameInOut, ppBlobOut);
// 	}
// 	else
// 	{
// 		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
// #ifdef _DEBUG
// 		// ���� D3DCOMPILE_DEBUG ��־���ڻ�ȡ��ɫ��������Ϣ���ñ�־���������������飬
// 		// ����Ȼ������ɫ�������Ż�����
// 		dwShaderFlags |= D3DCOMPILE_DEBUG;
// 
// 		// ��Debug�����½����Ż��Ա������һЩ����������
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
// 		// ��ָ��������ļ���������ɫ����������Ϣ���
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