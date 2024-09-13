//#define D3D_DEBUG_INFO
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <d3d9.h>
#include <d3dx9.h>
#include <irrKlang.h>

using namespace std;

#include "dx_core.h"
#include "dx_classes.h"
#include "gamecontrol.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "irrKlang.lib")

DrawableTex2D postProcessTex1;
DrawableTex2D postProcessTex2;
DrawableTex2D waterReflectionMap;
DrawableTex2D waterRefractionMap;

MeshObject meshWaterPlane("models/WaterPlane.x");
MeshObject meshLight("models/Light.x");
MeshObjectTN meshRoom("models/Room.x");

//Misc.
D3DXVECTOR3 vectorWallCentre_2D(D3DXVECTOR2 p1, D3DXVECTOR2 p2, D3DXVECTOR2 pos)
{
	D3DXVECTOR2 line = p2 - p1;
	float t = D3DXVec2Dot(&(pos - p1), &line)/D3DXVec2Dot(&line, &line);

	t = t < 0 ? 0 : t > 1 ? 1 : t;

	return D3DXVECTOR3(pos.x - (p1.x + t*line.x), 0, pos.y - (p1.y + t*line.y));
}
D3DXVECTOR3 handle_Collisions_Linear()
{
	D3DXVECTOR3 newPos = D3DXVECTOR3(0, 0, 0);
	D3DXVECTOR3 cumulativeCampos;

	//collision_lines_room
	for (unsigned i = 0; i < 8; i++)
	{
		cumulativeCampos = camPos + newPos;
		D3DXVECTOR3 vectorWallCentre = vectorWallCentre_2D(D3DXVECTOR2(collision_lines_room[i].x, collision_lines_room[i].y) , D3DXVECTOR2(collision_lines_room[i].z, collision_lines_room[i].w), D3DXVECTOR2(cumulativeCampos.x, cumulativeCampos.z));
		float distToWall = D3DXVec3Length(&vectorWallCentre);

		if (distToWall < wallThickness)
		{
			newPos += ((wallThickness/distToWall) - 1)*vectorWallCentre;
		}
	}

	return newPos;
}
void handle_Interactivity()
{
	if (!lightGrabbed)
	{
		if (D3DXVec3Length(&(camPos - lightPos)) < 3)
		{
			lightGrabbed = true;
			soundEngine->play2D("sound/grab.wav");
		}
	}

	else 
	{
		lightGrabbed = FALSE;
		soundEngine->play2D("sound/grab.wav");
	}
}
void sound_Footstep(bool first)
{
	if (first)
	{
		soundEngine->play2D("sound/step1.wav");
	}
	else
	{
		soundEngine->play2D("sound/step2.wav");
	}
}
template <class T> T lerp(T x, T y, T s)
{
	T result = x + s*(y - x);

	return result > y ? y : result < x ? x : result;
}

//Initialisation
void initProgram()
{
	D3DXMatrixPerspectiveFovLH(&matCamProjection, D3DXToRadian(65),	float(WIDTH)/float(HEIGHT), 0.1f, 100);
	D3DXMatrixIdentity(&matIdentity);

	SetCursorPos(unsigned(WIDTH/2), unsigned(HEIGHT/2));
	ShowCursor(FALSE);

	buzz = soundEngine->play3D("sound/buzz.wav", irrklang::vec3df(0, 0, 0), true, false, true);
}
bool initResources()
{
	//Mesh/independent texture initialisation
	if (!meshRoom.Load()){MessageBox(NULL, "Unable to load 'Room.x'", "Error", MB_OK); return false;}
		if (!meshRoom.ComputeTangentSpace()){MessageBox(NULL, "Unable to compute tangent space for 'Room.x'", "Error", MB_OK); return false;}
	if (!meshWaterPlane.Load()){MessageBox(NULL, "Unable to load 'WaterPlane.x'", "Error", MB_OK); return false;}
		if (!meshWaterPlane.ComputeTangentSpace()){MessageBox(NULL, "Unable to compute tangent space for 'WaterPlane.x'", "Error", MB_OK); return false;}
		if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/water_n_1.jpg", &waterNormalMap1))){MessageBox(NULL, "Unable to load 'water_n_1.jpg'", "Error", MB_OK); return false;}
		if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/water_n_2.jpg", &waterNormalMap2))){MessageBox(NULL, "Unable to load 'water_n_2.jpg'", "Error", MB_OK); return false;}
	if (!meshLight.Load()){MessageBox(NULL, "Unable to load 'Light.x'", "Error", MB_OK); return false;}

	//Shader initialisation
	if (FAILED(D3DXCreateEffectFromFile(d3ddev, "shaders/Shaders.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &globalLightingEffect, NULL)))
	{MessageBox(NULL, "Unable to read Shaders.fx", "Error", MB_OK); return false;}

	//Water initialisation
	waterMapViewport.X = 0; waterMapViewport.Y = 0;
	waterMapViewport.Width = WATER_MAP_SIZE; waterMapViewport.Height = WATER_MAP_SIZE;
    waterMapViewport.MinZ = 0; waterMapViewport.MaxZ = 1;
	if (!waterReflectionMap.CreateResources(WATER_MAP_SIZE, WATER_MAP_SIZE, D3DFMT_X8R8G8B8)){return false;}
	if (!waterRefractionMap.CreateResources(WATER_MAP_SIZE, WATER_MAP_SIZE, D3DFMT_X8R8G8B8)){return false;}

	//Post-processing initialisation
	if (FAILED(D3DXCreateSprite(d3ddev, &screenSprite))){return false;}
	if (!postProcessTex1.CreateResources(WIDTH, HEIGHT, D3DFMT_X8R8G8B8)){return false;}
	if (!postProcessTex2.CreateResources(WIDTH, HEIGHT, D3DFMT_X8R8G8B8)){return false;}

	//Sound engine
	soundEngine = irrklang::createIrrKlangDevice();
	if (!soundEngine){MessageBox(NULL, "irrKlang failed to initialise", "Error", MB_OK); return false;}

	return true;
}
bool initD3D(HWND hWnd)
{
	//Create D3D9
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL){return false;}

	//Check current display mode
	D3DDISPLAYMODE d3ddm; 
	if (FAILED(d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))){return false;}
	WIDTH = d3ddm.Width;
	HEIGHT = d3ddm.Height;
	REFRESH_RATE = d3ddm.RefreshRate;

	//DirectX parameters
    ZeroMemory(&d3dpp, sizeof(d3dpp));	
	d3dpp.BackBufferFormat = d3ddm.Format;				
	d3dpp.BackBufferWidth = WIDTH;							
	d3dpp.BackBufferHeight = HEIGHT;						
	d3dpp.BackBufferCount = 1;								
    d3dpp.Windowed = FALSE;								
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;		
	d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;			
	d3dpp.MultiSampleQuality = 0;						
	d3dpp.hDeviceWindow = hWnd;								
    d3dpp.EnableAutoDepthStencil = TRUE;					
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;			
	d3dpp.Flags = 0;									
	d3dpp.FullScreen_RefreshRateInHz = REFRESH_RATE;					
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; 

	//Check shader/vertex processing caps
	D3DCAPS9 d3dCaps;
	if(FAILED(d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps))){return false;}
	DWORD VertexProcessingMethod = d3dCaps.VertexProcessingCaps != 0 ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	if (d3dCaps.VertexShaderVersion < D3DVS_VERSION(2, 0) || d3dCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
	{MessageBox(NULL, "Default video adapter does not support shader version 2.0", "Error", MB_OK); return false;}

	//Device creation
    if(FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, VertexProcessingMethod, &d3dpp, &d3ddev))){return false;}

	//Resource initiation
	if (!initResources()){return false;}

	return true;
}

//Device handling
bool onLostDevice()
{
	postProcessTex1.DeleteResources();
	postProcessTex2.DeleteResources();
	waterReflectionMap.DeleteResources();
	waterRefractionMap.DeleteResources();

	if (FAILED(screenSprite->OnLostDevice())){return false;}

	if (FAILED(globalLightingEffect->OnLostDevice())){return false;}

	return true;
}
bool onResetDevice()
{
	if (!postProcessTex1.CreateResources(WIDTH, HEIGHT, D3DFMT_X8R8G8B8)){return false;}
	if (!postProcessTex2.CreateResources(WIDTH, HEIGHT, D3DFMT_X8R8G8B8)){return false;}
	if (!waterReflectionMap.CreateResources(WATER_MAP_SIZE, WATER_MAP_SIZE, D3DFMT_X8R8G8B8)){return false;}
	if (!waterRefractionMap.CreateResources(WATER_MAP_SIZE, WATER_MAP_SIZE, D3DFMT_X8R8G8B8)){return false;}

	if (FAILED(screenSprite->OnResetDevice())){return false;}

	if (FAILED(globalLightingEffect->OnResetDevice())){return false;}

	return true;
}
bool isDeviceLost()
{
	HRESULT hr = d3ddev->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST)
	{
		Sleep(100);
		return true;
	}

	else if (hr == D3DERR_DEVICENOTRESET)
	{
		if (!onLostDevice()){MessageBox(NULL, "Can't prepare lost device", "Error", MB_OK); return true;}
		if (FAILED(d3ddev->Reset(&d3dpp))){MessageBox(NULL, "Can't reset the present parameters for the device", "Error", MB_OK); return true;}
		if (!onResetDevice()){MessageBox(NULL, "Can't reset the device", "Error", MB_OK); return true;}
	}

	return false;
}

//Draw calls
bool draw_Water()
{
	waterCoordOffsetFirst += D3DXVECTOR2(rippleRate*timeDelta*0.1f, rippleRate*timeDelta*0.1f);
	waterCoordOffsetSecond += D3DXVECTOR2(rippleRate*timeDelta*0.05f, 0);

	globalLightingEffect->SetValue("v2_TexCoordDeltaFirst", &waterCoordOffsetFirst, sizeof(D3DXVECTOR2));
	globalLightingEffect->SetValue("v2_TexCoordDeltaSecond", &waterCoordOffsetSecond, sizeof(D3DXVECTOR2));

	globalLightingEffect->SetValue("v3_LightPos", &lightPos, sizeof(D3DXVECTOR3));
	globalLightingEffect->SetValue("v3_CamPos", &camPos, sizeof(D3DXVECTOR3));

	globalLightingEffect->SetValue("f_AmbientStrength", &zero, sizeof(float));
	globalLightingEffect->SetValue("f_LightPower", &lightPower, sizeof(float));
	globalLightingEffect->SetValue("f_LightRange", &lightRange, sizeof(float));
	globalLightingEffect->SetValue("f_RefractiveIndexScale", &refractiveIndexScale, sizeof(float));

	globalLightingEffect->SetTexture("t_WaterNormalMap1", waterNormalMap1);
	globalLightingEffect->SetTexture("t_WaterNormalMap2", waterNormalMap2);
	globalLightingEffect->SetTexture("t_RefractionMap", waterRefractionMap.drawTexture);
	globalLightingEffect->SetTexture("t_ReflectionMap", waterReflectionMap.drawTexture);

	globalLightingEffect->SetTechnique("Water");
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		meshWaterPlane.RenderGlobal(D3DXVECTOR3(0, waterPlane.d, 0), D3DXVECTOR3(0, 0, 0), matIdentity, true);
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Scene_Objects(D3DXPLANE clipPlane, bool reflect, bool specular)
{
	globalLightingEffect->SetValue("v3_LightPos", &lightPos, sizeof(D3DXVECTOR3));
	globalLightingEffect->SetValue("v3_CamPos", &camPos, sizeof(D3DXVECTOR3));

	globalLightingEffect->SetValue("v4_ClipPlane", &clipPlane, sizeof(D3DXPLANE));

	globalLightingEffect->SetValue("f_AmbientStrength", &lightAmbient, sizeof(float));
	globalLightingEffect->SetValue("f_LightPower", &lightPower, sizeof(float));
	globalLightingEffect->SetValue("f_LightRange", &lightRange, sizeof(float));

	if (reflect){globalLightingEffect->SetTechnique("SceneObjectReflect");}
	else {globalLightingEffect->SetTechnique("SceneObject");}
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		if (reflect)
		{
			meshRoom.RenderGlobal(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), matWaterReflect, specular);
		}
		else
		{
			meshRoom.RenderGlobal(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), matIdentity, specular);
		}
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Lights(D3DXPLANE clipPlane)
{
	globalLightingEffect->SetValue("v4_ClipPlane", &clipPlane, sizeof(D3DXPLANE));

	globalLightingEffect->SetValue("f_LightPower", &lightPower, sizeof(float));

	globalLightingEffect->SetTechnique("Light");
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		meshLight.RenderGlobal(lightPos, D3DXVECTOR3(0, 0, 0), matIdentity, false);
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Processed_Texture(D3DXHANDLE technique, DrawableTex2D screen_texture)
{
	if (FAILED(screenSprite->Begin(D3DXSPRITE_DO_NOT_ADDREF_TEXTURE | D3DXSPRITE_SORT_TEXTURE))){return false;}

	globalLightingEffect->SetValue("i_TextureWidth", &WIDTH, sizeof(unsigned));
	globalLightingEffect->SetValue("i_TextureHeight", &HEIGHT, sizeof(unsigned));

	globalLightingEffect->SetTexture("t_ScreenTexture", screen_texture.drawTexture);

	globalLightingEffect->SetTechnique(technique);
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		screenSprite->Draw(screen_texture.drawTexture, NULL, NULL, NULL, D3DCOLOR_ARGB(255, 255, 255, 255));
		if (FAILED(screenSprite->End())){return false;}
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}
	
	return true;
}

//Rendering
void prerender()
{
	//Allow for camera panning in cinematic mode
	if (cinematic)
	{
		float panSpeed = 0.001f;
		if (GetAsyncKeyState(0x4B)){camPitch += panSpeed;}
		else if (GetAsyncKeyState(0x49)){camPitch -= panSpeed;}
		if (GetAsyncKeyState(0x4C)){camYaw += panSpeed;}
		else if (GetAsyncKeyState(0x4A)){camYaw -= panSpeed;}
	}

	//Camera matrix setup
	D3DXVECTOR3 camUp = D3DXVECTOR3(0, 1, 0);
	D3DXVECTOR3 camForward = D3DXVECTOR3(0, 0, 1);
	D3DXVECTOR3 camRight = D3DXVECTOR3(1, 0, 0);

	D3DXMATRIX matCamYaw;
	D3DXMatrixRotationAxis(&matCamYaw, &camUp, camYaw);
	D3DXVec3TransformCoord(&camForward, &camForward, &matCamYaw);
	D3DXVec3TransformCoord(&camRight, &camRight, &matCamYaw);

	D3DXMATRIX matCamPitch;
	D3DXMatrixRotationAxis(&matCamPitch, &camRight, camPitch);
	D3DXVec3TransformCoord(&camForward, &camForward, &matCamPitch);
	D3DXVec3TransformCoord(&camUp, &camUp, &matCamPitch);

	//Camera motion input
	D3DXVECTOR3 walkDir(0, 0, 0);
	bool walking = false;
	walkSpeed = 0.1f;
	if (GetAsyncKeyState(0x51)){walkSpeed = 0.2f;}
	
	if (GetAsyncKeyState(0x57)){walkDir += camForward; walking = true;}
	else if (GetAsyncKeyState(0x53)){walkDir -= camForward; walking = true;}
	if (GetAsyncKeyState(0x44)){walkDir += camRight; walking = true;}
	else if (GetAsyncKeyState(0x41)){walkDir -= camRight; walking = true;}
	
	D3DXVec3Normalize(&walkDir, &D3DXVECTOR3(walkDir.x, 0, walkDir.z));
	camPos += walkDir*walkSpeed;

	if (walking){camHeadBob += 2*walkSpeed;}
	else {camHeadBob = 0;}

	//Walking effects
	if (!cinematic)
	{
		camPos += handle_Collisions_Linear();
		if (camHeadBob > 4*D3DX_PI){camHeadBob -= 4*D3DX_PI;}
		camPos.y = 5 + 0.1f*cos(camHeadBob);
	
		D3DXMATRIX matCamRoll;
		D3DXMatrixRotationAxis(&matCamRoll, &camForward, 0.01f*sin(0.5f*camHeadBob));
		D3DXVec3TransformCoord(&camRight, &camRight, &matCamRoll);
		D3DXVec3TransformCoord(&camUp, &camUp, &matCamRoll);
	}

	//Populate view matrix
	D3DXMatrixIdentity(&matCamView);
	matCamView._11 = camRight.x; matCamView._12 = camUp.x; matCamView._13 = camForward.x;
	matCamView._21 = camRight.y; matCamView._22 = camUp.y; matCamView._23 = camForward.y;
	matCamView._31 = camRight.z; matCamView._32 = camUp.z; matCamView._33 = camForward.z;
	matCamView._41 = -D3DXVec3Dot(&camPos, &camRight);
	matCamView._42 = -D3DXVec3Dot(&camPos, &camUp);
	matCamView._43 = -D3DXVec3Dot(&camPos, &camForward);

	//Take care of in-game events and variables
	if (lightGrabbed){lightPos = camPos + camForward;}
	D3DXMatrixReflect(&matWaterReflect, &D3DXPLANE(waterPlane.a, waterPlane.b, waterPlane.c, -waterPlane.d));

	soundEngine->setListenerPosition(irrklang::vec3df(camPos.x, camPos.y, camPos.z), irrklang::vec3df(camForward.x, camForward.y, camForward.z));
	buzz->setPosition(irrklang::vec3df(lightPos.x, lightPos.y, lightPos.z));
	buzz->setVolume(lightPower);
}
void renderFrame()
{
//PRERENDERING
	prerender();

	LPDIRECT3DSURFACE9 backBuffer = NULL;
	d3ddev->GetRenderTarget(0, &backBuffer);

//PASS ONE: render/blur refraction map
	postProcessTex1.SetAsTarget();

	d3ddev->SetViewport(&waterMapViewport);

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Lights(D3DXPLANE(waterPlane.a, -waterPlane.b, waterPlane.c, waterPlane.d));
		draw_Scene_Objects(D3DXPLANE(waterPlane.a, -waterPlane.b, waterPlane.c, waterPlane.d), false, false);
	
	d3ddev->EndScene();

	postProcessTex2.SetAsTarget();

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Processed_Texture("BlurH", postProcessTex1);
	
	d3ddev->EndScene();

	waterRefractionMap.SetAsTarget();

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Processed_Texture("BlurV", postProcessTex2);
	
	d3ddev->EndScene();

//PASS TWO: render reflection map
	waterReflectionMap.SetAsTarget();

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Scene_Objects(D3DXPLANE(waterPlane.a, waterPlane.b, waterPlane.c, -waterPlane.d), true, true);
	
	d3ddev->EndScene();

//PASS THREE: render scene
	d3ddev->SetRenderTarget(0, backBuffer);
	SAFE_RELEASE(&backBuffer);

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Lights(D3DXPLANE(0, 1, 0, 10));
		draw_Scene_Objects(D3DXPLANE(0, 1, 0, 10), false, true);
		draw_Water();

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);
}

//Cleaning
void cleanD3D()
{
	SAFE_RELEASE(&globalLightingEffect);
	SAFE_RELEASE(&d3ddev);
	SAFE_RELEASE(&d3d);

	SAFE_RELEASE(&waterNormalMap1);
	SAFE_RELEASE(&waterNormalMap2);

	postProcessTex1.DeleteResources();
	postProcessTex2.DeleteResources();
	waterReflectionMap.DeleteResources();
	waterRefractionMap.DeleteResources();

	meshWaterPlane.Clean();
	meshLight.Clean();
	meshRoom.Clean();

	SAFE_RELEASE(&screenSprite);
	
	if (buzz){buzz->drop();}
	if (soundEngine){soundEngine->drop();}
}

//Win32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_LBUTTONUP:
			handle_Interactivity();
			break;

		 case WM_KEYDOWN:
			switch (wParam)
			{
				case 0x43:
					cinematic = !cinematic;
					break;

				case VK_UP:
					waterPlane.d += 0.1f;
					break;

				case VK_DOWN:
					waterPlane.d -= 0.1f;
					break;

				case VK_ESCAPE:
					PostQuitMessage(WM_QUIT);
					break;
			}
			break;

		case WM_MOUSEMOVE:
			{
				POINT currMousePos;
				GetCursorPos(&currMousePos);
				D3DXVECTOR2 MouseDelta = D3DXVECTOR2(float(currMousePos.x) - WIDTH/2, float(currMousePos.y) - HEIGHT/2);
				if (D3DXVec2Length(&MouseDelta) > 0)
				{
					camYaw += mouseSensitivity*MouseDelta.x;
					camPitch += mouseSensitivity*MouseDelta.y;
					if (camYaw > 2*D3DX_PI){camYaw = 0;}
					else if (camYaw < 0){camYaw = 2*D3DX_PI;}
					if (camPitch > 2*D3DX_PI){camPitch = 0;}
					else if (camPitch < 0){camPitch = 2*D3DX_PI;}
					SetCursorPos(unsigned(WIDTH/2), unsigned(HEIGHT/2));
				}
			}
			break;

        case WM_DESTROY:
            PostQuitMessage(WM_QUIT);
			break;
    }
	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //Step 1: Registering the Window Class
	WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WindowClass";

    if(!RegisterClassEx(&wc))
	{MessageBox(NULL, "The window could not be registered!", "Error", MB_ICONEXCLAMATION | MB_OK); return 0;}

    //Step 2: Creating the Window
    hwnd = CreateWindow("WindowClass", "Direct3D water", WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if(hwnd == NULL){MessageBox(NULL, "Window Creation Failed!", "Error!", MB_OK); return 0;}

	if (!initD3D(hwnd)){MessageBox(NULL, "Direct3D failed to initialise", "Error", MB_ICONEXCLAMATION | MB_OK); return 0;}
	initProgram();

	ShowWindow(hwnd, nCmdShow);

	//Initiate timing variables
	LARGE_INTEGER countFrequency;
	QueryPerformanceFrequency(&countFrequency);
	float secsPerCount = 1/(float)countFrequency.QuadPart;

	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);

    //Step 3: The Message Loop
	MSG Msg;
	while (TRUE)
    {
		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }

        if (Msg.message == WM_QUIT)
            break;

		LARGE_INTEGER currTime;
		QueryPerformanceCounter(&currTime);
		timeDelta = (currTime.QuadPart - lastTime.QuadPart)*secsPerCount;
		if (!isDeviceLost()){renderFrame();}
		lastTime = currTime;
    }

	    cleanD3D();
	    return Msg.wParam;
}
