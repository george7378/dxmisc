#ifndef CORE_H
#define CORE_H

//D3D and Windows
unsigned WIDTH = 640;
unsigned HEIGHT = 480;
HWND hwnd = NULL;					
LPDIRECT3D9 d3d = NULL;				
LPDIRECT3DDEVICE9 d3ddev = NULL;		
D3DPRESENT_PARAMETERS d3dpp;			

template <class T> void SAFE_RELEASE(T **ppT) //Release and nullify pointers
{
    if (*ppT)
	{
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif