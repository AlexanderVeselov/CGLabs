Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;
}

struct VS_INPUT
{
    float4 Position : POSITION0;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : NORMAL0;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);
    Output.Texcoord = Input.Texcoord;

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = Input.Normal;
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float4 albedo = txDiffuse.Sample(samLinear, Input.Texcoord);
    return float4(albedo.xyz, albedo.x);
    return float4(Input.Texcoord, 0.0f, 0.5f);
    //return float4(Input.WorldPos.xyz, 1.0f);
}
