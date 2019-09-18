#pragma once

//#include <d3d11.h>
#include <vector>
#include "Sail/api/Mesh.h"
//#include "Sail/api/Renderer.h"
//#include "Sail/utils/Utils.h"
#include <memory>

// Forward declarations
//class ShaderPipeline;
//class Material;

class Shader;

class Model {
public:
	typedef std::unique_ptr<Model> Ptr;

public: 
	//Model(std::vector<Mesh::Data>& data, ShaderSet* shaderSet);
	Model();
	Model(Mesh::Data& data, Shader* shader);
	//Model(const std::string& path, ShaderSet* shaderSet);
	~Model();

	Mesh* addMesh(std::unique_ptr<Mesh> mesh);

	//void setBuildData(Data& buildData);
	//void buildBufferForShader(ShaderSet* shader);

	// Draws the model using its material
	//void draw(const Renderer& renderer);

	Mesh* getMesh(unsigned int index);
	unsigned int getNumberOfMeshes() const;
	/*ShaderSet* getShader() const;
	Material* getMaterial();*/
	//const AABB& getAABB() const;
	//void updateAABB();

private:
	//void calculateAABB();

private:
	std::vector<Mesh::Ptr> m_meshes;

	//Material::SPtr m_material;

	//AABB m_aabb;

};
