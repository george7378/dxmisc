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
	LPDIRECT3DTEXTURE9 *meshTextures;
	LPDIRECT3DTEXTURE9 *meshNormals;
public:
	std::string filename;		

	MeshObject(std::string name)
	{filename = name;};
	
	bool Load()
	{
		if (FAILED(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_SYSTEMMEM, d3ddev, NULL, &materialBuffer, NULL, &numMaterials, &objectMesh))){return false;}

		meshMaterials = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();
		meshTextures = new LPDIRECT3DTEXTURE9[numMaterials];
		meshNormals = new LPDIRECT3DTEXTURE9[numMaterials];

		for (DWORD i = 0; i < numMaterials; i++)
		{
			meshTextures[i] = NULL;
			meshNormals[i] = NULL;

			if (meshMaterials[i].pTextureFilename)
			{
				std::string texture_path = std::string(meshMaterials[i].pTextureFilename);
				std::string texture_ext = texture_path.substr(texture_path.find("."), 4);
				texture_path.erase(texture_path.end() - 4, texture_path.end());
				std::string texture_n = texture_path + "_n" + texture_ext;

				if (FAILED(D3DXCreateTextureFromFile(d3ddev, meshMaterials[i].pTextureFilename, &meshTextures[i]))){return false;}
				if (FAILED(D3DXCreateTextureFromFile(d3ddev, texture_n.c_str(), &meshNormals[i]))){return false;}
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

	void RenderGlobal(D3DXVECTOR3 pos, D3DXVECTOR3 rot)
	{
		D3DXMATRIX translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, pos.x, pos.y, pos.z);
		D3DXMATRIX xRotate, yRotate, zRotate;
		D3DXMatrixRotationX(&xRotate, rot.x); D3DXMatrixRotationY(&yRotate, rot.y); D3DXMatrixRotationZ(&zRotate, rot.z);
		D3DXMATRIX worldMatrix = xRotate*yRotate*zRotate*translationMatrix;

		globalLightingEffect->SetMatrix("m_World", &worldMatrix);
		globalLightingEffect->SetMatrix("m_WorldViewProjection", &(worldMatrix*matCamView*matCamProjection));
		globalLightingEffect->CommitChanges();

		for (DWORD i = 0; i < numMaterials; i++)
		{
			globalLightingEffect->SetTexture("t_Texture", meshTextures[i]);
			globalLightingEffect->SetTexture("t_NormalMap", meshNormals[i]);
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

#endif