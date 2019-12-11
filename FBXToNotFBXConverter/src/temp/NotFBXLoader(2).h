#pragma once

#include <string>

class AnimationStack;
class Shader;
struct Data;

namespace NotFBXLoader {

	void Load(const std::string& filename, Mesh::Data* model, Shader* shader, AnimationStack*& animationStack);
	void Save(const std::string& filename, Mesh::Data* mdata, AnimationStack* animationStack);

}