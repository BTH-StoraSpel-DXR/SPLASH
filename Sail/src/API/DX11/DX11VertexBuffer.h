#pragma once
//#include <d3d11.h>
#include "shader/DX11InputLayout.h"
#include "Sail/api/VertexBuffer.h"

struct ID3D11Buffer;

class DX11VertexBuffer : public VertexBuffer {
public:
	DX11VertexBuffer(const InputLayout& inputLayout, Mesh::Data& modelData);
	~DX11VertexBuffer();

	ID3D11Buffer* const* getBuffer() const;
	virtual void bind(void* cmdList) const override;

private:
	ID3D11Buffer* m_vertBuffer;
	ID3D11Buffer* m_instanceBuffer;
};

