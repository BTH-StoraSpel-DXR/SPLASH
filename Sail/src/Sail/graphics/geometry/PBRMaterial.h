#pragma once

#include "sail/api/Texture.h"

class Shader;

class PBRMaterial {
public:
	typedef std::shared_ptr<PBRMaterial> SPtr;
public:
	// Matching shader struct
	struct PBRSettings {
		glm::vec4 modelColor;
		float metalnessScale;
		float roughnessScale;
		float aoScale;
		float padding;
		int hasAlbedoTexture;
		int hasNormalTexture;
		int hasMetalnessRoughnessAOTexture;
	};

public:
	PBRMaterial(Shader* shader);
	~PBRMaterial();

	void bind(void* cmdList = nullptr);

	void setMetalnessScale(float metalness);
	void setRoughnessScale(float roughness);
	void setAOScale(float ao);
	void setColor(const glm::vec4& color);

	void setAlbedoTexture(const std::string& filename);
	void setNormalTexture(const std::string& filename);
	void setMetalnessRoughnessAOTexture(const std::string& filename);

	void manuallySetTexture(Texture* tex, unsigned int arraySlot);

	/*	Returns a texture
		Default texture id is as follows
		0 - Albedo
		1 - Normal map
		2 - Metalness/Roughness/AO as r/g/b
	*/
	Texture* getTexture(unsigned int id) const;

	const PBRSettings& getPBRSettings() const;

	Shader* getShader() const;

private:
	void getAndInsertTexture(const std::string& filename, int arraySlot);

private:
	Shader* m_shader;

	PBRSettings m_pbrSettings;
	Texture* m_textures[3];

	UINT m_numTextures;

};