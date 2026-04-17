struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 PS(PS_INPUT input) : SV_Target
{
    return input.col;
}