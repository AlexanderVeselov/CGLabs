Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;

}

cbuffer PSConstantBuffer : register(b1)
{
    float4 lightPositions[3];
    float4 lightColors[3];
    float4 viewPosition;
    float4 phongScale;
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
    float3 WorldPos : TEXCOORD2;
    float4 ProjPos  : TEXCOORD3;
    
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);
    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;

    Output.Position = mul(Output.Position, matViewProjection);
    Output.ProjPos  = Output.Position;
    Output.Normal   = Input.Normal;
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float depth = Input.ProjPos.z / Input.ProjPos.w;
    return float4(depth.xxx, 1.0f);

}
