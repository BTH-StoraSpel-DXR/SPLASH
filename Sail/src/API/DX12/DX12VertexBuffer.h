#pragma once
#include "Sail/api/VertexBuffer.h"
#include "DX12API.h"

class DX12VertexBuffer : public VertexBuffer {
public:
	DX12VertexBuffer(const InputLayout& inputLayout, Mesh::Data& modelData);
	~DX12VertexBuffer();

	virtual void bind(void* cmdList) const override;
	ID3D12Resource1* getBuffer() const;

private:
	wComPtr<ID3D12Resource1> m_vertexBuffer;

};

