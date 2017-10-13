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

float4 ps_main(VS_OUTPUT Input) : SV_Target
{
	return float4(1.0f, 0.0f, 1.0f, 1.0f);

}
