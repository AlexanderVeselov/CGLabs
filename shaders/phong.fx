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
    
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);
    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = Input.Normal;
    return Output;
}

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
    float3 normal = normalize(Input.Normal);
    float fresnel = pow(1.0 + dot(normalize(Input.WorldPos - viewPosition.xyz), normal), 4);
    float3 diffuse = 0.0f;
    float3 specular = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        float3 lightPos = normalize(lightPositions[i].xyz - Input.WorldPos);
        float dist = distance(Input.WorldPos, lightPositions[i]);
        diffuse += 1.0 / (1.0f + 0.01 *dist*dist) * max(dot(lightPos, normal) * lightColors[i].xyz, 0.0f) * 4.0f;

        float3 reflectVec = reflect(-normalize(viewPosition.xyz), normal);
        specular += pow(saturate(dot(reflectVec, lightPos)), 64) * lightColors[i].xyz * 2.0f;
    }
    float3 albedo = txDiffuse.Sample(samLinear, Input.Texcoord).xyz;
    float3 ambient = float3(0.0f, 0.2f, 0.4f);

    return float4(albedo * (diffuse + ambient) + specular * phongScale.x + fresnel * ambient, 0.6f);

}
