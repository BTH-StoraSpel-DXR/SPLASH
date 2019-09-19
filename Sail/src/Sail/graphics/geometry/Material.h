#pragma once

#include <d3d11.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "sail/api/Texture.h"

class Shader;

class Material {
public:
	typedef std::shared_ptr<Material> SPtr;
public:
	// Matching shader struct
	struct PhongSettings {
		glm::vec4 modelColor;
		float ka;
		float kd;
		float ks;
		float shininess;
		int hasDiffuseTexture;
		int hasNormalTexture;
		int hasSpecularTexture;
	};

public:
	Material(Shader* shader);
	~Material();

	void bind(void* cmdList = nullptr);

	void setKa(float ka);
	void setKd(float kd);
	void setKs(float ks);
	void setShininess(float shininess);
	void setColor(const glm::vec4& color);

	void setDiffuseTexture(const std::string& filename);
	void setDiffuseTextureFromHandle(Texture* srv);

	void setNormalTexture(const std::string& filename);
	void setNormalTextureFromHandle(Texture* srv);

	void setSpecularTexture(const std::string& filename);
	void setSpecularTextureFromHandle(Texture* srv);

	//void setTextures(ID3D11ShaderResourceView** srvs, UINT numTextures);

	/*	Returns a texture
		Default texture id is as follows
		0 - Diffuse texture
		1 - Normal map
		2 - Specular map
	*/ 
	Texture* getTexture(unsigned int id) const;

	//const glm::vec4& getColor() const;
	const PhongSettings& getPhongSettings() const;

	Shader* getShader() const;
	//const bool* getTextureFlags() const;// TODO: remove

private:
	void getAndInsertTexture(const std::string& filename, int arraySlot);

private:
	Shader* m_shader;

	PhongSettings m_phongSettings;
	Texture* m_textures[3];

	UINT m_numTextures;

	//ID3D11ShaderResourceView** m_customSRVs;

};