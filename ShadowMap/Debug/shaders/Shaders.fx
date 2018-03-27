////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;
float4x4 m_LightWorldViewProjection;

float f_AmbientStrength;
float f_DiffuseStrength;
float f_SpecularPower;
float f_ShadowBias;
float f_ShadowFarClip;

float3 v3_LightDir;
float3 v3_CamPos;

Texture t_Texture;
Texture t_ShadowMap;


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

sampler ShadowMapSampler = sampler_state 
{
texture = <t_ShadowMap>;
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

struct ShadowMapVertexToPixel
{
    float4 Position      : POSITION;
    float Depth   		 : TEXCOORD0;
};

struct SceneVertexToPixel
{
    float4 Position	   	 : POSITION;
    float2 TexCoords     : TEXCOORD0;
    float3 Normal        : TEXCOORD1;
    float4 Position3D    : TEXCOORD2;
    float4 Pos2DLight    : TEXCOORD3;
};


///////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for rendering a shadow map//
///////////////////////////////////////////////////
ShadowMapVertexToPixel ShadowMapVertexShader(float4 inPos : POSITION)
{
    ShadowMapVertexToPixel Output = (ShadowMapVertexToPixel)0;

    Output.Position = mul(inPos, m_LightWorldViewProjection);
    Output.Depth = Output.Position.z;

    return Output;
}

PixelColourOut ShadowMapPixelShader(ShadowMapVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;            

    Output.Colour = float4(PSIn.Depth, PSIn.Depth, PSIn.Depth, 1);

    return Output;
}


technique ShadowMap
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 ShadowMapVertexShader();
        PixelShader = compile ps_2_0 ShadowMapPixelShader();
    }
}


///////////////////////////////////////////////////////
//TECHNIQUE 2: Shaders for rendering a shadowed scene//
///////////////////////////////////////////////////////
SceneVertexToPixel ShadowedSceneVertexShader(float4 inPos : POSITION, float3 inNormal : NORMAL, float2 inTexCoords : TEXCOORD0) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.TexCoords = inTexCoords;
    Output.Normal = normalize(mul(inNormal, (float3x3)m_World));
    Output.Position3D = mul(inPos, m_World);
    Output.Pos2DLight = mul(inPos, m_LightWorldViewProjection);
    
    return Output;
}

PixelColourOut ShadowedScenePixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    float2 ProjectedTexCoords;
    ProjectedTexCoords[0] = PSIn.Pos2DLight.x/PSIn.Pos2DLight.w/2.0f + 0.5f;
    ProjectedTexCoords[1] = -PSIn.Pos2DLight.y/PSIn.Pos2DLight.w/2.0f + 0.5f;
    
    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
    
    float diffuseLightingFactor = saturate(dot(normalize(-v3_LightDir), PSIn.Normal))*f_DiffuseStrength;
    
    float3 eyeVector = normalize(float4(v3_CamPos, 1) - PSIn.Position3D);
    float3 reflectionVector = normalize(reflect(v3_LightDir, PSIn.Normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, eyeVector)), f_SpecularPower);
    
    if ((saturate(ProjectedTexCoords.x) == ProjectedTexCoords.x) && (saturate(ProjectedTexCoords.y) == ProjectedTexCoords.y))
     {
        if ((PSIn.Pos2DLight.z - f_ShadowBias) > tex2D(ShadowMapSampler, ProjectedTexCoords).r && (PSIn.Pos2DLight.z - f_ShadowBias) <= f_ShadowFarClip)
        {
        	diffuseLightingFactor = 0;
        	specularLightingFactor = 0;
	 	}
     }

    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique ShadowedScene
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 ShadowedSceneVertexShader();
        PixelShader = compile ps_2_0 ShadowedScenePixelShader();
    }
}