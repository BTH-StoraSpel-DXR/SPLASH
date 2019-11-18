#pragma once
#include "..//BaseComponentSystem.h"

template <typename T>
class MetaballSubmitSystem final : public BaseComponentSystem {
public:
	MetaballSubmitSystem();
	~MetaballSubmitSystem() = default;

	void submitAll(const float alpha);
};