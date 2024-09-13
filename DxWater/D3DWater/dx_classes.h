#ifndef CLASSES_H
#define CLASSES_H

//Mesh
class MeshObject
{
private:
	LPD3DXMESH objectMesh;
	DWORD numMaterials;
	LPD3DXBUFFER materialBuffer;
	LPD3DXMATERIAL meshMaterials;
	float boundingRadius;
public:
	string filename;

	MeshObject(string name)
	{filename = name;};
	
	bool Load()
	{
		if (FAILED(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_SYSTEMMEM, d3ddev, NULL, &materialBuffer, NULL, &numMaterials, &objectMesh))){return false;}
		
		BYTE* v = 0;
		if (FAILED(objectMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&v))){return false;}
		if (FAILED(D3DXComputeBoundingSphere((D3DXVECTOR3*)v, objectMesh->GetNumVertices(), D3DXGetFVFVertexSize(objectMesh->GetFVF()), &D3DXVECTOR3(0, 0, 0), &boundingRadius))){return false;}
		if (FAILED(objectMesh->UnlockVertexBuffer())){return false;}

		meshMaterials = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();

		return true;
	}

	bool ComputeTangentSpace()
	{
		D3DVERTEXELEMENT9 elements[] =
		{
			{0, sizeof(float)*0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, sizeof(float)*3, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
			{0, sizeof(float)*6, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, sizeof(float)*9, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
			{0, sizeof(float)*12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
			D3DDECL_END()
		};

		LPD3DXMESH tempMesh = NULL;
		if (FAILED(objectMesh->CloneMesh(D3DXMESH_MANAGED, elements, d3ddev, &tempMesh)))
		{SAFE_RELEASE(&tempMesh); return false;}

		SAFE_RELEASE(&objectMesh);
		objectMesh = tempMesh;

		if (FAILED(D3DXComputeTangentFrameEx(objectMesh, D3DDECLUSAGE_TEXCOORD, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_NORMAL, 0, 0, 0, 0.01f, 0.25f, 0.01f, &objectMesh, 0)))
		{return false;}

		SAFE_RELEASE(&tempMesh);

		return true;
	}

	void RenderGlobal(D3DXVECTOR3 pos, D3DXVECTOR3 rot, D3DXMATRIX reflectionMatrix, bool specular)
	{
		D3DXMATRIX translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, pos.x, pos.y, pos.z);
		D3DXMATRIX xRotate, yRotate, zRotate;
		D3DXMatrixRotationX(&xRotate, rot.x); D3DXMatrixRotationY(&yRotate, rot.y); D3DXMatrixRotationZ(&zRotate, rot.z);
		D3DXMATRIX worldMatrix = xRotate*yRotate*zRotate*translationMatrix;

		globalLightingEffect->SetMatrix("m_World", &worldMatrix);
		globalLightingEffect->SetMatrix("m_WorldViewProjection", &(worldMatrix*reflectionMatrix*matCamView*matCamProjection));
		globalLightingEffect->CommitChanges();

		for (DWORD i = 0; i < numMaterials; i++)
		{
			float reflectivity = specular ? meshMaterials[i].MatD3D.Specular.r : 0;
			globalLightingEffect->SetValue("f_Reflectivity", &reflectivity, sizeof(float));
			globalLightingEffect->SetValue("f_SpecularPower", &meshMaterials[i].MatD3D.Power, sizeof(float));
			globalLightingEffect->CommitChanges();

			objectMesh->DrawSubset(i);
		}
	}

	void Clean()
	{
		SAFE_RELEASE(&objectMesh);
		SAFE_RELEASE(&materialBuffer);
	}
};
class MeshObjectTN
{
private:
	LPD3DXMESH objectMesh;
	DWORD numMaterials;
	LPD3DXBUFFER materialBuffer;
	LPD3DXMATERIAL meshMaterials;
	LPDIRECT3DTEXTURE9 *meshTextures;
	LPDIRECT3DTEXTURE9 *meshNormals;
	float boundingRadius;
public:
	string filename;

	MeshObjectTN(string name)
	{filename = name;};
	
	bool Load()
	{
		if (FAILED(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_SYSTEMMEM, d3ddev, NULL, &materialBuffer, NULL, &numMaterials, &objectMesh))){return false;}
		
		BYTE* v = 0;
		if (FAILED(objectMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&v))){return false;}
		if (FAILED(D3DXComputeBoundingSphere((D3DXVECTOR3*)v, objectMesh->GetNumVertices(), D3DXGetFVFVertexSize(objectMesh->GetFVF()), &D3DXVECTOR3(0, 0, 0), &boundingRadius))){return false;}
		if (FAILED(objectMesh->UnlockVertexBuffer())){return false;}

		meshMaterials = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();
		meshTextures = new LPDIRECT3DTEXTURE9[numMaterials];
		meshNormals = new LPDIRECT3DTEXTURE9[numMaterials];

		for (DWORD i = 0; i < numMaterials; i++)
		{
			meshTextures[i] = NULL;
			meshNormals[i] = NULL;

			if (meshMaterials[i].pTextureFilename)
			{
				string texture_path_n = string(meshMaterials[i].pTextureFilename);
				texture_path_n.insert(texture_path_n.find("."), "_n");

				if (FAILED(D3DXCreateTextureFromFile(d3ddev, meshMaterials[i].pTextureFilename, &meshTextures[i]))){return false;}				
				if (FAILED(D3DXCreateTextureFromFile(d3ddev, texture_path_n.c_str(), &meshNormals[i])))
				{
					if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/default/default_n.jpg", &meshNormals[i]))){return false;}
				}
			}
		}

		return true;
	}
	
	bool ComputeTangentSpace()
	{
		D3DVERTEXELEMENT9 elements[] =
		{
			{0, sizeof(float)*0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, sizeof(float)*3, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
			{0, sizeof(float)*6, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, sizeof(float)*9, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
			{0, sizeof(float)*12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
			D3DDECL_END()
		};

		LPD3DXMESH tempMesh = NULL;
		if (FAILED(objectMesh->CloneMesh(D3DXMESH_MANAGED, elements, d3ddev, &tempMesh)))
		{SAFE_RELEASE(&tempMesh); return false;}

		SAFE_RELEASE(&objectMesh);
		objectMesh = tempMesh;

		if (FAILED(D3DXComputeTangentFrameEx(objectMesh, D3DDECLUSAGE_TEXCOORD, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_NORMAL, 0, 0, 0, 0.01f, 0.25f, 0.01f, &objectMesh, 0)))
		{return false;}

		SAFE_RELEASE(&tempMesh);

		return true;
	}

	void RenderGlobal(D3DXVECTOR3 pos, D3DXVECTOR3 rot, D3DXMATRIX reflectionMatrix, bool specular)
	{
		D3DXMATRIX translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, pos.x, pos.y, pos.z);
		D3DXMATRIX xRotate, yRotate, zRotate;
		D3DXMatrixRotationX(&xRotate, rot.x); D3DXMatrixRotationY(&yRotate, rot.y); D3DXMatrixRotationZ(&zRotate, rot.z);
		D3DXMATRIX worldMatrix = xRotate*yRotate*zRotate*translationMatrix;

		globalLightingEffect->SetMatrix("m_World", &worldMatrix);
		globalLightingEffect->SetMatrix("m_WorldViewProjection", &(worldMatrix*reflectionMatrix*matCamView*matCamProjection));
		globalLightingEffect->CommitChanges();

		for (DWORD i = 0; i < numMaterials; i++)
		{
			float reflectivity = specular ? meshMaterials[i].MatD3D.Specular.r : 0;
			globalLightingEffect->SetValue("f_Reflectivity", &reflectivity, sizeof(float));
			globalLightingEffect->SetValue("f_SpecularPower", &meshMaterials[i].MatD3D.Power, sizeof(float));
			globalLightingEffect->SetTexture("t_Texture", meshTextures[i]);
			globalLightingEffect->SetTexture("t_TextureN", meshNormals[i]);
			globalLightingEffect->CommitChanges();
			
			objectMesh->DrawSubset(i);
		}
	}

	void Clean()
	{
		SAFE_RELEASE(&objectMesh);
		SAFE_RELEASE(&materialBuffer);
		for (DWORD i = 0; i < numMaterials; i++)
		{
			SAFE_RELEASE(&meshTextures[i]);
			SAFE_RELEASE(&meshNormals[i]);
		}
		delete[] meshTextures;
		delete[] meshNormals;
	}
};

//Texture
class DrawableTex2D
{
private:
	LPDIRECT3DSURFACE9 drawSurface;
public:
	LPDIRECT3DTEXTURE9 drawTexture;
	
	DrawableTex2D(){};

	bool CreateResources(unsigned width, unsigned height, D3DFORMAT format)
	{
		if (FAILED(d3ddev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &drawTexture, NULL))){return false;}
		if (FAILED(drawTexture->GetSurfaceLevel(0, &drawSurface))){return false;}
		
		return true;
	}

	bool SetAsTarget()
	{
		if (FAILED(d3ddev->SetRenderTarget(0, drawSurface))){return false;}
		
		return true;
	}

	void DeleteResources()
	{
		SAFE_RELEASE(&drawTexture);
		SAFE_RELEASE(&drawSurface);
	}
};

#endif
