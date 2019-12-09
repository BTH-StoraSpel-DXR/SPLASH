#pragma once

#include "Sail/api/Mesh.h"
#include "DX12API.h"

class DX12Mesh : public Mesh {
public:
	DX12Mesh(Data& buildData, Shader* shader);
	DX12Mesh(unsigned int numVertices, Shader* shader);
	virtual ~DX12Mesh();

	virtual void draw(const Renderer& renderer, void* cmdList) override;
	void draw_new(const Renderer& renderer, void* cmdList, int srvOffset);
	virtual void bindMaterial(void* cmdList);
	unsigned int getSRVIndex();
private:
	DX12API* m_context;
	//Points to the srv on the descriptor heap this frame
	unsigned int m_SRVIndex;
};