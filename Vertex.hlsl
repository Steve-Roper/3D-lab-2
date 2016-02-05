

struct VS_IN
{
	float3 Pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_OUT
{
	float4 Pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

//-----------------------------------------------------------------------------------------
// VertexShader: VSScene
//-----------------------------------------------------------------------------------------
VS_OUT VS_main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;

	output.Pos = float4(input.Pos, 1.0f);
	//output.Pos = mul(float4(input.Pos, 1), Transformation);
	output.uv = input.uv;
	//output.Color = float3( Transformation._m00, Transformation._m11, Transformation._m22 );

	return output;
}