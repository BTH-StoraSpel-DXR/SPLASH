#pragma once

#include "SailImGuiWindow.h"
#include "Sail/states/StateIdentifiers.h"

class WaitingForPlayersWindow : public SailImGuiWindow {

public:
	WaitingForPlayersWindow(bool showWindow = true);
	~WaitingForPlayersWindow();

	virtual void renderWindow() override;
	void setStateStatus(States::ID state, char status);
private:
	States::ID state; //What state the player should be in
	char minStatus; //minimum status within that state the player should be in before being considered as done
	
};