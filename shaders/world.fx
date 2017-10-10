Texture2D txDiffuse : register(t0);
Texture2D txDepth : register(t1);
SamplerState samLinear : register(s0);
SamplerState samPoint : register(s1);

cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;
    matrix matShadowToWorld;

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
    float4 ShadowPos: TEXCOORD3;
    
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);
    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;
    Output.ShadowPos = mul(Output.Position, matShadowToWorld);

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = Input.Normal;
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float3 albedo = txDiffuse.Sample(samLinear, Input.Texcoord).xyz;
    float3 ShadowPos = Input.ShadowPos.xyz / Input.ShadowPos.w;
    ShadowPos.xy = ShadowPos.xy * 0.5f + 0.5f;
    ShadowPos.y = - ShadowPos.y;

    float shadow = txDepth.Sample(samPoint, ShadowPos) > ShadowPos.z - 0.0001;
    return float4(shadow * albedo, 1.0f);

    return float4(albedo, 1.0f);
    return float4(albedo * dot(normalize(viewPosition.xyz - Input.WorldPos), Input.Normal), 1.0f);

}
