#pragma once

#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/geometry/Animation.h"
#include <string>

namespace NotFBXLoader {

	void Load(const std::string& filename, Model*& model, Shader* shader, AnimationStack*& animationStack);
	void Save(const std::string& filename, Model* model, AnimationStack* animationStack);

}