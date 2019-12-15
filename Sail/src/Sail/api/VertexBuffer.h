#pragma once

#include "shader/InputLayout.h"
#include "Mesh.h"

class VertexBuffer {
public:
	static VertexBuffer* Create(const InputLayout& inputLayout, const Mesh::Data& modelData, bool allowCpuUpdates = false);
	static VertexBuffer* Create(const InputLayout& inputLayout, unsigned int numVertices, bool allowCpuUpdates = false);

	VertexBuffer(const InputLayout& inputLayout, unsigned int numVertices);
	virtual ~VertexBuffer() {};

	virtual void bind(void* cmdList = nullptr) const = 0;
	virtual void update(Mesh::Data& data) = 0;
	unsigned int getVertexDataStride() const;

	virtual unsigned int getByteSize() const = 0;

protected:
	void* getVertexData(const Mesh::Data& modelData);
	unsigned int getVertexDataSize() const;
protected:
	const InputLayout& inputLayout;

private:
	unsigned int m_byteSize;
	unsigned int m_stride;

};