////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;

float f_AmbientStrength;
float f_DiffuseStrength;
float f_SpecularPower;

float3 v3_ObjectColour;
float3 v3_LightDir;
float3 v3_CamPos;


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
    float4 Position3D    : TEXCOORD1;
};


/////////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for a specular shaded object//
/////////////////////////////////////////////////////
SceneVertexToPixel SpecularVertexShader(float4 inPos : POSITION, float3 inNormal : NORMAL) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.Normal = normalize(mul(inNormal, (float3x3)m_World));
    Output.Position3D = mul(inPos, m_World);
    
    return Output;
}

PixelColourOut SpecularPixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float diffuseLightingFactor = saturate(dot(normalize(-v3_LightDir), PSIn.Normal))*f_DiffuseStrength;
    
    float3 eyeVector = normalize(float4(v3_CamPos, 1) - PSIn.Position3D);
    float3 reflectionVector = normalize(reflect(v3_LightDir, PSIn.Normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, eyeVector)), f_SpecularPower);

    Output.Colour = float4(v3_ObjectColour, 1)*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique SpecularLight
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 SpecularVertexShader();
        PixelShader = compile ps_2_0 SpecularPixelShader();
    }
}