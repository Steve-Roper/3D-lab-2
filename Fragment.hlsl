Texture2D Texture : register(t0);
sampler Sampler : register(s0);

struct VS_OUT
{
	float4 Pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 Normal : NORMAL;
	float4 WorldPos : POSITION;
};

float4 PS_main(VS_OUT input) : SV_Target
{
	float4 textureColor = Texture.Sample(Sampler, input.uv);


	float4 L = normalize(float4(0, 0, -3, 0) - input.WorldPos);
	float4 kd = textureColor;
	float4 diffuse = max(0, dot(L, input.Normal)) * kd; //ligth source contribution is max (ld = 1) and therefore omitted

	return diffuse;
};