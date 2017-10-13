Texture2D txDiffuse : register(t0);
Texture2D txDepth : register(t1);
Texture2D txNormal : register(t2);
SamplerState samLinear : register(s0);
SamplerComparisonState  samDepth : register(s1);

cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;
    matrix matShadowToWorld;

}

cbuffer PSConstantBuffer : register(b1)
{
    float3 lightPositions[3];
    float3 lightColors[3];
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
    Output.Normal   = Input.Normal;
    Output.Tangent_S = Input.Tangent_S;
    Output.Tangent_T = Input.Tangent_T;
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float3 albedo = txDiffuse.Sample(samLinear, Input.Texcoord).xyz;
    
    float3 ShadowPos;    
    ShadowPos.x =  Input.ShadowPos.x / Input.ShadowPos.w * 0.5f + 0.5f;
    ShadowPos.y = -Input.ShadowPos.y / Input.ShadowPos.w * 0.5f + 0.5f;
    ShadowPos.z =  Input.ShadowPos.z / Input.ShadowPos.w;

    float3x3 matTangentToWorld = transpose(float3x3(Input.Tangent_S, Input.Tangent_T, Input.Normal));
    float3 normal = txNormal.Sample(samLinear, Input.Texcoord).xyz * 2.0f - 1.0f;
    normal = normalize(mul(matTangentToWorld, normal));

    float3 shadow = txDepth.SampleCmpLevelZero(samDepth, ShadowPos.xy, ShadowPos.z - 0.001).xxx;
    if (ShadowPos.x > 1.0f || ShadowPos.x < 0.0f) shadow = 1.0f;
    if (ShadowPos.y > 1.0f || ShadowPos.y < 0.0f) shadow = 1.0f;
    float diffuse = dot(normalize(lightPositions[0].xyz), normal);
    float3 ambient = float3(0.3, 0.5, 0.8) * 0.5f;

    float phong = pow(saturate(dot(reflect(-normalize(viewPosition - Input.WorldPos), normal), normalize(lightPositions[0]))), 64.0f);

    return float4(albedo * (ambient + diffuse * shadow) + phong * shadow, 1.0f);

}
