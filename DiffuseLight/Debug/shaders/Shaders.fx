////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;

float f_AmbientStrength;
float f_DiffuseStrength;

float3 v3_ObjectColour;
float3 v3_LightDir;


//////////////////
//I/O structures//
//////////////////
struct PixelColourOut
{
    float4 Colour        : COLOR0;
};

struct SceneVertexToPixel
{
    float4 Position	   	 : POSITION;
    float3 Normal        : TEXCOORD0;
};


////////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for a diffuse shaded object//
////////////////////////////////////////////////////
SceneVertexToPixel DiffuseVertexShader(float4 inPos : POSITION, float3 inNormal : NORMAL) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.Normal = normalize(mul(inNormal, (float3x3)m_World));
    
    return Output;
}

PixelColourOut DiffusePixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float diffuseLightingFactor = saturate(dot(normalize(-v3_LightDir), PSIn.Normal))*f_DiffuseStrength;

    Output.Colour = float4(v3_ObjectColour, 1)*(diffuseLightingFactor + f_AmbientStrength);

    return Output;
}


technique DiffuseLight
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 DiffuseVertexShader();
        PixelShader = compile ps_2_0 DiffusePixelShader();
    }
}