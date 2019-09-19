#pragma once

#include "Sail/api/Mesh.h"
#include "DX12API.h"

class DX12Mesh : public Mesh {
public:
	DX12Mesh(Data& buildData, Shader* shader);
	~DX12Mesh();

	/*[Depricated]*/
	virtual void draw(const Renderer& renderer, void* cmdList) override;
	virtual void draw_new(const Renderer& renderer, void* cmdList, int meshIndex);
	virtual void bindMaterial(void* cmdList, int meshIndex);
	unsigned int getSRVIndex();
private:
	DX12API* m_context;
	//Points to the srv on the descriptor heap this frame
	unsigned int m_SRVIndex;
};