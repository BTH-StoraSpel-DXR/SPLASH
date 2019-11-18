#pragma once
#include "..//BaseComponentSystem.h"

template <typename T>
class ModelSubmitSystem final : public BaseComponentSystem {
public:
	ModelSubmitSystem();
	~ModelSubmitSystem() = default;

	void submitAll(const float alpha);
};