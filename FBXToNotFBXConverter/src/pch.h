#pragma once
#define FBX_CONVERTER
#define SAIL_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace Mesh {
	typedef ::glm::vec4 vec4;
	typedef ::glm::vec3 vec3;
	typedef ::glm::vec2 vec2;

	struct Data {
		Data() : numIndices(0), numInstances(0), indices(nullptr), numVertices(0), normals(nullptr), positions(nullptr), colors(nullptr), texCoords(nullptr), tangents(nullptr), bitangents(nullptr) {};

		//void deepCopy(const Data& other);
		// will throw away data outside of range
		void resizeVertices(const unsigned int num);
		unsigned int numIndices;
		unsigned long* indices;
		unsigned int numVertices;
		unsigned int numInstances;
		glm::vec3* positions;
		glm::vec3* normals;
		glm::vec4* colors;
		glm::vec2* texCoords;
		glm::vec3* tangents;
		glm::vec3* bitangents;
	};
}