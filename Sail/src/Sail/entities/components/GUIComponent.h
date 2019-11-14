#pragma once
#include "Component.h"

class GUIComponent : public Component<GUIComponent> {
public:
	GUIComponent(Model* model) : m_ppCurrentModel(model) { }
	~GUIComponent() { }

	Model* getCurrentModel() {
		return m_ppCurrentModel;
	}
	void setCurrentModel(Model* model) {
		m_ppCurrentModel = model;
	}

public:
	// GUIComponent does not 'own' the model, just a pointer to it.
	Model* m_ppCurrentModel = nullptr;	
	
};
