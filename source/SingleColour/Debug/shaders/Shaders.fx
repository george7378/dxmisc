////////////////////
//Global variables//
////////////////////
float4x4 m_WorldViewProjection;

float3 v3_ObjectColour;


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
};


/////////////////////////////////////////////////////
//TECHNIQUE 1: Shaders for a single coloured object//
/////////////////////////////////////////////////////
SceneVertexToPixel SingleColourVertexShader(float4 inPos : POSITION) 
{
    SceneVertexToPixel Output = (SceneVertexToPixel)0;

    Output.Position = mul(inPos, m_WorldViewProjection);
    
    return Output;
}

PixelColourOut SingleColourPixelShader(SceneVertexToPixel PSIn)
{
    PixelColourOut Output = (PixelColourOut)0;

    Output.Colour = float4(v3_ObjectColour, 1);

    return Output;
}


technique SingleColour
{
    pass Pass0
    {
        VertexShader = compile vs_2_0 SingleColourVertexShader();
        PixelShader = compile ps_2_0 SingleColourPixelShader();
    }
}