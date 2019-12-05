#include "pch.h"
#include "NetworkedStructs.h"

namespace Netcode {

	/* Special values:
	   0: not initialized
	   1: used for the player's ComponentID
	   2: used for the player's gun's ComponentID
	   3: used for the player's torch's ComponentID
	*/
	std::atomic<ComponentID> gNetworkIDCounter = RESET_VALUE;
	ComponentID gNetworkBotIDCounter = 0U;
}
