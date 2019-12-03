#pragma once

#include <Map>
#include <memory>
#include "TextureData.h"
#include "AudioData.h"
#include "Sail/api/Texture.h"
//#include "ParsedScene.h"
#include "loaders/AssimpLoader.h"
#include "loaders/FBXLoader.h"

//class DeferredGeometryShader;
class ShaderPipeline;
class Shader;
//class SoundManager;

class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	enum ImporterType {
		SAIL_FBXSDK,
		SAIL_ASSIMP
	};
	bool setDefaultShader(Shader* shader);


	// AudioData
	void loadAudioData(const std::string& filename, IXAudio2* xAudio2);
	AudioData& getAudioData(const std::string& filename);
	bool hasAudioData(const std::string& filename);

	static const std::string SAIL_DEFAULT_MODEL_LOCATION;
	static const std::string SAIL_DEFAULT_SOUND_LOCATION;
	static const std::string SAIL_DEFAULT_TEXTURE_LOCATION;

	

	// TextureData
	void loadTextureData(const std::string& filename);
	TextureData& getTextureData(const std::string& filename);
	bool hasTextureData(const std::string& filename);

	// Texture
	void loadTexture(const std::string& filename);
	Texture& getTexture(const std::string& filename);
	bool hasTexture(const std::string& filename);

	// Models
	void addModel(const std::string& modelName, Model* model);
	bool loadModel(const std::string& filename, Shader* shader = nullptr, const ImporterType type = SAIL_FBXSDK);
	Model& getModel(const std::string& filename, Shader* shader = nullptr, const ImporterType type = SAIL_FBXSDK);
	Model& getModelCopy(const std::string& filename, Shader* shader = nullptr);
	bool hasModel(const std::string& filename);
	void clearSceneData();

	// Animations
	void loadAnimationStack(const std::string& fileName, const ImporterType type = SAIL_FBXSDK);
	AnimationStack& getAnimationStack(const std::string& fileName);
	bool hasAnimationStack(const std::string& fileName);

	// ShaderSets
	template <typename T>
	void loadShaderSet();

	template <typename T>
	T& getShaderSet() {
		auto pos = m_shaderSets.find(typeid(T).name());
		if (pos == m_shaderSets.end()) {
			// ShaderSet was not yet loaded, load it and return
			loadShaderSet<T>();
			return dynamic_cast<T&>(*(m_shaderSets.find(typeid(T).name())->second));
		}

		return dynamic_cast<T&>(*pos->second);
	}
	template <typename T>
	bool hasShaderSet() {
		return m_shaderSets.find(typeid(T).name()) != m_shaderSets.end();
	}

	template <typename T>
	void reloadShader() {
		std::string name = typeid(T).name();
		auto it = m_shaderSets.find(name);
		if (it == m_shaderSets.end()) {
			SAIL_LOG("Cannot reload shader " + name + " since it is not loaded in the first place.");
			return;
		}
		T* shader = dynamic_cast<T*>(it->second);
		shader->~T();
		shader = new (shader) T();
		SAIL_LOG("Reloaded shader " + name);
	}


	const unsigned int numberOfModels() const;
	const unsigned int numberOfTextures() const;
	const unsigned int getByteSize() const;
	const unsigned int getModelByteSize() const;
	const unsigned int getAnimationsByteSize() const;
	const unsigned int getAudioByteSize() const;
	const unsigned int getTextureByteSize() const;
	const unsigned int getGenericByteSize() const;
	// SoundManager
	//SoundManager* getSoundManager();

private:
	const std::string getSuitableName(const std::string& name);

	enum RMDataType {
		Models = 0,
		Animations,
		Audio,
		Textures,
		Generic
	};
	unsigned int m_byteSize[5];

private:
	// Audio files/data mapped to their filenames
	std::map<std::string, std::unique_ptr<AudioData>> m_audioDataAll;
	// Textures mapped to their filenames
	
	std::map<std::string, std::unique_ptr<TextureData>> m_textureDatas;
	std::map<std::string, std::unique_ptr<Texture>> m_textures;
	// Models mapped to their filenames
	//std::map<std::string, std::unique_ptr<ParsedScene>> m_fbxModels;
	std::mutex m_modelMutex;
	std::map < std::string, std::unique_ptr<Model>> m_models;
	std::mutex m_animationMutex;
	std::map < std::string, std::unique_ptr<AnimationStack>> m_animationStacks;
	// ShaderSets mapped to their identifiers
	std::map<std::string, Shader*> m_shaderSets;
	// SoundManager containing all sounds
	//std::unique_ptr<SoundManager> m_soundManager;


	std::unique_ptr<AssimpLoader> m_assimpLoader;
	std::unique_ptr<FBXLoader> m_fbxLoader;
	Shader* m_defaultShader;
};

template <typename T>
void ResourceManager::loadShaderSet() {
	// Insert and get the new ShaderSet
	//m_shaderSets.insert({ typeid(T).name(), std::make_unique<T>() });
	m_shaderSets.insert({ typeid(T).name(), SAIL_NEW T() });
}
