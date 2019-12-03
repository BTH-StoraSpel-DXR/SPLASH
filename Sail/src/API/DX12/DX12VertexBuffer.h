#pragma once
#include "Sail/api/VertexBuffer.h"
#include "DX12API.h"

class DX12VertexBuffer : public VertexBuffer {
public:
	DX12VertexBuffer(const InputLayout& inputLayout, const Mesh::Data& modelData);
	DX12VertexBuffer(const InputLayout& inputLayout, unsigned int numVertices);
	~DX12VertexBuffer();

	virtual void bind(void* cmdList) const override;
	ID3D12Resource* getBuffer(int frameOffset = 0) const;
	void update(Mesh::Data& data);
	void setAsUpdated();
	bool hasBeenUpdated() const;
	void resetHasBeenUpdated();
	bool init(ID3D12GraphicsCommandList4* cmdList);

private:
	void init(void* data);

private:
	DX12API* m_context;
	unsigned int m_byteSize;
	std::vector<wComPtr<ID3D12Resource>> m_uploadVertexBuffers;
	std::vector<wComPtr<ID3D12Resource>> m_defaultVertexBuffers;
	std::vector<bool> m_hasBeenInitialized;
	std::vector<bool> m_hasBeenUpdated;
};

