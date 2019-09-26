#pragma once
#include "Component.h"
#include "Sail/graphics/geometry/Material.h"

class MetaballComponent : public Component <MetaballComponent>{
	MetaballComponent(float radius = 1.0f);
	~MetaballComponent();

	float getRadius() const;
	void setRadius(float radius);
private:
	float m_radius;
	Material m_material;
};

