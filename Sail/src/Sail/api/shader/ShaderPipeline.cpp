#include "pch.h"
#include "ShaderPipeline.h"
#include "Sail/Application.h"
#include <regex>

ShaderPipeline* ShaderPipeline::CurrentlyBoundShader = nullptr;
const std::string ShaderPipeline::DEFAULT_SHADER_LOCATION = "res/shaders/";

using namespace Utils::String;

ShaderPipeline::ShaderPipeline(const std::string& filename)
	: vsBlob(nullptr)
	, gsBlob(nullptr)
	, psBlob(nullptr)
	, dsBlob(nullptr)
	, hsBlob(nullptr)
	, csBlob(nullptr)
	, filename(filename)
	, wireframe(false)
	, cullMode(GraphicsAPI::Culling::NO_CULLING)
	, numRenderTargets(1)
	, enableDepth(true)
	, enableDepthWrite(true)
	, blendMode(GraphicsAPI::NO_BLENDING)
{
	inputLayout = std::unique_ptr<InputLayout>(InputLayout::Create());
}

ShaderPipeline::~ShaderPipeline() {
	//Memory::safeRelease(VSBlob); // Do this?
}

void ShaderPipeline::compile() {
	std::string filepath = DEFAULT_SHADER_LOCATION + filename;
	std::string source = Utils::readFile(filepath);
	if (source == "")
		SAIL_LOG_ERROR("Shader file is empty or does not exist: " + filename);
	parse(source);

	if (parsedData.hasVS) {
		vsBlob = compileShader(source, filepath, ShaderComponent::VS);
		//Memory::safeRelease(VSBlob); // is this right?
	}
	if (parsedData.hasPS) {
		psBlob = compileShader(source, filepath, ShaderComponent::PS);
		//Memory::safeRelease(blob);
	}
	if (parsedData.hasGS) {
		gsBlob = compileShader(source, filepath, ShaderComponent::GS);
		//Memory::safeRelease(blob);
	}
	if (parsedData.hasDS) {
		dsBlob = compileShader(source, filepath, ShaderComponent::DS);
		//Memory::safeRelease(blob);
	}
	if (parsedData.hasHS) {
		hsBlob = compileShader(source, filepath, ShaderComponent::HS);
		//Memory::safeRelease(blob);
	}
	if (parsedData.hasCS) {
		csBlob = compileShader(source, filepath, ShaderComponent::CS);
	}
}

void ShaderPipeline::finish() {
}

void ShaderPipeline::bind(void* cmdList) {
	// Don't bind if already bound
	// This is to cut down on shader state changes
	if (CurrentlyBoundShader == this)
		return;

	for (auto& it : parsedData.cBuffers) {
		it.cBuffer->bind(cmdList);
	}
	for (auto& it : parsedData.samplers) {
		it.sampler->bind();
	}


	// Set input layout as active
	inputLayout->bind();
}

void ShaderPipeline::parse(const std::string& source) {
	
	// Find what shader types are contained in the source
	if (source.find("VSMain") != std::string::npos) { parsedData.hasVS = true; }
	if (source.find("PSMain") != std::string::npos) { parsedData.hasPS = true; }
	if (source.find("GSMain") != std::string::npos) { parsedData.hasGS = true; }
	if (source.find("DSMain") != std::string::npos) { parsedData.hasDS = true; }
	if (source.find("HSMain") != std::string::npos) { parsedData.hasHS = true; }
	if (source.find("CSMain") != std::string::npos) { parsedData.hasCS = true; }

	if (!parsedData.hasVS && !parsedData.hasPS && !parsedData.hasGS && !parsedData.hasDS && !parsedData.hasHS && !parsedData.hasCS) {
		SAIL_LOG_ERROR("No main function found in shader. The main function(s) needs to be named VSMain, PSMain, GSMain, DSMain, HSMain or CSMain");
		assert(false);
	}

	// Remove comments from source
	std::string cleanSource = removeComments(source);

	const char* src;

	// Count and reserve memory for the vector of parsed data
	// This is needed to avoid copying/destructor calling
	{
		int numCBuffers = 0;
		src = cleanSource.c_str();
		while (src = findToken("cbuffer", src))	numCBuffers++;
		parsedData.cBuffers.reserve(numCBuffers);
	}
	{
		int numStructBuffers = 0;
		src = cleanSource.c_str();
		while (src = findToken("StructuredBuffer", src)) numStructBuffers++;
		parsedData.structuredBuffers.reserve(numStructBuffers);
	}
	{
		int numSamplers = 0;
		src = cleanSource.c_str();
		while (src = findToken("SamplerState", src)) numSamplers++;
		parsedData.samplers.reserve(numSamplers);
	}
	
	// Process all CBuffers
	src = cleanSource.c_str();
	while (src = findToken("cbuffer", src)) {
		parseCBuffer(getBlockStartingFrom(src));
	}

	// Process all samplers
	src = cleanSource.c_str();
	while (src = findToken("SamplerState", src)) {
		parseSampler(src);
	}

	// Process all textures
	src = cleanSource.c_str();
	while (src = findToken("RWTexture2D", src)) {
		parseRWTexture(src);
	}
	src = cleanSource.c_str();
	while (src = findToken("Texture2D", src)) {
		parseTexture(src);
	}

	// Process all structured buffers (used in some compute shaders)
	src = cleanSource.c_str();
	while (src = findToken("StructuredBuffer", src)) {
		char* rwCheck = (char*)(src - 18);
		bool isRW = false;
		if (rwCheck >= cleanSource.c_str() && rwCheck[0] == 'R' && rwCheck[1] == 'W') {
			isRW = true;
		}
		parseStructuredBuffer(src, isRW);
	}

}

void ShaderPipeline::parseCBuffer(const std::string& source) {

	const char* src = source.c_str();

	std::string bufferName = nextToken(src);
	auto bindShader = getBindShaderFromName(bufferName);


	int registerSlot = findNextIntOnLine(src);
	src = findToken("{", src); // Place ptr on same line as starting bracket
	src = nextLine(src);

	//SAIL_LOG("Slot: " + std::to_string(registerSlot));

	UINT size = 0;
	std::vector<ShaderCBuffer::CBufferVariable> vars;

	while (src < source.c_str() + source.size()) {
		std::string type = nextToken(src);
		src += type.size();
		UINT tokenSize;
		std::string name = nextTokenAsName(src, tokenSize, true);
		src = nextLine(src);

		// Push the name and byteOffset to the vector
		vars.push_back({name, size});
		size += getSizeOfType(type);

		/*SAIL_LOG("Type: " + type);
		SAIL_LOG("Name: " + name);*/
	}

	// Memory align to 16 bytes
	if (size % 16 != 0)
		size = size - (size % 16) + 16;

	void* initData = malloc(size);
	memset(initData, 0, size);
	parsedData.cBuffers.emplace_back(vars, initData, size, bindShader, registerSlot);
	free(initData);

	//SAIL_LOG(src);
}

void ShaderPipeline::parseSampler(const char* source) {
	UINT tokenSize = 0;
	std::string name = nextTokenAsName(source, tokenSize);
	source += tokenSize;

	auto bindShader = getBindShaderFromName(name);

	int slot = findNextIntOnLine(source);
	if (slot == -1) slot = 0; // No slot specified, use 0 as default

	ShaderResource res(name, static_cast<UINT>(slot));
	parsedData.samplers.emplace_back(res, Texture::WRAP, Texture::MIN_MAG_MIP_LINEAR, bindShader, slot);
}

void ShaderPipeline::parseTexture(const char* source) {
	if (source[0] == '<') {
		// A type was found in the place of a name
		// This probably means that it is a RWTexture2D and has already been handled
		source = nextLine(source);
		return;
	}

	UINT tokenSize = 0;
	std::string name = nextTokenAsName(source, tokenSize);
	source += tokenSize;

	int slot = findNextIntOnLine(source);
	if (slot == -1) slot = 0; // No slot specified, use 0 as default

	parsedData.textures.emplace_back(name, slot);
}

void ShaderPipeline::parseRWTexture(const char* source) {
	UINT tokenSize = 0;
	std::string type = nextTokenAsType(source, tokenSize);
	source += tokenSize;

	tokenSize = 0;
	std::string name = nextTokenAsName(source, tokenSize);
	source += tokenSize;

	tokenSize = 0;
	int slot = findNextIntOnLine(source);
	if (slot == -1) {
		slot = 0; // No slot specified, use 0 as default
	}

	// Get texture format from source, if specified
	Texture::FORMAT format = Texture::R8G8B8A8;
	const char* newLine = strchr(source, '\n');
	size_t lineLength = newLine - source;
	char* lineCopy = (char*)malloc(lineLength + 1);
	memset(lineCopy, '\0', lineLength + 1);
	strncpy_s(lineCopy, lineLength + 1, source, lineLength);
	if (strstr(lineCopy, "SAIL_RGBA16_FLOAT")) {
		format = Texture::R16G16B16A16_FLOAT;
	}
	free(lineCopy);

	std::string nameSuffix(" File: " + filename + " slot " + std::to_string(slot));
	parsedData.renderableTextures.emplace_back(ShaderResource(name, slot), format, nameSuffix);
}

void ShaderPipeline::parseStructuredBuffer(const char* source, bool isRW) {
	// TODO: use type for something (or remove it)
	UINT tokenSize = 0;
	std::string type = nextTokenAsType(source, tokenSize);
	source += tokenSize;

	tokenSize = 0;
	std::string name = nextTokenAsName(source, tokenSize);
	source += tokenSize;
	auto bindShader = getBindShaderFromName(name);

	int slot = findNextIntOnLine(source);
	if (slot == -1) {
		slot = 0; // No slot specified, use 0 as default
	}

	UINT numElements = 1; // Buffers larger than one element will have to resize on first usage
	UINT stride = getSizeOfType(type);
	UINT size = numElements * stride;

	void* initData = malloc(size);
	memset(initData, 0, size);
	parsedData.structuredBuffers.emplace_back(name, initData, size, numElements, stride, bindShader, slot, isRW);
	free(initData);
}

std::string ShaderPipeline::nextTokenAsName(const char* source, UINT& outTokenSize, bool allowArray) const {
	std::string name = nextToken(source);
	outTokenSize = (UINT)name.size() + 1U; /// +1 to account for the space before the name
	if (name[name.size() - 1] == ';') {
		name = name.substr(0, name.size() - 1); // Remove ending ';'
	}
	bool isArray = name[name.size() - 1] == ']';
	if (!allowArray && isArray) {
		SAIL_LOG_ERROR("Shader resource with name \"" + name + "\" is of unsupported type - array");
	}
	if (isArray) {
		// remove [asd] part from the name
		auto start = name.find("[");
		auto size = name.substr(start).find(']') + 1;
		name.erase(start, size);
	}
	return name;
}

std::string ShaderPipeline::nextTokenAsType(const char* source, UINT& outTokenSize) const {
	std::string type = nextToken(source);
	outTokenSize = (UINT)type.size();
	// Remove first '<' and last '>' character
	type = type.substr(1, type.size() - 2);

	return type;
}

ShaderComponent::BIND_SHADER ShaderPipeline::getBindShaderFromName(const std::string& name) const {
	if (startsWith(name.c_str(), "VSPS") || startsWith(name.c_str(), "PSVS")) return ShaderComponent::BIND_SHADER(ShaderComponent::VS | ShaderComponent::PS);
	if (startsWith(name.c_str(), "VS")) { return ShaderComponent::VS; }
	if (startsWith(name.c_str(), "PS")) { return ShaderComponent::PS; }
	if (startsWith(name.c_str(), "GS")) { return ShaderComponent::GS; }
	if (startsWith(name.c_str(), "DS")) { return ShaderComponent::DS; }
	if (startsWith(name.c_str(), "HS")) { return ShaderComponent::HS; }
	if (startsWith(name.c_str(), "CS")) { return ShaderComponent::CS; }
	SAIL_LOG_WARNING("Shader resource with name \"" + name + "\" not starting with VS/PS etc, using VS as default in shader: \"" + filename + "\"");
	return ShaderComponent::VS; // Default to binding to VertexShader
}

bool ShaderPipeline::isComputeShader() const {
	return csBlob;
}

void ShaderPipeline::setWireframe(bool wireframeState) {
	wireframe = wireframeState;
}

void ShaderPipeline::setCullMode(GraphicsAPI::Culling newCullMode) {
	cullMode = newCullMode;
}

void ShaderPipeline::setNumRenderTargets(unsigned int numRenderTargets) {
	this->numRenderTargets = numRenderTargets;
}

void ShaderPipeline::enableDepthStencil(bool enable) {
	this->enableDepth = enable;
}

void ShaderPipeline::enableDepthWriting(bool enable) {
	this->enableDepthWrite = enable;
}

void ShaderPipeline::setBlending(GraphicsAPI::Blending blendMode) {
	this->blendMode = blendMode;
}

InputLayout& ShaderPipeline::getInputLayout() {
	return *inputLayout.get();
}

void* ShaderPipeline::getVsBlob() {
	return vsBlob;
}

const std::string& ShaderPipeline::getName() const {
	return filename;
}

RenderableTexture* ShaderPipeline::getRenderableTexture(const std::string& name) const {
	for (auto& it : parsedData.renderableTextures) {
		if (it.res.name == name) {
			return it.renderableTexture.get();
		}
	}
	SAIL_LOG_ERROR("Tried to get a RenderableTexture named \"" + name + "\" which does not exist in the ShaderPipeline.");
	return nullptr;
}

// TODO: size isnt really needed, can be read from the byteOffset of the next var
void ShaderPipeline::setCBufferVar(const std::string& name, const void* data, UINT size) {
	bool success = trySetCBufferVar(name, data, size);
	if (!success) {
		SAIL_LOG_WARNING("Tried to set CBuffer variable that did not exist (" + name + ")");
	}
}

bool ShaderPipeline::trySetCBufferVar(const std::string& name, const void* data, UINT size) {
	for (auto& it : parsedData.cBuffers) {
		for (auto& var : it.vars) {
			if (var.name == name) {
				ShaderComponent::ConstantBuffer& cbuffer = *it.cBuffer.get();
				cbuffer.updateData(data, size, var.byteOffset);
				return true;
			}
		}
	}
	return false;
}

void ShaderPipeline::setStructBufferVar(const std::string& name, const void* data, UINT numElements) {
	bool success = trySetStructBufferVar(name, data, numElements);
	if (!success) {
		SAIL_LOG_WARNING("Tried to set StructuredBuffer variable that did not exist (" + name + ")");
	}
}

bool ShaderPipeline::trySetStructBufferVar(const std::string& name, const void* data, UINT numElements) {
	for (auto& it : parsedData.structuredBuffers) {
		if (it.name == name) {
			ShaderComponent::StructuredBuffer& sbuffer = *it.sBuffer.get();
			sbuffer.updateData(data, numElements);
			return true;
		}
	}
	return false;
}

// TODO: registerTypeSize(typeName, size)
UINT ShaderPipeline::getSizeOfType(const std::string& typeName) const {
	if (typeName == "uint") { return 4; }
	if (typeName == "bool") { return 4; }
	if (typeName == "float") { return 4; }
	if (typeName == "float2" || typeName == "int2" || typeName == "uint2") { return 4 * 2; }
	if (typeName == "float3") { return 4 * 3; }
	if (typeName == "float4") { return 4 * 4; }
	if (typeName == "float3x3") { return 4 * 3 * 3; }
	if (typeName == "float4x4" || typeName == "matrix") { return 4 * 4 * 4; }

	if (typeName == "Material") { return 48; }
	if (typeName == "DirectionalLight") { return 32; }
	if (typeName == "PointLight") { return 32; }
	if (typeName == "PointlightInput") { return 32 * 12; }
	if (typeName == "SpotlightInput") { return 12 * 4 * 12; }
	if (typeName == "DeferredPointLightData") { return 32; }
	if (typeName == "DeferredDirLightData") { return 32; }
	if (typeName == "Vertex") { return 4 * 14; }
	if (typeName == "VertConnections") { return 4 + 4*5 + 4*5; }
	if (typeName == "ParticleInput") { return (12 * 312 + 312 + 12) * 4; }
	if (typeName == "IndexMap") { return 4 * 4 * 12 * 2; } // NUM_POINT_LIGHTS * 2 * int

	SAIL_LOG_ERROR("Found shader variable type with unknown size (" + typeName + ")");
	return 0;
}

UINT ShaderPipeline::findSlotFromName(const std::string& name, const std::vector<ShaderResource>& resources) const {
	for (auto& resource : resources) {
		if (resource.name == name)
			return resource.slot;
	}
	SAIL_LOG_ERROR("Could not find shader resource named \"" + name + "\"");
	return -1;
}