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
public:
	std::string filename;		

	MeshObject(std::string name)
	{filename = name;};
	
	bool Load()
	{
		if (FAILED(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_SYSTEMMEM, d3ddev, NULL, &materialBuffer, NULL, &numMaterials, &objectMesh))){return false;}

		meshMaterials = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();
		meshTextures = new LPDIRECT3DTEXTURE9[numMaterials];
		
		for (DWORD i = 0; i < numMaterials; i++)
		{
			meshTextures[i] = NULL;

			if (meshMaterials[i].pTextureFilename)
			{
				if (FAILED(D3DXCreateTextureFromFile(d3ddev, meshMaterials[i].pTextureFilename, &meshTextures[i]))){return false;}
			}
		}

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
		}
		delete[] meshTextures;
	}
};

#endif