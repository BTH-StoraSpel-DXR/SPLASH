#pragma once
#include "..//BaseComponentSystem.h"
#include "Sail.h"


class GUISubmitSystem  : public BaseComponentSystem {
public:
	GUISubmitSystem();
	~GUISubmitSystem();

	void submitAll();

private:
};
