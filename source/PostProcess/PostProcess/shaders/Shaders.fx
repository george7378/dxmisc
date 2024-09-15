////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;
float4x4 m_LightWorldViewProjection;

int i_TextureWidth;
int i_TextureHeight;

float f_AmbientStrength;
float f_DiffuseStrength;
float f_SpecularPower;
float f_ShadowBias;
float f_ShadowFarClip;

float3 v3_LightDir;
float3 v3_CamPos;

Texture t_Texture;
Texture t_ShadowMap;
Texture t_ScreenTexture;


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

sampler ScreenTextureSampler = sampler_state
{
texture = <t_ScreenTexture>;
magfilter = POINT;
minfilter = POINT; 
mipfilter = POINT; 
AddressU = Mirror; 
AddressV = Mirror;
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


////////////////////////////////
//TECHNIQUE 3: Post-processing//
////////////////////////////////
float BlurWeights[13] =
{
	0.00903789f,
	0.0217894f,
	0.044765f,
	0.0783688f,
	0.116912f,
	0.148625f,
	0.161003f,
	0.148625f,
	0.116912f,
	0.0783688f,
	0.044765f,
	0.0217894f,
	0.00903789f,
};

PixelColourOut ScreenPixelShader(float2 inTexCoords : TEXCOORD0)
{
	PixelColourOut Output = (PixelColourOut)0;
	
    float4 baseColour = tex2D(ScreenTextureSampler, inTexCoords);
    
    Output.Colour = baseColour;
    
    return Output;
}

PixelColourOut GreyscalePixelShader(float2 inTexCoords : TEXCOORD0)
{
	PixelColourOut Output = (PixelColourOut)0;
	
    float4 baseColour = tex2D(ScreenTextureSampler, inTexCoords);
    
    float average = (baseColour.r + baseColour.g + baseColour.b)/3;
    Output.Colour = float4(average, average, average, baseColour.a);
    
    return Output;
}

PixelColourOut EmbossPixelShader(float2 inTexCoords : TEXCOORD0)
{
	PixelColourOut Output = (PixelColourOut)0;
	
    float4 baseColour = tex2D(ScreenTextureSampler, inTexCoords);
    
	baseColour -= tex2D(ScreenTextureSampler, inTexCoords.xy - 0.003f)*2.7f;
	baseColour += tex2D(ScreenTextureSampler, inTexCoords.xy + 0.003f)*2.7f;
	float average = (baseColour.r + baseColour.g + baseColour.b)/3;
	Output.Colour = float4(average, average, average, baseColour.a);
    
    return Output;
}

PixelColourOut BlurHPixelShader(float2 inTexCoords : TEXCOORD0)
{
	PixelColourOut Output = (PixelColourOut)0;
	
	float pixelWidth = 1.0f/i_TextureWidth;
	float4 colour = 0;
	float2 blurCoordinates = inTexCoords;
	int kernel = -6;
	
	for (int i = 0; i < 13; i++) 
    {
    	blurCoordinates.x = inTexCoords.x + kernel*pixelWidth;
        colour += tex2D(ScreenTextureSampler, blurCoordinates)*BlurWeights[i];
        kernel += 1;
    }

    Output.Colour = colour;
    
    return Output;
}

PixelColourOut BlurVPixelShader(float2 inTexCoords : TEXCOORD0)
{
	PixelColourOut Output = (PixelColourOut)0;
	
	float pixelHeight = 1.0f/i_TextureHeight;
	float4 colour = 0;
	float2 blurCoordinates = inTexCoords;
	int kernel = -6;
	
	for (int i = 0; i < 13; i++) 
    {
    	blurCoordinates.y = inTexCoords.y + kernel*pixelHeight;
        colour += tex2D(ScreenTextureSampler, blurCoordinates)*BlurWeights[i];
        kernel += 1;
    }

    Output.Colour = colour;
    
    return Output;
}


technique None
{
    pass Pass0
    {
        PixelShader = compile ps_2_0 ScreenPixelShader();
    }
}

technique Greyscale
{
    pass Pass0
    {
        PixelShader = compile ps_2_0 GreyscalePixelShader();
    }
}

technique Emboss
{
    pass Pass0
    {
        PixelShader = compile ps_2_0 EmbossPixelShader();
    }
}

technique BlurH
{
    pass Pass0
    {
        PixelShader = compile ps_2_0 BlurHPixelShader();
    }
}

technique BlurV
{
    pass Pass0
    {
        PixelShader = compile ps_2_0 BlurVPixelShader();
    }
}