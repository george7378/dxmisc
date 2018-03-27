////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;

float f_AmbientStrength;
float f_DiffuseStrength;
float f_SpecularPower;

float3 v3_LightDir;
float3 v3_CamPos;

Texture t_Texture;


//////////////////
//Sampler states//
//////////////////
sampler TextureSampler = sampler_state 
{
texture = <t_Texture>;
magfilter = LINEAR;
minfilter = LINEAR;
mipfilter = LINEAR;
AddressU = Wrap; 
AddressV = Wrap;
};


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
    float2 TexCoords     : TEXCOORD0;
    float3 Normal        : TEXCOORD1;
    float4 Position3D    : TEXCOORD2;
};


//////////////////////////////////////////////
//TECHNIQUE 1: Shaders for a textured object//
//////////////////////////////////////////////
SceneVertexToPixel TextureVertexShader(float4 inPos : POSITION, float3 inNormal : NORMAL, float2 inTexCoords : TEXCOORD0) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.TexCoords = inTexCoords;
    Output.Normal = normalize(mul(inNormal, (float3x3)m_World));
    Output.Position3D = mul(inPos, m_World);
    
    return Output;
}

PixelColourOut TexturePixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
    
    float diffuseLightingFactor = saturate(dot(normalize(-v3_LightDir), PSIn.Normal))*f_DiffuseStrength;
    
    float3 eyeVector = normalize(float4(v3_CamPos, 1) - PSIn.Position3D);
    float3 reflectionVector = normalize(reflect(v3_LightDir, PSIn.Normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, eyeVector)), f_SpecularPower);

    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique TexturedLit
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 TextureVertexShader();
        PixelShader = compile ps_2_0 TexturePixelShader();
    }
}