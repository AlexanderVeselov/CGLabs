struct PS_INPUT
{
    float2 Texcoord : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float3 Tangent_S: TEXCOORD2;
    float3 Tangent_T: TEXCOORD3;
    float3 WorldPos : TEXCOORD4;
    
};

float4 ps_main(PS_INPUT Input) : SV_Target
{
	return float4(normalize(Input.Normal) * 0.5f + 0.5f, 1.0f);
}
