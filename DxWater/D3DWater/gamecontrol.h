#ifndef GAMECONTROL_H
#define GAMECONTROL_H

//Lighting
D3DXVECTOR3 lightPos(15, 7, 0);
float lightPower = 1;
float lightAmbient = 0.1f;
float lightRange = 40;

//Camera
D3DXVECTOR3 camPos(-20, 5, 0);
float camHeadBob = 0;
float camPitch = 0;
float camYaw = D3DX_PI/2;
float walkSpeed = 0.1f;
bool cinematic = FALSE;

//Water
unsigned WATER_MAP_SIZE = 512;
D3DXVECTOR2 waterCoordOffsetFirst, waterCoordOffsetSecond;
D3DXPLANE waterPlane(0, 1, 0, 0);
float rippleRate = 0.3f;
float refractiveIndexScale = 0.020f;

//Collisions
float wallThickness = 1;
D3DXVECTOR4 collision_lines_room[8] =
{
	//Room walls
	D3DXVECTOR4(-25, -25, -25, 25),
	D3DXVECTOR4(-25, 25, 25, 25),
	D3DXVECTOR4(25, 25, 25, -25),
	D3DXVECTOR4(25, -25, -25, -25),

	//Pool edge
	D3DXVECTOR4(-10, -10, -10, 10),
	D3DXVECTOR4(-10, 10, 10, 10),
	D3DXVECTOR4(10, 10, 10, -10),
	D3DXVECTOR4(10, -10, -10, -10),
};

//Misc.
float zero = 0;
float one = 1;
float timeDelta = 0;
float mouseSensitivity = 0.002f;
bool lightGrabbed = FALSE;

#endif
