#include "pch.h"
#include "Mesh.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/Application.h"

Mesh::Mesh(Data& buildData, Shader* shader)
	: meshData(buildData)
{
}

Mesh::Mesh(unsigned int numVertices, Shader* shader) {
	meshData.numVertices = numVertices;
}



Mesh::~Mesh() {
	Memory::SafeDeleteArr(meshData.indices);
	Memory::SafeDeleteArr(meshData.positions);
	Memory::SafeDeleteArr(meshData.normals);
	Memory::SafeDeleteArr(meshData.bitangents);
	Memory::SafeDeleteArr(meshData.colors);
	Memory::SafeDeleteArr(meshData.tangents);
	Memory::SafeDeleteArr(meshData.texCoords);
}



float Mesh::getSize() {
	return meshData.numVertices * sizeof(glm::vec3) * 4 + meshData.numVertices * sizeof(glm::vec2);
}

const Mesh::Data& Mesh::getMeshData() {
	return meshData;
}

PBRMaterial* Mesh::getMaterial() {
	return material.get();
}

unsigned int Mesh::getNumVertices() const {
	return meshData.numVertices;
}
unsigned int Mesh::getNumIndices() const {
	return meshData.numIndices;
}
unsigned int Mesh::getNumInstances() const {
	return meshData.numInstances;
}
VertexBuffer& Mesh::getVertexBuffer() const {
	return *vertexBuffer;
}
const IndexBuffer& Mesh::getIndexBuffer() const {
	return *indexBuffer;
}
const Mesh::Data& Mesh::getData() const {
	return meshData;
}

void Mesh::Data::deepCopy(const Mesh::Data& other) {
	this->numIndices = other.numIndices;
	this->numVertices = other.numVertices;
	this->numInstances = other.numInstances;
	if (other.indices) {
		this->indices = SAIL_NEW unsigned long[other.numIndices];
		for (unsigned int i = 0; i < other.numIndices; i++)
			this->indices[i] = other.indices[i];
	}
	unsigned int numVerts = (other.numIndices > 0) ? other.numVertices : other.numIndices;
	if (other.positions) {
		this->positions = SAIL_NEW Mesh::vec3[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->positions[i] = other.positions[i];
	}
	if (other.normals) {
		this->normals = SAIL_NEW Mesh::vec3[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->normals[i] = other.normals[i];
	}
	if (other.colors) {
		this->colors = SAIL_NEW Mesh::vec4[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->colors[i] = other.colors[i];
	}
	if (other.texCoords) {
		this->texCoords = SAIL_NEW Mesh::vec2[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->texCoords[i] = other.texCoords[i];
	}
	if (other.tangents) {
		this->tangents = SAIL_NEW Mesh::vec3[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->tangents[i] = other.tangents[i];
	}
	if (other.bitangents) {
		this->bitangents = SAIL_NEW Mesh::vec3[numVerts];
		for (unsigned int i = 0; i < numVerts; i++)
			this->bitangents[i] = other.bitangents[i];
	}
}

void Mesh::Data::resizeVertices(const unsigned int num) {
	assert(num != 0 && "resize in mesh was 0" );
	numVertices = num;

	Mesh::vec3* tempPositions = SAIL_NEW Mesh::vec3[num];
	Mesh::vec3* tempNormals = SAIL_NEW Mesh::vec3[num];
	Mesh::vec3* tempTangents = SAIL_NEW Mesh::vec3[num];
	Mesh::vec3* tempBitangents = SAIL_NEW Mesh::vec3[num];
	Mesh::vec2* tempUV = SAIL_NEW Mesh::vec2[num];

	memcpy(tempPositions, positions, num * sizeof(Mesh::vec3));
	memcpy(tempNormals, normals, num * sizeof(Mesh::vec3));
	memcpy(tempTangents, tangents, num * sizeof(Mesh::vec3));
	memcpy(tempBitangents, bitangents, num * sizeof(Mesh::vec3));
	memcpy(tempUV, texCoords, num * sizeof(Mesh::vec2));

	Memory::SafeDeleteArr(positions);
	Memory::SafeDeleteArr(normals);
	Memory::SafeDeleteArr(tangents);
	Memory::SafeDeleteArr(bitangents);
	Memory::SafeDeleteArr(texCoords);

	positions = tempPositions;
	normals = tempNormals;
	tangents = tempTangents;
	bitangents = tempBitangents;
	texCoords = tempUV;
}
