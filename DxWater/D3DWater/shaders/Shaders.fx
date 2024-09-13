// <effectEd direct3D="9" profile="fx_2_0" shaderFlags="None, WarningsAreErrors" />

////////////////////
//Global variables//
////////////////////
float4x4 m_World;
float4x4 m_WorldViewProjection;

int i_TextureWidth;
int i_TextureHeight;

float f_AmbientStrength;
float f_LightPower;
float f_LightRange;
float f_Reflectivity;
float f_SpecularPower;
float f_RefractiveIndexScale;

float2 v2_TexCoordDeltaFirst;
float2 v2_TexCoordDeltaSecond;

float3 v3_LightPos;
float3 v3_CamPos;

float4 v4_ClipPlane;

Texture t_Texture;
Texture t_TextureN;
Texture t_WaterNormalMap1;
Texture t_WaterNormalMap2;
Texture t_RefractionMap;
Texture t_ReflectionMap;
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

sampler NormalMapSampler = sampler_state 
{
	texture = <t_TextureN>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Wrap; 
	AddressV = Wrap;
};

sampler WaterNormalMap1Sampler = sampler_state
{
texture = <t_WaterNormalMap1>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Mirror;
	AddressV = Mirror;
};

sampler WaterNormalMap2Sampler = sampler_state
{
	texture = <t_WaterNormalMap2>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Mirror;
	AddressV = Mirror;
};

sampler RefractionMapSampler = sampler_state
{
	texture = <t_RefractionMap>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Mirror;
	AddressV = Mirror;
};

sampler ReflectionMapSampler = sampler_state
{
	texture = <t_ReflectionMap>;
	magfilter = LINEAR;
	minfilter = LINEAR;
	mipfilter = LINEAR;
	AddressU = Mirror;
	AddressV = Mirror;
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
    float4 Colour     	   : COLOR0;
};

struct WaterVertexToPixel
{
    float4 Position	   	   : POSITION;
    float2 TexCoords       : TEXCOORD0;
    float3 LightDirTS	   : TEXCOORD1;
    float3 ViewDirTS	   : TEXCOORD2;
    float4 SamplingPos     : TEXCOORD3;
    float4 Position3D      : TEXCOORD4;
};

struct SceneVertexToPixel
{
    float4 Position	   	   : POSITION;
    float2 TexCoords       : TEXCOORD0;
    float3 LightDirTS      : TEXCOORD1;
    float3 ViewDirTS       : TEXCOORD2;
    float4 Position3D      : TEXCOORD3;
};

struct LightVertexToPixel
{
    float4 Position	   	   : POSITION;
    float4 Position3D      : TEXCOORD0;
};


////////////////////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for drawing reflective/refractive water//
////////////////////////////////////////////////////////////////
WaterVertexToPixel WaterVertexShader(float4 inPos : POSITION, float2 inTexCoords : TEXCOORD0, float3 inNormal : NORMAL, float3 inTangent : TANGENT, float3 inBinormal : BINORMAL) 
{
    WaterVertexToPixel Output = (WaterVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
	Output.TexCoords = inTexCoords;

    float3x3 worldToTangentSpace;
    worldToTangentSpace[0] = mul(inTangent, (float3x3)m_World);
    worldToTangentSpace[1] = mul(inBinormal, (float3x3)m_World);
    worldToTangentSpace[2] = mul(inNormal, (float3x3)m_World);

	Output.Position3D = mul(inPos, m_World);
    Output.SamplingPos = Output.Position;

	Output.LightDirTS = mul(worldToTangentSpace, Output.Position3D.xyz - v3_LightPos);
    Output.ViewDirTS = mul(worldToTangentSpace, v3_CamPos - Output.Position3D.xyz);

    return Output;
}

PixelColourOut WaterPixelShader(WaterVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;

    float3 normalFirst = 2*tex2D(WaterNormalMap1Sampler, PSIn.TexCoords + v2_TexCoordDeltaFirst).xyz - 1;
    float3 normalSecond = 2*tex2D(WaterNormalMap2Sampler, PSIn.TexCoords + v2_TexCoordDeltaSecond).xyz - 1;
 
    float3 normal = normalize(0.5f*(normalFirst + normalSecond));

    float2 projTexCoord;
    projTexCoord.x = PSIn.SamplingPos.x/PSIn.SamplingPos.w/2 + 0.5f;
    projTexCoord.y = -PSIn.SamplingPos.y/PSIn.SamplingPos.w/2 + 0.5f;
    projTexCoord += (normal.xy*f_RefractiveIndexScale);
    
    float4 refractionColor = tex2D(RefractionMapSampler, projTexCoord);
    float4 reflectionColor = tex2D(ReflectionMapSampler, projTexCoord);

	float attenuation = 1 - saturate(length(PSIn.LightDirTS)/f_LightRange);

    float3 reflectionVector = normalize(reflect(PSIn.LightDirTS, normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, normalize(PSIn.ViewDirTS))), f_SpecularPower)*f_Reflectivity*f_LightPower*attenuation;
    
    float3 eyeVector = normalize(v3_CamPos - PSIn.Position3D);
    float fresnelTerm = 1/pow(1 + dot(eyeVector, float3(0, 1, 0)), 5);
    float4 waterColourFactor = lerp(refractionColor, reflectionColor, fresnelTerm);

    Output.Colour = waterColourFactor + float4(float3(1, 1, 1)*specularLightingFactor, 1);

    return Output;
}


technique Water
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 WaterVertexShader();
        PixelShader = compile ps_2_0 WaterPixelShader();
    }
}


////////////////////////////////////////////////////////////////////
//TECHNIQUE 2: Shaders for drawing an object with general lighting//
////////////////////////////////////////////////////////////////////
SceneVertexToPixel GeneralLightingVertexShader(float4 inPos : POSITION, float2 inTexCoords : TEXCOORD0, float3 inNormal : NORMAL, float3 inTangent : TANGENT, float3 inBinormal : BINORMAL)
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
	Output.TexCoords = inTexCoords;
	
    float3x3 worldToTangentSpace;
    worldToTangentSpace[0] = mul(inTangent, (float3x3)m_World);
    worldToTangentSpace[1] = mul(inBinormal, (float3x3)m_World);
    worldToTangentSpace[2] = mul(inNormal, (float3x3)m_World);
    
    Output.Position3D = mul(inPos, m_World);
    
    Output.LightDirTS = mul(worldToTangentSpace, Output.Position3D.xyz - v3_LightPos);
    Output.ViewDirTS = mul(worldToTangentSpace, v3_CamPos - Output.Position3D.xyz);

    return Output;
}

PixelColourOut GeneralLightingPixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;

    clip(dot(PSIn.Position3D, v4_ClipPlane.xyz) + v4_ClipPlane.w);

    float4 baseColour = tex2D(TextureSampler, PSIn.TexCoords);
	float3 normal = normalize(2*tex2D(NormalMapSampler, PSIn.TexCoords).xyz - 1);
	
	float attenuation = 1 - saturate(length(PSIn.LightDirTS)/f_LightRange);

    float diffuseLightingFactor = saturate(dot(normalize(-PSIn.LightDirTS), normal))*f_LightPower*attenuation;

    float3 reflectionVector = normalize(reflect(PSIn.LightDirTS, normal));
    float specularLightingFactor = pow(saturate(dot(reflectionVector, normalize(PSIn.ViewDirTS))), f_SpecularPower)*f_Reflectivity*f_LightPower*attenuation;

    Output.Colour = baseColour*(diffuseLightingFactor + f_AmbientStrength) + float4(float3(1, 1, 1)*specularLightingFactor, 1.0f);

    return Output;
}


technique SceneObject
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 GeneralLightingVertexShader();
        PixelShader = compile ps_2_0 GeneralLightingPixelShader();
    }
}

technique SceneObjectReflect
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 GeneralLightingVertexShader();
        PixelShader = compile ps_2_0 GeneralLightingPixelShader();
        CullMode = CW;
    }
}


//////////////////////////////////////////////////////
//TECHNIQUE 3: Shaders for drawing an emissive light//
//////////////////////////////////////////////////////
LightVertexToPixel LightVertexShader(float4 inPos : POSITION)
{
    LightVertexToPixel Output = (LightVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    Output.Position3D = mul(inPos, m_World);

    return Output;
}

PixelColourOut LightPixelShader(LightVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;
    
    clip(dot(PSIn.Position3D, v4_ClipPlane.xyz) + v4_ClipPlane.w);

    Output.Colour = float4(float3(1, 1, 1)*f_LightPower, 1);

    return Output;
}


technique Light
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 LightVertexShader();
        PixelShader = compile ps_2_0 LightPixelShader();
    }
}


////////////////////////////////
//TECHNIQUE 4: Post-processing//
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
