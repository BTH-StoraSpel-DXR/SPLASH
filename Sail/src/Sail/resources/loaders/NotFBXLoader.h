#pragma once

#include <string>

#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/geometry/Animation.h"

namespace NotFBXLoader {

	bool Load(const std::string& filename, Mesh::Data* mData, AnimationStack*& animationStack);
	void Save(const std::string& filename, Mesh::Data* mdata, AnimationStack* animationStack);

}