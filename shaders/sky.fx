Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

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
    Output.Position = Input.Position;
    Output.Position = mul(Output.Position, matWorld);
    Output.Position.xyz += vs_viewPosition;

    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;
    Output.ShadowPos = mul(Output.Position, matShadowToWorld);

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = mul(Input.Normal, matWorld);
    Output.Tangent_S = mul(Input.Tangent_S, matWorld);
    Output.Tangent_T = mul(Input.Tangent_T, matWorld);
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float3 albedo = txDiffuse.Sample(samLinear, Input.Texcoord).xyz;
    float3 sun = pow(saturate(dot(-normalize(lightPos - viewPosition), normalize(Input.Normal))), 2048.0f);
    sun += pow(saturate(dot(-normalize(lightPos - viewPosition), normalize(Input.Normal))), 8.0f) * pow(lightColor * 0.3f, 2.0f);

    return float4(albedo + sun, 1.0f);

}
