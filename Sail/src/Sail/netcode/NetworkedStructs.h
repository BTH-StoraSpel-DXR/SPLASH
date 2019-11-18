#pragma once
#include "ArchiveHelperFunctions.h"
#include "NetcodeTypes.h"

#include <atomic>

namespace Netcode {
	// Global counter
	extern std::atomic<ComponentID> gNetworkIDCounter;
	static ComponentID createNetworkID()    { return ++gNetworkIDCounter; }
	static ComponentID nrOfNetworkObjects() { return gNetworkIDCounter; }


	// Used to signify NetworkMessages sent Internally
	static constexpr PlayerID MESSAGE_FROM_SELF_ID = 255;
	
	// ID for sprinkler
	static constexpr PlayerID MESSAGE_SPRINKLER_ID = 254;
	static constexpr PlayerID MESSAGE_INSANITY_ID = 253;
	static constexpr PlayerID NONE_PLAYER_ID_START = 200;

	// ComponentID has 32 bits and the first 8 are the PlayerID of the owner which
	// can be extracted by shifting the ComponentID 18 bits to the right.
	static constexpr ComponentID SHIFT_AMOUNT = 18;


	// Generates a unique ID for a NetworkSenderComponent based on the player's PlayerID
	static ComponentID generateUniqueComponentID(PlayerID ownerID) {
		return (createNetworkID() | (static_cast<ComponentID>(ownerID) << SHIFT_AMOUNT));
	}
	// Extract the PlayerID of the owner of a NetworkComponent from the component's ID
	static constexpr PlayerID getComponentOwner(ComponentID componentID) {
		return static_cast<PlayerID>(componentID >> SHIFT_AMOUNT);
	}


	/*
	  Enums for the kind of entities/data that will be sent over the network,
	  will just be used to read to/write from in the NetworkSystems
	*/


	// Pre-defined entity types so that other players know which entity to create
	enum class EntityType : __int8 {
		PLAYER_ENTITY = 1,
		CANDLE_ENTITY,
		GUN_ENTITY,
		PROJECTILE_ENTITY,
		MECHA_ENTITY, // RIP Mecha-Jörgen (2019-2019)
		INVALID_ENTITY,
	};

	// TODO: should be one message type for tracked entities and one for events
	// The message type decides how the subsequent data will be parsed and used
	enum class MessageType : __int8 {
		CREATE_NETWORKED_PLAYER = 1,
		DESTROY_ENTITY,
		CHANGE_LOCAL_POSITION,
		CHANGE_LOCAL_ROTATION,
		CHANGE_ABSOLUTE_POS_AND_ROT,
		SPAWN_PROJECTILE,
		ANIMATION,
		SHOOT_START,
		SHOOT_LOOP,
		SHOOT_END,
		PLAYER_JUMPED,
		PLAYER_LANDED,
		WATER_HIT_PLAYER,
		SET_CANDLE_HEALTH,
		EXTINGUISH_CANDLE,
		PLAYER_DIED,
		MATCH_ENDED,
		PREPARE_ENDSCREEN,			// Clients send relevant data for the endgame screen
		ENDGAME_STATS,
		CANDLE_HELD_STATE,
		RUNNING_METAL_START,
		RUNNING_TILE_START,
		RUNNING_WATER_METAL_START,
		RUNNING_WATER_TILE_START,
		RUNNING_STOP_SOUND,
		IGNITE_CANDLE,
		UPDATE_SANITY,
		HIT_BY_SPRINKLER,
		ENABLE_SPRINKLERS,
		START_THROWING,
		STOP_THROWING,
		EMPTY,
		COUNT
	}; 
	
	static const std::string MessageNames[] = {
		"CREATE_NETWORKED_PLAYER",
		"DESTROY_ENTITY",
		"CHANGE_LOCAL_POSITION",
		"CHANGE_LOCAL_ROTATION",
		"CHANGE_ABSOLUTE_POS_AND_ROT",
		"SPAWN_PROJECTILE",
		"ANIMATION",
		"SHOOT_START",
		"SHOOT_LOOP",
		"SHOOT_END",
		"PLAYER_JUMPED",
		"PLAYER_LANDED",
		"WATER_HIT_PLAYER",
		"SET_CANDLE_HEALTH",
		"EXTINGUISH_CANDLE",
		"PLAYER_DIED",
		"MATCH_ENDED",
		"PREPARE_ENDSCREEN",
		"ENDGAME_STATS",
		"CANDLE_HELD_STATE",
		"RUNNING_METAL_START",
		"RUNNING_TILE_START",
		"RUNNING_WATER_METAL_START",
		"RUNNING_WATER_TILE_START",
		"RUNNING_STOP_SOUND",
		"IGNITE_CANDLE",
		"UPDATE_SANITY",
		"HIT_BY_SPRINKLER",
		"ENABLE_SPRINKLERS",
		"START_THROWING",
		"STOP_THROWING",
		"EMPTY",
		"COUNT"
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

	class MessageCreatePlayer : public MessageData {
	public:
		MessageCreatePlayer(Netcode::ComponentID playerNetID, Netcode::ComponentID candleNetID, Netcode::ComponentID gunNetID, glm::vec3 pos)
			: playerCompID(playerNetID), candleCompID(candleNetID), gunCompID(gunNetID), position(pos) {
		}
		virtual ~MessageCreatePlayer() {}

		Netcode::ComponentID playerCompID;
		Netcode::ComponentID candleCompID;
		Netcode::ComponentID gunCompID;
		glm::vec3 position;
	};

	class MessageSpawnProjectile : public MessageData {
	public:
		MessageSpawnProjectile(glm::vec3 translation_, glm::vec3 velocity_, 
			Netcode::ComponentID projectileCompID, Netcode::ComponentID ownerComponentID, float frequency)
			: translation(translation_), velocity(velocity_),
			projectileComponentID(projectileCompID), ownerPlayerComponentID(ownerComponentID),
			lowPassFrequency(frequency)
		{}
		virtual ~MessageSpawnProjectile() {}

		glm::vec3 translation;
		glm::vec3 velocity;
		float lowPassFrequency;
		Netcode::ComponentID projectileComponentID;
		Netcode::ComponentID ownerPlayerComponentID;
	};

	class MessageWaterHitPlayer : public MessageData {
	public:
		MessageWaterHitPlayer(Netcode::ComponentID whoWasHit)
			: playerWhoWasHitID(whoWasHit)
		{}
		~MessageWaterHitPlayer() {}

		Netcode::ComponentID playerWhoWasHitID;
	};

	class MessageSetCandleHealth : public MessageData {
	public:
		MessageSetCandleHealth(Netcode::ComponentID candleID, float candleHealth)
			: candleThatWasHit(candleID), health(candleHealth) {
		}
		~MessageSetCandleHealth() {}

		Netcode::ComponentID candleThatWasHit;
		float health;
	};

	class MessageExtinguishCandle : public MessageData {
	public:
		MessageExtinguishCandle(Netcode::ComponentID candleID, Netcode::PlayerID extinguishedBy)
			: candleThatWasHit(candleID), playerWhoExtinguishedCandle(extinguishedBy) {
		}
		~MessageExtinguishCandle() {}

		Netcode::ComponentID candleThatWasHit;
		Netcode::PlayerID playerWhoExtinguishedCandle;
	};

	class MessagePlayerJumped : public MessageData {
	public:
		MessagePlayerJumped(Netcode::ComponentID id) : playerWhoJumped(id) {}
		~MessagePlayerJumped() {}
		Netcode::ComponentID playerWhoJumped;
	};

	class MessagePlayerLanded : public MessageData {
	public:
		MessagePlayerLanded(Netcode::ComponentID id) : playerWhoLanded(id) {}
		~MessagePlayerLanded() {}
		Netcode::ComponentID playerWhoLanded;
	};

	class MessagePlayerDied : public MessageData {
	public:
		MessagePlayerDied(Netcode::ComponentID id, Netcode::PlayerID shooterID) : playerWhoDied(id), playerWhoFired(shooterID) {}
		~MessagePlayerDied() {}
		Netcode::ComponentID playerWhoDied;
		Netcode::PlayerID playerWhoFired;
	};

	class MessageCandleHeldState : public MessageData {
	public:
		MessageCandleHeldState(Netcode::ComponentID id, bool held, glm::vec3 pos) : candleOwnerID(id), isHeld(held), candlePos(pos) {}
		~MessageCandleHeldState() {}
		Netcode::ComponentID candleOwnerID;
		bool isHeld;
		glm::vec3 candlePos;
	};

	class MessageEndGameStats : public MessageData {
	public:
		MessageEndGameStats() {}
		~MessageEndGameStats() {}
	};

	class MessagePrepareEndScreen : public MessageData {
	public:
		MessagePrepareEndScreen() {}
		~MessagePrepareEndScreen() {}

	private:

	};

	class MessageRunningMetalStart : public MessageData {
	public:
		MessageRunningMetalStart(Netcode::ComponentID id) : runningPlayer(id) {}
		~MessageRunningMetalStart() {}
		Netcode::ComponentID runningPlayer;
	};

	class MessageRunningTileStart : public MessageData {
	public:
		MessageRunningTileStart(Netcode::ComponentID id) : runningPlayer(id) {}
		~MessageRunningTileStart() {}
		Netcode::ComponentID runningPlayer;
	};

	class MessageRunningWaterMetalStart : public MessageData {
	public:
		MessageRunningWaterMetalStart(Netcode::ComponentID id) : runningPlayer(id) {}
		~MessageRunningWaterMetalStart() {}
		Netcode::ComponentID runningPlayer;
	};

	class MessageRunningWaterTileStart : public MessageData {
	public:
		MessageRunningWaterTileStart(Netcode::ComponentID id) : runningPlayer(id) {}
		~MessageRunningWaterTileStart() {}
		Netcode::ComponentID runningPlayer;
	};
	class MessageRunningStopSound : public MessageData {
	public:
		MessageRunningStopSound(Netcode::ComponentID id) : runningPlayer(id) {}
		~MessageRunningStopSound() {}
		Netcode::ComponentID runningPlayer;
	};
	class MessageIgniteCandle : public MessageData {
	public:
		MessageIgniteCandle(Netcode::ComponentID candleId) : candleCompId(candleId) {}
		~MessageIgniteCandle() {}
		Netcode::ComponentID candleCompId;
	};
	class MessageInsanityScream : public MessageData {
	public:
		MessageInsanityScream(Netcode::ComponentID id) : screamingPlayer(id) {}
		~MessageInsanityScream() {}
		Netcode::ComponentID screamingPlayer;
	};
	class MessageHitBySprinkler : public MessageData {
	public:
		MessageHitBySprinkler(Netcode::ComponentID id) : candleOwnerID(id) {}
		~MessageHitBySprinkler() {}
		Netcode::ComponentID candleOwnerID;
	};
	class MessageEnableSprinklers : public MessageData {
	public:
		MessageEnableSprinklers() {}
		~MessageEnableSprinklers() {}
	};
	class MessageStartThrowing : public MessageData {
	public:
		MessageStartThrowing(Netcode::ComponentID id) : playerCompID(id) {}
		~MessageStartThrowing() {}
		Netcode::ComponentID playerCompID;
	};
	class MessageStopThrowing : public MessageData {
	public:
		MessageStopThrowing(Netcode::ComponentID id) : playerCompID(id) {}
		~MessageStopThrowing() {}
		Netcode::ComponentID playerCompID;
	};

}
