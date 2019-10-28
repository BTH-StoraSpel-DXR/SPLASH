#include "pch.h"
#include "VertexBuffer.h"
#include "Sail/utils/Utils.h"

VertexBuffer::VertexBuffer(const InputLayout& inputLayout, const Mesh::Data& modelData)
	: inputLayout(inputLayout) 
{
	m_stride = inputLayout.getVertexSize();
	if (m_stride == 0) {
		Logger::Error("Input layout not set up properly in shader");
		__debugbreak();
	}
	m_byteSize = modelData.numVertices * m_stride;
}

void* VertexBuffer::getVertexData(const Mesh::Data& modelData) {
	void* vertices = malloc(modelData.numVertices * m_stride);

	UINT byteOffset = 0;
	for (UINT i = 0; i < modelData.numVertices; i++) {
		// Loop through the input layout to get the order the data should have
		for (const InputLayout::InputType& inputType : inputLayout.getOrderedInputs()) {
			void* addr = (char*)vertices + byteOffset;

			if (inputType == InputLayout::POSITION) {
				UINT size = sizeof(glm::vec3);
				memcpy(addr, &modelData.positions[i], size);
				byteOffset += size;
			} else if (inputType == InputLayout::POSITION2D) {
				UINT size = sizeof(glm::vec2);
				memcpy(addr, &modelData.positions[i], size);
				byteOffset += size;
			} else if (inputType == InputLayout::TEXCOORD) {
				UINT size = sizeof(glm::vec2);
				// Check if model data contains texCoords
				if (modelData.texCoords)
					memcpy(addr, &modelData.texCoords[i], size);
				else
					memset(addr, 0, size);
				byteOffset += size;
			} else if (inputType == InputLayout::NORMAL) {
				UINT size = sizeof(glm::vec3);
				if (modelData.normals)
					memcpy(addr, &modelData.normals[i], size);
				else
					memset(addr, 0, size);
				byteOffset += size;
			} else if (inputType == InputLayout::TANGENT) {
				UINT size = sizeof(glm::vec3);
				if (modelData.tangents)
					memcpy(addr, &modelData.tangents[i], size);
				else
					memset(addr, 0, size);
				byteOffset += size;
			} else if (inputType == InputLayout::BITANGENT) {
				UINT size = sizeof(glm::vec3);
				if (modelData.bitangents)
					memcpy(addr, &modelData.bitangents[i], size);
				else
					memset(addr, 0, size);
				byteOffset += size;
			}
		}
	}
	return vertices;
}

unsigned int VertexBuffer::getVertexDataSize() const {
	return m_byteSize;
}

unsigned int VertexBuffer::getVertexDataStride() const {
	return m_stride;
}
