#include "ChromaKey.hlsli"
uniform Texture2D image[2];	// 从PSSetShaderResources函数传来的变量

// 接受从C++传入的参数
cbuffer ChromaParam : register(b0) {
    float4 color;
    float2 chroma_key;
    float2 pixel_size;
    float2 placehold;
    float contrast;
    float brightness;
    float gamma;
    float similarity;
    float smoothness;
    float spill;
};

SamplerState textureSampler {	// 声明一个采样器
    Filter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

float4 CalcColor(float4 rgba)
{
    return float4(pow(rgba.rgb, float3(gamma, gamma, gamma)) * contrast + brightness, rgba.a);
}

float GetChromaDist(float3 rgb)	// 计算出rgb与chroma_key的距离
{
	float4x4 yuv_mat = { 0.182586, 0.614231, 0.062007, 0.062745,
							-0.100644, -0.338572, 0.439216, 0.501961,
							0.439216, -0.398942, -0.040274, 0.501961,
							0.000000, 0.000000, 0.000000, 1.000000 };
    float4 yuvx = mul(float4(rgb.rgb, 1.0), yuv_mat);	// rgba转yuv
    return distance(chroma_key, yuvx.yz);				// 比较uv判断
}

float4 SampleTexture(float2 uv)
{
    return image[0].Sample(textureSampler, uv);
}

float GetBoxFilteredChromaDist(float3 rgb, float2 texCoord)
{
    float2 h_pixel_size = pixel_size / 2.0;
    float2 point_0 = float2(pixel_size.x, h_pixel_size.y);
    float2 point_1 = float2(h_pixel_size.x, -pixel_size.y);
    float distVal = GetChromaDist(SampleTexture(texCoord - point_0).rgb);
    distVal += GetChromaDist(SampleTexture(texCoord + point_0).rgb);
    distVal += GetChromaDist(SampleTexture(texCoord - point_1).rgb);
    distVal += GetChromaDist(SampleTexture(texCoord + point_1).rgb);
    distVal *= 2.0;
    distVal += GetChromaDist(rgb);
    return distVal / 9.0;
}

float4 ProcessChromaKey(float4 rgba, VertData v_in)
{
    float chromaDist = GetBoxFilteredChromaDist(rgba.rgb, v_in.uv);
    float baseMask = chromaDist - similarity;
    float fullMask = pow(saturate(baseMask / smoothness), 1.5);
    float spillVal = pow(saturate(baseMask / spill), 1.5);

    rgba.rgba *= color;		// 透明度
    rgba.a *= fullMask;

    float desat = (rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722);
    rgba.rgb = saturate(float3(desat, desat, desat)) * (1.0 - spillVal) + rgba.rgb * spillVal;

    return CalcColor(rgba);
}

float4 main(VertData v_in) : SV_Target
{
    float4 rgba = image[0].Sample(textureSampler, v_in.uv);
	float4 afterrgba = ProcessChromaKey(rgba, v_in);
	float4 bkrgba = image[1].Sample(textureSampler, v_in.uv);
	return afterrgba.a * afterrgba + (1 - afterrgba.a) * bkrgba;
}