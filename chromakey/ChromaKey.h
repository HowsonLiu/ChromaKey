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

// ��C++����HLSL�Ĳ�����������16�ֽڵı���
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

// ʵ������HLSL��VertData�ṹ���C++��ʾ
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

	//fnChromaKeyRender ���ӷ���
    bool InitTargetTexture(int width, int height);							// ��ʼ���������
    bool LoadImage(int with, int height, int slicePitch, void * data);		// ��ʼ����������
    bool LoadEffect();														// ������Ⱦ���̵ĸ��ֲ���
    bool UpdateImage(int with, int height, int slicePitch, void * data);	// ������Ƶ֡��������Դ
    bool UpdateConfig();													// ���²���
    bool UpdateBkImage();													// ������Ļ��������
    void Render(int out_format, void* out_ptr, int out_size, int in_format, void* in_ptr, int in_size, int w, int h);
    bool GetRenderResult(void **data);

    void SaveTextureToFile(const std::wstring &file);

private:
    ID3D11Device            * m_pD3DDevice = nullptr;				// �豸�����ڴ���������
    ID3D11DeviceContext     * m_pD3DContext = nullptr;				// �����ģ���ʾһ����Ⱦ����

    ID3D11Texture2D         * m_pBackgroundTexture2D = nullptr;		// ��Ļ��������
    ID3D11Texture2D         * m_pImageTexture2D = nullptr;			// ��Ƶ֡����
    ID3D11Texture2D         * m_pRenderTexture2D = nullptr;			// �������

    ID3D11VertexShader      * m_pVertexShader = nullptr;			// ������ɫ��
    ID3D11InputLayout       * m_pVertexLayout = nullptr;			// ������ɫ����C++������hlsl������ӳ��
    ID3D11Buffer            * m_pVertexConstants = nullptr;			// ������ɫ���ĳ���������(ʵ�ʵı�������)

    ID3D11PixelShader       * m_pPixelShader = nullptr;				// ������ɫ��
    ID3D11SamplerState      * m_pSamplerState = nullptr;			// ������״̬�����԰󶨵��κ���ɫ���׶ν����������
    ID3D11Buffer            * m_pPixelConstants = nullptr;			// ������ɫ���ĳ���������(ʵ�ʵı�������)

    ID3D11BlendState        * m_pBlendState = nullptr;				// ���״̬
    ID3D11DepthStencilState * m_pZstencilState = nullptr;			// ���ģ�����
    ID3D11RasterizerState   * m_pRasterState = nullptr;				// ��դ������

	ID3D11ShaderResourceView * m_ppShaderResourceView[2] = {};		// ����������ɫ�����������飬0����Ƶ֡����1�Ǳ�������
    ID3D11RenderTargetView   * m_pRenderTargetView = nullptr;		// ��ȾĿ�꣬������ȡ������
    
    bool m_bTargetInited = false;
    bool m_bImageInited = false;
    bool m_bEffectInited = false;
	bool m_bConfigUpdated = true;
	bool m_bBkImageUpdated = true;

    unsigned int m_uKeyColor = 0;				// Ҫ�ٳ���ɫ��rgba
    float m_similarity = 0.2;					// �ٳ���ɫ���������Χ
    float m_smoothness = 0.1;					// ƽ����
    float m_spill = 0.1;							
	unsigned int m_uOpacityColor = 0xFFFFFFFF;	// ͸����
    float m_contrast = 1.0;						
    float m_brightness = 0;
    float m_gamma = 1.0;

    int m_nWidth = 1;
    int m_nHeight = 1;

	TCHAR m_chBkImagePath[MAX_PATH] = {};
};