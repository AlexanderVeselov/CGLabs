Texture2D txDiffuse : register(t0);
Texture2D txDepth : register(t1);

SamplerState samLinear : register(s0);
SamplerComparisonState  samDepth : register(s1);

cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;
    matrix matShadowToWorld;
    float3 vs_viewPosition;

}

cbuffer PSConstantBuffer : register(b1)
{
    float3 lightPos;
    float3 lightColor;
    float3 viewPosition;
    float3 phongScale;
}

struct VS_INPUT
{
    float4 Position : POSITION0;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : NORMAL0;
    float3 Tangent_S: TANGENT_S0;
    float3 Tangent_T: TANGENT_T0;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float3 Tangent_S: TEXCOORD2;
    float3 Tangent_T: TEXCOORD3;
    float3 WorldPos : TEXCOORD4;
    float4 ShadowPos: TEXCOORD5;
    
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);

    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;
    Output.ShadowPos = mul(Output.Position, matShadowToWorld);

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = mul(Input.Normal, matWorld);
    Output.Tangent_S = mul(Input.Tangent_S, matWorld);
    Output.Tangent_T = mul(Input.Tangent_T, matWorld);
    return Output;
}

float SampleShadow(float3 coord)
{
    float shadow = txDepth.SampleCmpLevelZero(samDepth, coord.xy, coord.z - 0.0025).xxx;
    if (coord.x > 1.0f || coord.x < 0.0f) shadow = 1.0f;
    if (coord.y > 1.0f || coord.y < 0.0f) shadow = 1.0f;
    return shadow;
}

float PCF(float3 coord)
{
    float shadow = 0.0f;
    for (int i = -2; i <= 2; ++i)
    for (int j = -2; j <= 2; ++j)
    {
        shadow += SampleShadow(float3(coord.xy + float2(i, j) / 2048.0f, coord.z));
    }
    return shadow / 25.0f;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float4 albedo = txDiffuse.Sample(samLinear, Input.Texcoord);
    float3 normal = Input.Normal;

    float3 ambient = float3(0.3, 0.5, 0.8) * 0.5f;

    float3 ShadowPos;    
    ShadowPos.x =  Input.ShadowPos.x / Input.ShadowPos.w * 0.5f + 0.5f;
    ShadowPos.y = -Input.ShadowPos.y / Input.ShadowPos.w * 0.5f + 0.5f;
    ShadowPos.z =  Input.ShadowPos.z / Input.ShadowPos.w;
    float3 shadow = PCF(ShadowPos);
    
    float fog = saturate(distance(viewPosition, Input.WorldPos) / 512.0f);
    float3 fogColor = ambient + lightColor * 0.25f;

    float3 result = albedo.rgb * (ambient + lightColor * shadow);
    result = lerp(result, fogColor, fog);
    return float4(result, albedo.a * 2.0f * Input.Normal.r);

}
