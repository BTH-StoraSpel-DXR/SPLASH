#pragma once
#include "ArchiveHelperFunctions.h"

namespace Netcode {
	typedef unsigned __int32 NetworkObjectID;
	
	// Global counter
	extern NetworkObjectID gNetworkIDCounter;
	
	static NetworkObjectID createNetworkID()    { return ++gNetworkIDCounter; }
	static NetworkObjectID nrOfNetworkObjects() { return gNetworkIDCounter; }


	/*
	  Enums for the kind of entities/data that will be sent over the network,
	  will just be used to read to/write from in the NetworkSystems
	*/


	// Pre-defined entity types so that other players know which entity to create
	enum EntityType : __int32 {
		PLAYER_ENTITY = 1,
		MECHA_ENTITY = 2,
	};

	// The message type decides how the subsequent data will be parsed and used
	enum MessageType : __int32 {
		CREATE_NETWORKED_ENTITY = 1,
		MODIFY_TRANSFORM,
		SPAWN_PROJECTILE,
		ROTATION_TRANSFORM,
		ANIMATION,
		PLAYER_JUMPED,
		WATER_HIT_PLAYER,
		SET_CANDLE_HEALTH,
		PLAYER_DIED,
		PLAYER_DISCONNECT,
		MATCH_ENDED,
		CANDLE_HELD_STATE,
		SEND_ALL_BACK_TO_LOBBY,
		EMPTY = 69
	};

	/*
	  Structs for the kind of data that will be sent/
	  They map to the message types as if the message consists of a tagged union

	  Logical structure of a message:
	  
	  struct NetworkMessage {
	      NetworkObjectID objectID;
	      MessageType     type;
	      union {
	          ModifyTransform transform;
	          SpawnProjectile spawnProjectile;
	          ...
	      }
	  }
	*/

	// MODIFY_TRANSFORM
	struct ModifyTransform {
		glm::vec3 transform;

		template <class Archive>
		void save(Archive& ar) const {
			ArchiveHelpers::serializeVec3(ar, position);
		}

		template <class Archive>
		void load(Archive& ar) {
			ArchiveHelpers::serializeVec3(ar, position);
		}
	};

	// ROTATION_TRANSFORM
	struct RotationTransform {
		glm::vec3 rotation;

		template <class Archive>
		void save(Archive& ar) const {
			ArchiveHelpers::serializeVec3(ar, position);
		}

		template <class Archive>
		void load(Archive& ar) {
			ArchiveHelpers::serializeVec3(ar, position);
		}
	};


	class MessageData {
	public:
		MessageData() {}
		virtual ~MessageData() {}
	};

	class MessageDataProjectile : public MessageData {
	public:
		MessageDataProjectile(glm::vec3 translation_, glm::vec3 velocity_)
			: translation(translation_), velocity(velocity_)
		{}
		virtual ~MessageDataProjectile() {}

		glm::vec3 translation;
		glm::vec3 velocity;
	};

	class MessageDataWaterHitPlayer : public MessageData {
	public:
		MessageDataWaterHitPlayer(Netcode::NetworkObjectID idWasHit)
			: playerWhoWasHitID(idWasHit)
		{}
		~MessageDataWaterHitPlayer() {}

		Netcode::NetworkObjectID playerWhoWasHitID;
	};

	class MessageDataPlayerDied : public MessageData {
	public:
		// Player ID of shooter is stored in its candle.
		MessageDataPlayerDied(Netcode::NetworkObjectID networkIdOfKilled, unsigned char playerIdOfShooter) 
			: playerWhoDied(networkIdOfKilled), playerWhoFired(playerIdOfShooter){}
				~MessageDataPlayerDied() {}
				Netcode::NetworkObjectID playerWhoDied;
				unsigned char playerWhoFired;
		};

	class MessageDataCandleHeldState : public MessageData {
	public:
		MessageDataCandleHeldState(Netcode::NetworkObjectID id, bool b, glm::vec3 pos) : candleOwnerID(id), isHeld(b), candlePos(pos) {}
		~MessageDataCandleHeldState() {}
		Netcode::NetworkObjectID candleOwnerID;
		bool isHeld;
		glm::vec3 candlePos;
	};

	class MessageDataPlayerDisconnect : public MessageData {
	public:
		MessageDataPlayerDisconnect(unsigned char id) : playerID(id) {}
		~MessageDataPlayerDisconnect() {}
		unsigned char playerID;
	};

}

