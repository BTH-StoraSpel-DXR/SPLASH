#pragma once

#include <string>

//class AnimationStack;
//class Shader;

#ifdef FBX_CONVERTER
#include "pch.h"
#include "Animation.h"
#else

#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/geometry/Animation.h"
#endif // !FBX_CONVERTER


namespace NotFBXLoader {

	void Load(const std::string& filename, Mesh::Data* mData, AnimationStack*& animationStack);
	void Save(const std::string& filename, Mesh::Data* mdata, AnimationStack* animationStack);

}