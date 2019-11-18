#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "Sail/graphics/geometry/PBRMaterial.h"
#include "Sail/api/Renderer.h"

class VertexBuffer;
class IndexBuffer;

class Mesh {
public:
	typedef std::unique_ptr<Mesh> Ptr;
	typedef std::shared_ptr<Mesh> SPtr;

	struct vec2 {
		glm::vec2 vec;
		vec2(float x = 0.f, float y = 0.f) {
			this->vec.x = x; this->vec.y = y;
		}
		const bool operator==(const vec2& other) const {
			return this->vec == other.vec;
		}
	};
	struct vec3 {
		glm::vec3 vec;
		vec3(float x = 0.f, float y = 0.f, float z = 0.f) {
			this->vec.x = x; this->vec.y = y; this->vec.z = z;
		}
		const bool operator==(const vec3& other) const{
			return this->vec == other.vec;
		}
	};
	struct vec4 {
		glm::vec4 vec;
		vec4(float x = 0.f, float y = 0.f, float z = 0.f, float w = 0.f) {
			this->vec.x = x; this->vec.y = y; this->vec.z = z; this->vec.w = w;
		}
		const bool operator==(const vec4& other) const {
			return this->vec == other.vec;
		}
	};

	struct Data {
		Data() : numIndices(0), numInstances(0), indices(nullptr), numVertices(0), normals(nullptr), positions(nullptr), colors(nullptr), texCoords(nullptr), tangents(nullptr), bitangents(nullptr) {};
		
		void deepCopy(const Data& other);
		// will throw away data outside of range
		void resizeVertices(const unsigned int num);
		unsigned int numIndices;
		unsigned long* indices;
		unsigned int numVertices;
		unsigned int numInstances;
		Mesh::vec3* positions;
		Mesh::vec3* normals;
		Mesh::vec4* colors;
		Mesh::vec2* texCoords;
		Mesh::vec3* tangents;
		Mesh::vec3* bitangents;
	};

public:
	static Mesh* Create(Data& buildData, Shader* shader);
	static Mesh* Create(unsigned int numVertices, Shader* shader);
	Mesh(Data& buildData, Shader* shader);
	Mesh(unsigned int numVertices, Shader* shader);
	virtual ~Mesh();

	virtual void draw(const Renderer& renderer, void* cmdList = nullptr) = 0;

	// In bytes
	float getSize();
	const Data& getMeshData();

	PBRMaterial* getMaterial();

	unsigned int getNumVertices() const;
	unsigned int getNumIndices() const;
	unsigned int getNumInstances() const;
	VertexBuffer& getVertexBuffer() const;
	const IndexBuffer& getIndexBuffer() const;
	const Mesh::Data& getData() const;

protected:
	PBRMaterial::SPtr material;

	std::unique_ptr<VertexBuffer> vertexBuffer;
	std::unique_ptr<IndexBuffer> indexBuffer;
	Data meshData;

};