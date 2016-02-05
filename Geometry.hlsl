cbuffer ConstantBuffer
{
	matrix World;
	matrix Transformation;
};

struct GS_IN
{
	float4 Pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

struct GS_OUT
{
	float4 Pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 Normal : NORMAL;
	float4 WorldPos : POSITION;
};

[maxvertexcount(6)]
void GS_main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> outputStream)
{
	GS_OUT output = (GS_OUT)0;

	float3 vector1 = input[1].Pos.xyz - input[0].Pos.xyz;
	float3 vector2 = input[2].Pos.xyz - input[0].Pos.xyz;
	float4 normal = float4(normalize(cross(vector1, vector2)), 1.0f);
	float4 actualNormal = mul(World, normal);

	for (uint i = 0; i < 2; ++i)
	{
		for (uint j = 0; j < 3; ++j)
		{
			output.Pos = mul(input[j].Pos + i * normal, Transformation);
			output.uv = input[j].uv;
			output.Normal = actualNormal;
			output.WorldPos = mul(output.Pos, World);

			outputStream.Append(output);
		}

		outputStream.RestartStrip();
	}
}