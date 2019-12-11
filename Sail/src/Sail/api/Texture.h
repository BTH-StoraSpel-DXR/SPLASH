#pragma once

#include <string.h>
#include "Sail/resources/TextureData.h"

class Texture {
public:
	enum ADDRESS_MODE {
		WRAP,
		MIRROR,
		CLAMP,
		BORDER,
		MIRROR_ONCE
	};
	enum FILTER {
		MIN_MAG_MIP_POINT,
		MIN_MAG_POINT_MIP_LINEAR,
		MIN_POINT_MAG_LINEAR_MIP_POINT,
		MIN_POINT_MAG_MIP_LINEAR,
		MIN_LINEAR_MAG_MIP_POINT,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MIN_MAG_LINEAR_MIP_POINT,
		MIN_MAG_MIP_LINEAR,
		ANISOTROPIC
		// TODO: add more filters if needed
	};
	enum FORMAT {
		R8,
		R8G8,
		R8G8B8A8,
		R16_FLOAT,
		R16G16_FLOAT,
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT
	};
public:
	static Texture* Create(const std::string& filename);
	virtual ~Texture() {}

	virtual unsigned int getByteSize() const = 0;

	//virtual SailTexture* getHandle() = 0;

protected:
	TextureData& getTextureData(const std::string& filename) const;

};