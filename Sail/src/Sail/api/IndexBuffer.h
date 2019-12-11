#pragma once

#include "Mesh.h"

class IndexBuffer {
public:
	static IndexBuffer* Create(Mesh::Data& modelData);
	IndexBuffer(Mesh::Data& modelData);
	virtual ~IndexBuffer() { }

	virtual void bind(void* cmdList = nullptr) = 0;

	virtual unsigned int getByteSize() const = 0;

protected:
	unsigned long* getIndexData(Mesh::Data& modelData);
	unsigned int getIndexDataSize() const;

private:
	unsigned int m_byteSize;

};