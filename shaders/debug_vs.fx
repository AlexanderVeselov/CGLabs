cbuffer ConstantBuffer : register(b0)
{
    matrix matWorld;
    matrix matViewProjection;
    matrix matShadowToWorld;

}

struct VS_INPUT
{
    float4 Position : POSITION0;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 Tangent_S: TANGENT_S;
    float3 Tangent_T: TANGENT_T;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float3 Tangent_S: TEXCOORD2;
    float3 Tangent_T: TEXCOORD3;
    float3 WorldPos : TEXCOORD4;
    
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
    Output.Position = mul(Input.Position, matWorld);
    Output.Texcoord = Input.Texcoord;
    Output.WorldPos = Output.Position.xyz;

    Output.Position = mul(Output.Position, matViewProjection);
    Output.Normal   = Input.Normal;
    Output.Tangent_S = Input.Tangent_S;
    Output.Tangent_T = Input.Tangent_T;
    return Output;
}
