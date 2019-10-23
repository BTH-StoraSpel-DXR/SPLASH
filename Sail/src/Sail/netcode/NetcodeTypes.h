#pragma once

namespace Netcode {
	// Used to identify individual players
	typedef unsigned char    PlayerID;
	
	// Used to identify Sender-/ReceiverComponents
	// The first byte == PlayerID of the player the component belongs to
	typedef unsigned __int32 ComponentID; 
}