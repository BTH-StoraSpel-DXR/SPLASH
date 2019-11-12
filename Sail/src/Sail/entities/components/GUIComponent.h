#pragma once
#include "Component.h"



class GUIComponent : public Component<GUIComponent> {
public:
	GUIComponent(Model* model) : m_model(model) { }
	~GUIComponent() { }

	Model* getModel() {
		return m_model;
	}
private:
	Model* m_model;

};
