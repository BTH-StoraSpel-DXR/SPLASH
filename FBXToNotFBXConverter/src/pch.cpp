#include "pch.h"

void Mesh::Data::resizeVertices(const unsigned int num) {
	assert(num != 0 && "resize in mesh was 0");
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

	delete [] positions;
	delete [] normals;
	delete [] tangents;
	delete [] bitangents;
	delete [] texCoords;

	positions = tempPositions;
	normals = tempNormals;
	tangents = tempTangents;
	bitangents = tempBitangents;
	texCoords = tempUV;
}
