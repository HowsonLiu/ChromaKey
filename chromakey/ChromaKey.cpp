#include "stdafx.h"
#include "ChromaKey.h"
#include "d3dUtil.h"
#include <D3Dx11tex.h>
#include "struct_util.h"
#include <atlstr.h>

using namespace DirectX;

ChromaKeyHelper chroma_key;

CHROMAKEY_API bool fnInitChromaKey(int, int)
{
    return chroma_key.Init();
}

CHROMAKEY_API bool fnUnInitChromaKey()
{
    return chroma_key.UnInit();
}

CHROMAKEY_API void fnConfigChromakey(unsigned int chroma_color, int similarity, int smoothness, int spill, 
	int opacity, int contrast, int brightness, int gamma)
{
    chroma_key.ConfigChromakey(chroma_color, similarity, smoothness, spill, opacity, contrast, brightness, gamma);
}

CHROMAKEY_API void fnConfigBackgroundImage(const char* path)
{
	chroma_key.ConfigBkImage(path);
}

CHROMAKEY_API void fnChromaKeyRender(
    int out_format, void* out_ptr, int out_size,
    int in_format, void* in_ptr, int in_size,
    int w, int h)
{
    chroma_key.InitTargetTexture(w, h);
    chroma_key.LoadImage(w, h, w * 4, in_ptr);
    chroma_key.LoadEffect();
    chroma_key.UpdateImage(w, h, 4 * w, in_ptr);
	chroma_key.UpdateBkImage();
    chroma_key.UpdateConfig();
    chroma_key.Render(out_format, out_ptr, out_size, in_format, in_ptr, in_size, w, h);
    chroma_key.GetRenderResult(&out_ptr);
}

ChromaKeyHelper::ChromaKeyHelper()
{
}

ChromaKeyHelper::~ChromaKeyHelper()
{
}

bool ChromaKeyHelper::Init()
{
    HRESULT hr = S_OK;
    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, 0, 0, D3D11_SDK_VERSION, &m_pD3DDevice, &featureLevel, &m_pD3DContext);
	return !FAILED(hr);
}

bool ChromaKeyHelper::UnInit()
{
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_ppShaderResourceView[0]);
	SAFE_RELEASE(m_ppShaderResourceView[1]);
	SAFE_RELEASE(m_pRasterState);
	SAFE_RELEASE(m_pZstencilState);
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pPixelConstants);
	SAFE_RELEASE(m_pSamplerState);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pVertexConstants);
	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pBackgroundTexture2D);
	SAFE_RELEASE(m_pImageTexture2D);
	SAFE_RELEASE(m_pRenderTexture2D);
	SAFE_RELEASE(m_pD3DContext);
	SAFE_RELEASE(m_pD3DDevice);
    return true;
}

void ChromaKeyHelper::ConfigChromakey(unsigned int chroma_color, int similarity, int smoothness, int spill, 
	int opacity, int contrast, int brightness, int gamma)
{
    
	m_uKeyColor = chroma_color;
    m_similarity = (float)similarity / 1000.0f;
	m_smoothness = (float)smoothness / 1000.0f;
	m_spill = (float)spill / 1000.0f;
	m_uOpacityColor = 0xFFFFFF | (((opacity * 255) / 100) << 24);
	m_contrast = contrast;
	m_brightness = brightness;
	m_gamma = gamma;
	m_bConfigUpdated = true;
}

void ChromaKeyHelper::ConfigBkImage(const char* path)
{
	_tcscpy_s(m_chBkImagePath, CA2T(path));
	m_bBkImageUpdated = true;
}

bool ChromaKeyHelper::InitTargetTexture(int width, int height)
{
    if (m_bTargetInited) return true;

	// 输出目标视图描述
    D3D11_TEXTURE2D_DESC Desc;
    ZeroMemory(&Desc, sizeof(D3D11_TEXTURE2D_DESC));
    Desc.Width = width;
    Desc.Height = height;
    Desc.MipLevels = 1;														// 1为多重纹理采样
    Desc.ArraySize = 1;														// 纹理数组的大小
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;								// rgba
    Desc.SampleDesc.Count = 1;												// 每个像素多重采样1次
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;	// 此纹理可用于shader资源以及渲染目标
    HRESULT hr = m_pD3DDevice->CreateTexture2D(&Desc, nullptr, &m_pRenderTexture2D);
    if (FAILED(hr)) return false;

    D3D11_RENDER_TARGET_VIEW_DESC rtv;
    rtv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv.Texture2D.MipSlice = 0;
    hr = m_pD3DDevice->CreateRenderTargetView(m_pRenderTexture2D, nullptr, &m_pRenderTargetView);	// targetview用于在pipeline stage共享资源
    if (FAILED(hr)) return false;

	m_nWidth = width;
	m_nHeight = height;
    m_bTargetInited = true;

    return true;
}

bool ChromaKeyHelper::UpdateBkImage()
{
	if (!m_bBkImageUpdated) return true;
    D3DX11_IMAGE_LOAD_INFO loadInfo;
    ZeroMemory(&loadInfo, sizeof(D3DX11_IMAGE_LOAD_INFO));
    loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    loadInfo.MipLevels = D3DX11_DEFAULT;
    loadInfo.MipFilter = D3DX11_FILTER_LINEAR;
    HRESULT hr = D3DX11CreateTextureFromFile(m_pD3DDevice, m_chBkImagePath, 
		&loadInfo, NULL, (ID3D11Resource**)(&m_pBackgroundTexture2D), NULL);
	if (FAILED(hr)) return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC res_desc = {};
	res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	res_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	res_desc.Texture2D.MipLevels = 1;

	hr = m_pD3DDevice->CreateShaderResourceView(m_pBackgroundTexture2D, &res_desc, &m_ppShaderResourceView[1]);
	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pBackgroundTexture2D);
		return false;
	}
	m_bBkImageUpdated = false;
    return true;
}

bool ChromaKeyHelper::LoadImage(int with, int height, int slicePitch, void * data)
{
    if (m_bImageInited) return true;
	SAFE_RELEASE(m_pImageTexture2D);	// 清空输入纹理

    D3D11_TEXTURE2D_DESC Desc;
    ZeroMemory(&Desc, sizeof(D3D11_TEXTURE2D_DESC));
    Desc.Width = with;
    Desc.Height = height;
    Desc.MipLevels = 1;
    Desc.ArraySize = 1;
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.SampleDesc.Count = 1;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = data;
    sd.SysMemPitch = slicePitch;	// width * 4
	// sd.SysMemSlicePitch 仅在3d有用，不初始化也没问题

    HRESULT hr = m_pD3DDevice->CreateTexture2D(&Desc, &sd, &m_pImageTexture2D);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC res_desc = {};
    res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    res_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    res_desc.Texture2D.MipLevels = 1;

    hr = m_pD3DDevice->CreateShaderResourceView(m_pImageTexture2D, &res_desc, &m_ppShaderResourceView[0]);
    if (FAILED(hr)) {
		SAFE_RELEASE(m_pImageTexture2D);
        return false;
    }

    m_bImageInited = true;
    return true;
}

bool ChromaKeyHelper::UpdateImage(int with, int height, int slicePitch, void * data)
{
    if (!m_pImageTexture2D) return false;
	// 取得视频帧纹理的子资源，然后更新
    D3D11_MAPPED_SUBRESOURCE resource;
    UINT subresource = D3D11CalcSubresource(0, 0, 0);
    HRESULT hr = m_pD3DContext->Map(m_pImageTexture2D, subresource, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    int size = slicePitch * height;
    memcpy_s(resource.pData, size, data, size);
    m_pD3DContext->Unmap(m_pImageTexture2D, subresource);
    return true;
}

bool ChromaKeyHelper::GetRenderResult(void **data)
{
    D3D11_TEXTURE2D_DESC desc;
    m_pRenderTexture2D->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D * pCopyTexture2D = NULL;
    HRESULT hr = m_pD3DDevice->CreateTexture2D(&desc, NULL, &pCopyTexture2D);
    if (FAILED(hr))
    {
		SAFE_RELEASE(pCopyTexture2D);
        return false;
    }
    m_pD3DContext->CopyResource(pCopyTexture2D, m_pRenderTexture2D);

    D3D11_MAPPED_SUBRESOURCE resource;
    UINT subresource = D3D11CalcSubresource(0, 0, 0);
    hr = m_pD3DContext->Map(pCopyTexture2D, subresource, D3D11_MAP_READ, 0, &resource);
    if (FAILED(hr))
    {
		SAFE_RELEASE(pCopyTexture2D);
        return false;
    }
    int s = resource.RowPitch * desc.Height;
    unsigned char* src = reinterpret_cast<unsigned char*>(resource.pData);
    memcpy_s(*data, s, src, s);
    m_pD3DContext->Unmap(pCopyTexture2D, subresource);
	SAFE_RELEASE(pCopyTexture2D);
}

bool ChromaKeyHelper::LoadEffect()
{
    if (m_bEffectInited) return true;
    HRESULT hr = S_OK;

	//////////////////////////////////////////////////////////////////////////
	// 默认混合状态对象
    D3D11_BLEND_DESC desc_blendstate = {};
    for (size_t i = 0; i < 8; i++)
        desc_blendstate.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;		// *允许写入
    hr = m_pD3DDevice->CreateBlendState(&desc_blendstate, &m_pBlendState);	
    if (FAILED(hr)) return false;

	// 创建默认深度模板状态对象，用于OM阶段深度模板测试
    D3D11_DEPTH_STENCIL_DESC desc_zstencilstate = {}; /* defaults all to off */
    hr = m_pD3DDevice->CreateDepthStencilState(&desc_zstencilstate, &m_pZstencilState);
    if (FAILED(hr)) return false;

	// 创建光栅化对象，用于RS阶段
    D3D11_RASTERIZER_DESC desc_rasterstate = {};
    desc_rasterstate.FillMode = D3D11_FILL_SOLID;	// 实体渲染并非线框渲染
    desc_rasterstate.CullMode = D3D11_CULL_NONE;	// 禁止裁剪三角形
    hr = m_pD3DDevice->CreateRasterizerState(&desc_rasterstate, &m_pRasterState);
    if (FAILED(hr)) return false;
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// 顶点着色器相关代码
    ID3DBlob* pVertexShaderBuffer = NULL;	// 保存顶点着色器的字节码
	// 从文件中编译顶点着色器，入口为main，着色器类型跟版本为vs_4.0，保存到字节码缓存中
    hr = DX11CompileShaderFromFile(L"d:\\code\\youdaoke\\myrepo\\youdaoke\\src\\plugins\\chromakey\\chromakey\\HLSL\\ChromaKey_VS.hlsl", 
		"main", "vs_4_0", &pVertexShaderBuffer);	// TODO: ASSERT
	if (FAILED(hr)) return false;
	// 从着色器字节码缓存创建一个着色器对象
    hr = m_pD3DDevice->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr)) return false;

	// 这里在C++描述了hlsli文件中的VertData结构体内容
    D3D11_INPUT_ELEMENT_DESC desc[2];
    desc[0].SemanticName = "SV_Position";
    desc[0].SemanticIndex = 0;
    desc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc[0].InputSlot = 0;
    desc[0].AlignedByteOffset = 0;
    desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[0].InstanceDataStepRate = 0;
    desc[1].SemanticName = "TEXCOORD";
    desc[1].SemanticIndex = 0;
    desc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    desc[1].InputSlot = 0;
    desc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// 在前一个元素后面
    desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    desc[1].InstanceDataStepRate = 0;

	// inputLayout建立C++结构体与hlsl结构体的对应关系
    hr = m_pD3DDevice->CreateInputLayout(desc, 2, pVertexShaderBuffer->GetBufferPointer(), 
		pVertexShaderBuffer->GetBufferSize(), &m_pVertexLayout);
	if (FAILED(hr)) return false;

	// 顶点缓冲区包含顶点数据(坐标，颜色，纹理坐标，法线数据)
	// 对应hlsl中的VertData，前面四个是pos:SV_Position，后面是uv:TEXCOORD0
    const Vertex verts[NUM_VERTS] = {
        { { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },	// 左上
        { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },	// 左下
        { { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },		// 右上
        { { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }		// 右下
    };
	// 顶点缓冲区的描述
    D3D11_BUFFER_DESC bd = { 0 };
    bd.ByteWidth = sizeof(Vertex) * NUM_VERTS;		// 一次传入NUM_VERTS个HLSL中的VertData数据
    bd.Usage = D3D11_USAGE_DEFAULT;					// 默认GPU读写
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;		// 将缓冲区作为顶点缓冲区绑定到IA阶段
    bd.CPUAccessFlags = 0;							// 不需要CPU访问
    bd.MiscFlags = 0;								// 忽略
	// 子资源数据
    D3D11_SUBRESOURCE_DATA srd = {};
    srd.pSysMem = (const void*)verts;
	// 创建顶点缓存区，将顶点数组以buffer的形式提供给IA阶段
    hr = m_pD3DDevice->CreateBuffer(&bd, &srd, &m_pVertexConstants);
	if (FAILED(hr)) return false;
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// 像素着色器相关代码
    ID3DBlob* pPixelShaderBuffer = NULL;	// 保存像素着色器的字节码
	// 编译
    hr = DX11CompileShaderFromFile(L"d:\\code\\youdaoke\\myrepo\\youdaoke\\src\\plugins\\chromakey\\chromakey\\HLSL\\ChromaKey_PS.hlsl", 
		"main", "ps_4_0", &pPixelShaderBuffer);
	if (FAILED(hr)) return false;	// TODO: ASSERT
	// 采样器状态描述
    D3D11_SAMPLER_DESC pixel_desc = {};
    pixel_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    pixel_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;		// 超出UV坐标部分使用Clamp模式
    pixel_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    pixel_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    pixel_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;	// 旧数据与新数据对比?
    pixel_desc.MaxAnisotropy = 1;	// ?
    pixel_desc.MaxLOD = FLT_MAX;	// mipmap上限
	// 创建采样器状态
    hr = m_pD3DDevice->CreateSamplerState(&pixel_desc, &m_pSamplerState);
    if (FAILED(hr)) return false;
	// 从字节码创建像素着色器
    hr = m_pD3DDevice->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(), nullptr, &m_pPixelShader);
    if (FAILED(hr)) return false;

	// hlsl中ChromaParam描述
    D3D11_BUFFER_DESC pixel_bd;
    ZeroMemory(&pixel_bd, sizeof(D3D11_BUFFER_DESC));
    pixel_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	// 常量缓存区
    pixel_bd.Usage = D3D11_USAGE_DEFAULT;				// 常量缓存区是否频繁更新
    pixel_bd.ByteWidth = sizeof(ChromaParam);
	// 创建像素着色器的常量缓冲区(后面填写)
    hr = m_pD3DDevice->CreateBuffer(&pixel_bd, NULL, &m_pPixelConstants);
    if (FAILED(hr)) return false;
	//////////////////////////////////////////////////////////////////////////

    m_bEffectInited = true;
    return true;
}

bool ChromaKeyHelper::UpdateConfig()
{
	if (!m_bConfigUpdated) return true;
	uint32_t key_color = m_uKeyColor;
	struct vec4 key_color_v4;
	struct matrix4 yuv_mat_m4;
	struct vec4 key_rgb;
	struct vec4 v4OpacityColor;

	vec4_from_rgba(&v4OpacityColor, m_uOpacityColor);

	vec4_from_rgba(&key_rgb, key_color | 0xFF000000);						// 这里是大端的rgba
	memcpy(&yuv_mat_m4, g_YuvMat, sizeof(g_YuvMat));
	vec4_transform(&key_color_v4, &key_rgb, &yuv_mat_m4);					// 将rgb数据转换为yuv数据
	XMFLOAT2 keyColorUV = XMFLOAT2(key_color_v4.y, key_color_v4.z);					// 只取yuv数据的uv部
	XMFLOAT2 pixel_size_ = XMFLOAT2(1.0f / (float)m_nWidth, 1.0f / (float)m_nHeight);	// 纹理uv中一个像素的大小

	ChromaParam cb;
	cb.color = XMFLOAT4(v4OpacityColor.x, v4OpacityColor.y, v4OpacityColor.z, v4OpacityColor.w);
	cb.chroma_key = keyColorUV;
	cb.pixel_size = pixel_size_;
	cb.contrast = m_contrast;
	cb.brightness = m_brightness;
	cb.gamma = m_gamma;
	cb.similarity = m_similarity;
	cb.smoothness = m_smoothness;
	cb.spill = m_spill;
	// 更新常量缓存区，这种方式不适合频繁调用，另一种是Map
	m_pD3DContext->UpdateSubresource(m_pPixelConstants, 0, nullptr, &cb, 0, 0);
	m_bConfigUpdated = false;

    return true;
}

void ChromaKeyHelper::Render(int out_format, void* out_ptr, int out_size, int in_format, void* in_ptr, int in_size, int w, int h)
{
	// viewport可以理解为分屏，例如在赛车对战游戏里面需要在一个窗口内显示两个玩家的界面，那么每个界面就是一个视口
    D3D11_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MaxDepth = 1;
    vp.MinDepth = 0;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;

    const float factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT stride = sizeof(Vertex);
    void *emptyptr = nullptr;
    UINT zero = 0;

    float color[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    m_pD3DContext->ClearRenderTargetView(m_pRenderTargetView, color);					// 将渲染目标的所有元素设置成一个值(非必需?)
	// 这是IA阶段，只支持两种输入缓存：顶点缓冲区以及索引缓冲区
    m_pD3DContext->IASetInputLayout(m_pVertexLayout);									// IA阶段绑定inputLayout
    m_pD3DContext->IASetVertexBuffers(0, 1, &m_pVertexConstants, &stride, &zero);		// IA阶段绑定顶点缓冲区 第0个插槽，1个缓冲区，跨步为stride，0偏移
    m_pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);		// IA阶段如何将顶点组装成图元(拓扑的方法)
    m_pD3DContext->VSSetShader(m_pVertexShader, nullptr, 0);							// VS阶段处理单个顶点，hlsl中仅仅做了简单的输出
    m_pD3DContext->GSSetShader(nullptr, nullptr, 0);									// GS阶段清空shader
    m_pD3DContext->SOSetTargets(1, (ID3D11Buffer**)&emptyptr, &zero);					// SO阶段恢复默认状态，不绑定输出的顶点缓冲区
    m_pD3DContext->RSSetState(m_pRasterState);											// RS阶段设置状态
    m_pD3DContext->RSSetViewports(1, &vp);												// RS阶段设置一个视口
    m_pD3DContext->PSSetSamplers(0, 1, &m_pSamplerState);								// PS阶段设置采样器
    m_pD3DContext->PSSetShaderResources(0, 2, m_ppShaderResourceView);					// 设置了hlsl中的全局变量image
    m_pD3DContext->PSSetShader(m_pPixelShader, nullptr, 0);								// PS阶段设置shader
    m_pD3DContext->PSSetConstantBuffers(0, 1, &m_pPixelConstants);						// PS阶段绑定常量存储区
    m_pD3DContext->OMSetBlendState(m_pBlendState, factor, 0xFFFFFFFF);
    m_pD3DContext->OMSetDepthStencilState(m_pZstencilState, 0);
    m_pD3DContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);	// 将一个或多个渲染目标以及深度模板缓存区绑定到合并输出阶段

    m_pD3DContext->Draw(4, 0);	// draw出视频帧的四个顶点
    // SaveTextureToFile(L"D:\\test.png");
}

void ChromaKeyHelper::SaveTextureToFile(const std::wstring &file)
{
    if (!m_pRenderTexture2D) return;
    D3D11_TEXTURE2D_DESC saveTextureDesc;
    m_pRenderTexture2D->GetDesc(&saveTextureDesc);

    ID3D11Texture2D * SaveTexture2D;
    ID3D11Resource * SaveTextureResource;

    saveTextureDesc.MipLevels = 1;
    saveTextureDesc.ArraySize = 1;
    saveTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    saveTextureDesc.SampleDesc.Count = 1;
    saveTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    saveTextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    saveTextureDesc.CPUAccessFlags = 0;
    saveTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	HRESULT hr = m_pD3DDevice->CreateTexture2D(&saveTextureDesc, NULL, &SaveTexture2D);
    if (FAILED(hr)) return;
    hr = SaveTexture2D->QueryInterface(__uuidof(ID3D11Resource), (void**)&SaveTextureResource);
    if (FAILED(hr)) return;

    m_pD3DContext->CopySubresourceRegion(SaveTexture2D, 0, 0, 0, 0, m_pRenderTexture2D, 0, nullptr);
    hr = D3DX11SaveTextureToFile(m_pD3DContext, SaveTextureResource, D3DX11_IFF_PNG, file.c_str());
    SaveTexture2D->Release();
}