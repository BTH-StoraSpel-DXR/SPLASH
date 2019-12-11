#pragma once

#include <Map>
#include <memory>
#include "TextureData.h"
#include "AudioData.h"
#include "Sail/api/Texture.h"
//#include "ParsedScene.h"

#include <glm/ext.hpp>
#include "Sail/graphics/geometry/Animation.h"
#include "Sail/graphics/geometry/Model.h"

#define LOAD_NOT_FBX

//class DeferredGeometryShader;
class ShaderPipeline;
class Shader;
//class SoundManager;

struct ID3D12GraphicsCommandList4;

class ResourceManager final {
public:
	ResourceManager();
	~ResourceManager();

	enum ImporterType {
		SAIL_FBXSDK,
		SAIL_NOT_FBXSDK,
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
	const unsigned int getShaderByteSize() const;
	const unsigned int getMiscByteSize() const;
	// SoundManager
	//SoundManager* getSoundManager();

	void uploadFinishedTextures(ID3D12GraphicsCommandList4* cmdList);
	void clearModelCopies();
	void releaseTextureUploadBuffers();

#ifdef DEVELOPMENT
	void unloadTextures();
	void logRemainingTextures() const;
	void printLoadedTexturesToFile() const;
	void printModelsToFile() const;
#endif

private:
	unsigned int calculateTextureByteSize() const;
	unsigned int calculateAnimationByteSize() const;
	unsigned int calculateModelByteSize() const;
	unsigned int calculateAudioByteSize() const;
	unsigned int calculateMiscByteSize() const;
	unsigned int calculateShaderByteSize() const;

	const std::string getSuitableName(const std::string& name);

	enum RMDataType {
		Models = 0,
		Animations,
		Audio,
		Textures,
		Shaders,
		N_dataTypes
	};
	unsigned int m_byteSize[static_cast<size_t>(N_dataTypes)];

private:
	// Audio files/data mapped to their filenames
	std::map<std::string, std::unique_ptr<AudioData>> m_audioDataAll;
	// Textures mapped to their filenames
	
	std::map<std::string, std::unique_ptr<TextureData>> m_textureDatas;
	std::map<std::string, std::unique_ptr<Texture>> m_textures;
	// Models mapped to their filenames
	//std::map<std::string, std::unique_ptr<ParsedScene>> m_fbxModels;
	mutable std::mutex m_modelMutex;
	std::map < std::string, std::unique_ptr<Model>> m_models;
	std::mutex m_animationMutex;
	std::map < std::string, std::unique_ptr<AnimationStack>> m_animationStacks;
	// ShaderSets mapped to their identifiers
	std::map<std::string, Shader*> m_shaderSets;
	// SoundManager containing all sounds
	//std::unique_ptr<SoundManager> m_soundManager;

	// Used when uploading textures to VRAM
	std::mutex m_finishedTexturesMutex;
	mutable std::mutex m_textureDatasMutex;
	std::vector<Texture*> m_finishedTextures;

	Shader* m_defaultShader;

#ifdef DEVELOPMENT
	std::vector<std::string> m_loadedTextures;
	mutable bool m_hasLoggedTextures = false;
	mutable bool m_hasLoggedModels = false;
#endif
};

template <typename T>
void ResourceManager::loadShaderSet() {
	// Insert and get the new ShaderSet
	//m_shaderSets.insert({ typeid(T).name(), std::make_unique<T>() });
	m_shaderSets.insert({ typeid(T).name(), SAIL_NEW T() });
}
