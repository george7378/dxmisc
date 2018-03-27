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
Texture t_NormalMap;


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

sampler NormalMapSampler = sampler_state 
{
texture = <t_NormalMap>;
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
    float4 Position3D    : TEXCOORD1;
    float3 lightDirTS	 : TEXCOORD2;
    float3 viewDirTS	 : TEXCOORD3;
};


///////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for a normal mapped object//
///////////////////////////////////////////////////
SceneVertexToPixel NMVertexShader(float4 inPos : POSITION, float2 inTexCoords : TEXCOORD0, float3 inNormal : NORMAL, float3 inTangent : TANGENT) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.TexCoords = inTexCoords;
    
    float3x3 worldToTangentSpace;
    worldToTangentSpace[0] = mul(inTangent, (float3x3)m_World);
    worldToTangentSpace[1] = mul(cross(inTangent, inNormal), (float3x3)m_World);
    worldToTangentSpace[2] = mul(inNormal, (float3x3)m_World);
    
    Output.lightDirTS = mul(worldToTangentSpace, float4(v3_LightDir, 1));
    Output.viewDirTS = mul(worldToTangentSpace, float4(v3_CamPos, 1) - mul(inPos, m_World));
    
    Output.Position3D = mul(inPos, m_World);
    
    return Output;
}

PixelColourOut NMPixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
    float3 normal = normalize(2*tex2D(NormalMapSampler, PSIn.TexCoords).xyz - 1);
    
    float diffuseLightingFactor = saturate(dot(normalize(-PSIn.lightDirTS), normal))*f_DiffuseStrength;

    float3 reflectionVector = normalize(reflect(PSIn.lightDirTS, normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, normalize(PSIn.viewDirTS))), f_SpecularPower);

    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique NormalMap
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 NMVertexShader();
        PixelShader = compile ps_2_0 NMPixelShader();
    }
}