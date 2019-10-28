#include "pch.h"
#include "ResourceManager.h"
//#include "../graphics/shader/deferred/DeferredGeometryShader.h"
//#include "audio/SoundManager.h"
#include "Sail/graphics/shader/Shader.h"
#include "Sail/api/shader/ShaderPipeline.h"
#include "Sail/api/Mesh.h"

const std::string ResourceManager::SAIL_DEFAULT_MODEL_LOCATION = "res/models/";

ResourceManager::ResourceManager() {
	//m_soundManager = std::make_unique<SoundManager>();
	m_assimpLoader = std::make_unique<AssimpLoader>();
	m_fbxLoader = std::make_unique<FBXLoader>();
	m_defaultShader = nullptr;
}
ResourceManager::~ResourceManager() {
	for (auto it : m_shaderSets) {
		delete it.second;
	}
}

//
// AudioData
//

bool ResourceManager::setDefaultShader(Shader* shader) {
	if (shader) {
		m_defaultShader = shader;
		return true;
	}
	return false;
}

void ResourceManager::loadAudioData(const std::string& filename, IXAudio2* xAudio2) {
	if (!this->hasAudioData(filename)) {
		m_audioDataAll.insert({ filename, std::make_unique<AudioData>(filename, xAudio2) });
	} 
}

AudioData& ResourceManager::getAudioData(const std::string& filename) {
	auto pos = m_audioDataAll.find(filename);
	if (pos == m_audioDataAll.end()) {
		Logger::Error("Tried to access an audio resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadAudioData(\"filename\") before accessing it.");
	}

	return *pos->second;
}

bool ResourceManager::hasAudioData(const std::string& filename) {
	return m_audioDataAll.find(filename) != m_audioDataAll.end();
}

//
// TextureData
//

void ResourceManager::loadTextureData(const std::string& filename) {
	m_textureDatas.insert({ filename, std::make_unique<TextureData>(filename) });
}
TextureData& ResourceManager::getTextureData(const std::string& filename) {
	auto pos = m_textureDatas.find(filename);
	if (pos == m_textureDatas.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadTextureData(\"filename\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTextureData(const std::string& filename) {
	return m_textureDatas.find(filename) != m_textureDatas.end();
}

//
// DXTexture
//

void ResourceManager::loadTexture(const std::string& filename) {
	m_textures.insert({ filename, std::unique_ptr<Texture>(Texture::Create(filename)) });
}
Texture& ResourceManager::getTexture(const std::string& filename) {
	auto pos = m_textures.find(filename);
	if (pos == m_textures.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().loadTexture(\"" + filename + "\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTexture(const std::string& filename) {
	return m_textures.find(filename) != m_textures.end();
}


//
// Model
//

// Used to add a model created from the application
void ResourceManager::addModel(const std::string& modelName, Model* model) {
	if (m_models.find(modelName) != m_models.end()) {
		Logger::Error("The model name is already in use");
		return;
	}
	Logger::Log("Added model: " + modelName);
	model->setName(modelName);
	m_models.insert({modelName, std::unique_ptr<Model>(model)});
}

void ResourceManager::loadModel(const std::string& filename, Shader* shader, const ImporterType type) {
	// Insert the new model
	Shader* shaderToUse = shader ? shader : m_defaultShader;
	if (m_models.find(filename) != m_models.end()) {
		return;

	}

	Model* temp = nullptr;
	if (type == ResourceManager::ImporterType::SAIL_ASSIMP) {
		temp = m_assimpLoader->importModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shaderToUse);
	}
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		temp = m_fbxLoader->fetchModel(SAIL_DEFAULT_MODEL_LOCATION + filename, shaderToUse);
	}

	if (temp) {
		Logger::Log("Loaded model: " + filename);
		temp->setName(filename);
		m_models.insert({ filename, std::unique_ptr<Model>(temp) });
	}
	else {
#ifdef _DEBUG
		Logger::Error("Could not Load model: (" + filename + ")");
		//assert(temp);
#endif
	}
}
Model& ResourceManager::getModel(const std::string& filename, Shader* shader, const ImporterType type) {
	auto pos = m_models.find(filename);
	if (pos == m_models.end()) {
		// Model was not yet loaded, load it and return
		Shader* shaderToUse = shader ? shader : m_defaultShader;
		loadModel(filename, shaderToUse, type);
		
		return *m_models.find(filename)->second;
	}

	return *pos->second;
}
Model& ResourceManager::getModelCopy(const std::string& filename, Shader* shader) {
	Shader* shaderToUse = shader ? shader : m_defaultShader;
	Model& model = getModel(filename, shaderToUse);
	
	Mesh* mesh = model.getMesh(0);
	Mesh::Data data;
	data.deepCopy(mesh->getData());
	Model* tempModel = new Model(data, shaderToUse);
	std::string nameCopy = getSuitableName(filename);
	tempModel->setName(nameCopy);
	Logger::Log("copied model: " + filename + ", using name: " + nameCopy);
	m_models.insert({ nameCopy, std::unique_ptr<Model>(tempModel) });

	return *m_models.find(nameCopy)->second;
}
bool ResourceManager::hasModel(const std::string& filename) {
	return m_models.find(filename) != m_models.end();
}

void ResourceManager::loadAnimationStack(const std::string& fileName, const ImporterType type) {
	AnimationStack* temp = nullptr;
	if (m_animationStacks.find(fileName) != m_animationStacks.end()) {
		return;
	}

	if (type == ResourceManager::ImporterType::SAIL_ASSIMP) {
		temp = m_assimpLoader->importAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName);
	}
	else if (type == ResourceManager::ImporterType::SAIL_FBXSDK) {
		//TODO: REMOVE SHADER FROM ANIMATION IMPORT
		assert(m_defaultShader&& "set default shader first, or load model first");
		temp = m_fbxLoader->fetchAnimationStack(SAIL_DEFAULT_MODEL_LOCATION + fileName, m_defaultShader);
	}

	if (temp) {
		m_animationStacks.insert({ fileName, std::unique_ptr<AnimationStack>(temp) });
	}
	else {
#ifdef _DEBUG
		Logger::Error("Could not Load model: (" + fileName + ")");
#endif
	}
}

AnimationStack& ResourceManager::getAnimationStack(const std::string& fileName) {
	//TODO : make more reliable
	if (m_animationStacks.find(fileName) == m_animationStacks.end()) {
		loadAnimationStack(fileName);
	}
	return *m_animationStacks[fileName].get();
}

bool ResourceManager::hasAnimationStack(const std::string& fileName) {
	return false;
}

const std::string ResourceManager::getSuitableName(const std::string& name) {
	unsigned int iterator = 1;
	while (iterator < 1000) {
		std::string tempName = name + std::to_string(iterator++);
		if (m_models.find(tempName) == m_models.end()) {
			return tempName;
		}
	}
	return "broken";
}

//void ResourceManager::reloadShaders() {
//	for (auto it = m_shaderSets.begin(); it != m_shaderSets.end(); ++it)
//		it->second->reload();
//}


// Sound Manager
//SoundManager* ResourceManager::getSoundManager() {
//	return m_soundManager.get();
//}