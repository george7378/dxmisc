#ifndef CORE_H
#define CORE_H

//Resource definitions
#define IDI_MAINICON		1000

//Matrices
D3DXMATRIX matCamProjection;
D3DXMATRIX matCamView;
D3DXMATRIX matWaterReflect;
D3DXMATRIX matIdentity;

//D3D and windows
unsigned WIDTH = 0;
unsigned HEIGHT = 0;
unsigned REFRESH_RATE = 0;
HWND hwnd = NULL;
LPDIRECT3D9 d3d = NULL;
LPDIRECT3DDEVICE9 d3ddev = NULL;
D3DPRESENT_PARAMETERS d3dpp;

//Effects
LPD3DXEFFECT globalLightingEffect = NULL;

//Sound
irrklang::ISoundEngine* soundEngine = NULL;
irrklang::ISound *buzz = NULL;

//Water
IDirect3DTexture9 *waterNormalMap1 = NULL;
IDirect3DTexture9 *waterNormalMap2 = NULL;
D3DVIEWPORT9 waterMapViewport;

//Post-processing
LPD3DXSPRITE screenSprite;

template <class T> void SAFE_RELEASE(T **ppT)
{
    if (*ppT)
	{
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif
