#ifndef CORE_H
#define CORE_H

//Matrices
D3DXMATRIX matCamProjection;
D3DXMATRIX matCamView;
D3DXMATRIX matLightProjection;
D3DXMATRIX matLightView;

//D3D and Windows
unsigned WIDTH = 640;
unsigned HEIGHT = 480;
HWND hwnd = NULL;					
LPDIRECT3D9 d3d = NULL;				
LPDIRECT3DDEVICE9 d3ddev = NULL;	
D3DPRESENT_PARAMETERS d3dpp;	

//Effects
LPD3DXEFFECT globalLightingEffect = NULL;

//Shadow settings
unsigned SHADOW_MAP_SIZE = 1024;
float shadowMapFarClip = 20;
float shadowMapBias = 0.01f;
D3DVIEWPORT9 shadowViewport;

//Camera and lighting
D3DXVECTOR3 camPos(-6, 2, 0);
D3DXVECTOR3 lightDir(0, -1, 1);
float lightAmbient = 0.1f;
float lightDiffuse = 1.2f;

//Post-processing
LPD3DXSPRITE screenSprite;

//Object properties
D3DXVECTOR3 torusRot(0, 0, 0);

template <class T> void SAFE_RELEASE(T **ppT) //Release and nullify pointers
{
    if (*ppT)
	{
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif