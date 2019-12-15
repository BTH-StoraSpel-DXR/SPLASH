#pragma once
#include "Sail/api/VertexBuffer.h"
#include "DX12API.h"

class DX12VertexBuffer : public VertexBuffer {
public:
	DX12VertexBuffer(const InputLayout& inputLayout, const Mesh::Data& modelData, bool allowCpuUpdates = false);
	DX12VertexBuffer(const InputLayout& inputLayout, unsigned int numVertices, bool allowCpuUpdates = false);
	~DX12VertexBuffer();

	virtual void bind(void* cmdList) const override;
	ID3D12Resource* getBuffer(int frameOffset = 0) const;
	void update(Mesh::Data& data);
	void setAsUpdated();
	bool hasBeenUpdated() const;
	void resetHasBeenUpdated();
	bool init(ID3D12GraphicsCommandList4* cmdList);

	unsigned int getByteSize() const override;

private:
	void init(void* data);

private:
	DX12API* m_context;
	bool m_allowCpuUpdates;
	unsigned int m_byteSize;

	std::vector<wComPtr<ID3D12Resource>> m_uploadVertexBuffers;
	DX12API::CommandQueue* m_queueUsedForUpload;
	UINT64 m_initFenceVal;
	unsigned int m_initFrameCount;

	std::vector<wComPtr<ID3D12Resource>> m_defaultVertexBuffers;
	std::vector<bool> m_hasBeenInitialized;
	std::vector<bool> m_hasBeenUpdated;
};

