#pragma once

#include <vector>
#include "Sail/api/Mesh.h"
#include "Sail/api/Renderer.h"

// Forward declarations
class ShaderPipeline;

class Model {
public:
	typedef std::unique_ptr<Model> Ptr;

public: 
	Model();
	Model(Mesh::Data& data, Shader* shader);
	Model(unsigned int numVertices, Shader* shader);
	~Model();
	void setName(const std::string& name);
	Mesh* addMesh(std::unique_ptr<Mesh> mesh);

	// Draws the model using its material
	void draw(const Renderer& renderer);

	Mesh* getMesh(unsigned int index);
	UINT getNumberOfMeshes() const;
	unsigned int getByteSize() const;

	void setIsAnimated(bool animated);
	bool isAnimated() const;

	void setCastShadows(bool cast);
	bool castsShadows() const;

private:
	std::string m_name;
	std::vector<Mesh::Ptr> m_meshes;
	bool m_isAnimated;
	bool m_castShadows;
};
