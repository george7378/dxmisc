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
	string filename;		

	MeshObject(string name)
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
		globalLightingEffect->SetMatrix("m_LightWorldViewProjection", &(worldMatrix*matLightView*matLightProjection));
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
class DrawableTex2DDepth
{
private:
	LPDIRECT3DSURFACE9 drawSurface;		
	LPDIRECT3DSURFACE9 depthSurface;
public:
	LPDIRECT3DTEXTURE9 drawTexture;	
	
	DrawableTex2DDepth(){};	

	bool CreateResources(unsigned width, unsigned height, D3DFORMAT format, D3DFORMAT depth_format)
	{
		if (FAILED(d3ddev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &drawTexture, NULL))){return false;}
		if (FAILED(drawTexture->GetSurfaceLevel(0, &drawSurface))){return false;}
		if (FAILED(d3ddev->CreateDepthStencilSurface(width, height, depth_format, D3DMULTISAMPLE_NONE, 0, TRUE, &depthSurface, NULL))){return false;}
		
		return true;
	}

	bool SetAsTarget()
	{
		d3ddev->SetRenderTarget(0, drawSurface);
		d3ddev->SetDepthStencilSurface(depthSurface);

		return true;
	}

	void DeleteResources()
	{
		SAFE_RELEASE(&drawTexture);
		SAFE_RELEASE(&drawSurface);
		SAFE_RELEASE(&depthSurface);
	}
};

#endif