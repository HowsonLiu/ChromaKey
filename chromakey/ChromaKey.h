#include <string>
#include <d3d11.h>
#include <DirectXMath.h>
#include "struct_def.h"

#ifdef CHROMAKEY_EXPORTS
#define CHROMAKEY_API extern "C" __declspec(dllexport)
#else
#define CHROMAKEY_API extern "C" __declspec(dllimport)
#endif

CHROMAKEY_API bool fnInitChromaKey(int, int);
CHROMAKEY_API bool fnUnInitChromaKey();
CHROMAKEY_API void fnConfigChromakey(unsigned int chroma_color, int similarity, int smoothness, int spill, int opacity, int contrast, int brightness, int gamma);
CHROMAKEY_API void fnConfigBackgroundImage(const char* path);
CHROMAKEY_API void fnChromaKeyRender(
    int out_format, void* out_ptr, int out_size,
    int in_format, void* in_ptr, int in_size,
    int w, int h);

// 从C++传入HLSL的参数，必须是16字节的倍数
struct ChromaParam
{
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 chroma_key;
    DirectX::XMFLOAT2 pixel_size;
    DirectX::XMFLOAT2 placeholder;
    float contrast;
    float brightness;
    float gamma;
    float similarity;
    float smoothness;
    float spill;
};

// 实际上是HLSL中VertData结构体的C++表示
#define NUM_VERTS 4
struct Vertex
{
	struct
	{
		float x, y, z, w;
	} pos;
	struct
	{
		float u, v;
	} tex;
};

static const float g_YuvMat[16] = { 0.182586f, -0.100644f, 0.439216f, 0.0f,
								0.614231f, -0.338572f, -0.398942f, 0.0f,
								0.062007f, 0.439216f, -0.040274f, 0.0f,
								0.062745f, 0.501961f, 0.501961f, 1.0f };

class ChromaKeyHelper {
public:
    ChromaKeyHelper();
    ~ChromaKeyHelper();

    bool Init();
    bool UnInit();
    void ConfigChromakey(unsigned int chroma_color, int similarity, int smoothness, int spill, 
		int opacity, int contrast, int brightness, int gamma);
	void ConfigBkImage(const char* path);

	//fnChromaKeyRender 的子方法
    bool InitTargetTexture(int width, int height);							// 初始化输出纹理
    bool LoadImage(int with, int height, int slicePitch, void * data);		// 初始化输入纹理
    bool LoadEffect();														// 配置渲染流程的各种参数
    bool UpdateImage(int with, int height, int slicePitch, void * data);	// 更新视频帧纹理子资源
    bool UpdateConfig();													// 更新参数
    bool UpdateBkImage();													// 更新绿幕背景纹理
    void Render(int out_format, void* out_ptr, int out_size, int in_format, void* in_ptr, int in_size, int w, int h);
    bool GetRenderResult(void **data);

    void SaveTextureToFile(const std::wstring &file);

private:
    ID3D11Device            * m_pD3DDevice = nullptr;				// 设备，用于创建其他类
    ID3D11DeviceContext     * m_pD3DContext = nullptr;				// 上下文，表示一组渲染操作

    ID3D11Texture2D         * m_pBackgroundTexture2D = nullptr;		// 绿幕背景纹理
    ID3D11Texture2D         * m_pImageTexture2D = nullptr;			// 视频帧纹理
    ID3D11Texture2D         * m_pRenderTexture2D = nullptr;			// 输出纹理

    ID3D11VertexShader      * m_pVertexShader = nullptr;			// 顶点着色器
    ID3D11InputLayout       * m_pVertexLayout = nullptr;			// 顶点着色器中C++变量到hlsl变量的映射
    ID3D11Buffer            * m_pVertexConstants = nullptr;			// 顶点着色器的常量缓冲区(实际的变量数据)

    ID3D11PixelShader       * m_pPixelShader = nullptr;				// 像素着色器
    ID3D11SamplerState      * m_pSamplerState = nullptr;			// 采样器状态，可以绑定到任何着色器阶段进行纹理采样
    ID3D11Buffer            * m_pPixelConstants = nullptr;			// 像素着色器的常量缓冲区(实际的变量数据)

    ID3D11BlendState        * m_pBlendState = nullptr;				// 混合状态
    ID3D11DepthStencilState * m_pZstencilState = nullptr;			// 深度模板对象
    ID3D11RasterizerState   * m_pRasterState = nullptr;				// 光栅化对象

	ID3D11ShaderResourceView * m_ppShaderResourceView[2] = {};		// 传入像素着色器的纹理数组，0是视频帧纹理，1是背景纹理
    ID3D11RenderTargetView   * m_pRenderTargetView = nullptr;		// 渲染目标，从这里取出数据
    
    bool m_bTargetInited = false;
    bool m_bImageInited = false;
    bool m_bEffectInited = false;
	bool m_bConfigUpdated = true;
	bool m_bBkImageUpdated = true;

    unsigned int m_uKeyColor = 0;				// 要抠出颜色的rgba
    float m_similarity = 0.2;					// 抠出颜色的误差允许范围
    float m_smoothness = 0.1;					// 平滑度
    float m_spill = 0.1;							
	unsigned int m_uOpacityColor = 0xFFFFFFFF;	// 透明度
    float m_contrast = 1.0;						
    float m_brightness = 0;
    float m_gamma = 1.0;

    int m_nWidth = 1;
    int m_nHeight = 1;

	TCHAR m_chBkImagePath[MAX_PATH] = {};
};