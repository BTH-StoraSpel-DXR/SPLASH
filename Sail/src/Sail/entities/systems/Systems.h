#pragma once
// Ordered by directory name, then file name

#include "Audio/AudioSystem.h"
#include "cameraRecording/CameraRecordSystem.h"

#include "entityManagement/EntityAdderSystem.h"
#include "entityManagement/EntityRemovalSystem.h"

//#include "Gameplay/ai/AiSystem.h"
#include "Gameplay/candles/CandleHealthSystem.h"
#include "Gameplay/candles/CandlePlacementSystem.h"
#include "Gameplay/candles/CandleReignitionSystem.h"
#include "Gameplay/candles/CandleThrowingSystem.h"
#include "Graphics/CrosshairSystem.h"
#include "Gameplay/GunSystem.h"
#include "Gameplay/LevelSystem/LevelSystem.h"
#include "Gameplay/lifetime/LifeTimeSystem.h"
#include "Gameplay/PlayerSystem.h"
#include "Gameplay/ProjectileSystem.h"
#include "Gameplay/sanitySoundSystem.h"
#include "Gameplay/SanitySystem.h"
#include "Gameplay/SprinklerSystem.h"
#include "Gameplay/TeamColorSystem.h"
#include "Gameplay/SprinklerSystem.h"
#include "Graphics/AnimationSystem.h"
#include "Graphics/AnimationChangerSystem.h"
#include "Graphics/ParticleSystem.h"

#include "input/GameInputSystem.h"
#include "input/SprintingSystem.h"
#include "input/SpectateInputSystem.h"

#include "light/LightSystem.h"
#include "light/LightListSystem.h"
#include "light/SpotLightSystem.h"

#include "network/receivers/KillCamReceiverSystem.h"

#include "network/receivers/NetworkReceiverSystem.h"
#include "network/receivers/NetworkReceiverSystemClient.h"
#include "network/receivers/NetworkReceiverSystemHost.h"
#include "network/NetworkSenderSystem.h"
#include "network/HostSendToSpectatorSystem.h"
#include "physics/CollisionSystem.h"
#include "physics/MovementPostCollisionSystem.h"
#include "physics/MovementSystem.h"
#include "physics/OctreeAddRemoverSystem.h"
#include "physics/SpeedLimitSystem.h"
#include "physics/UpdateBoundingBoxSystem.h"

#include "prepareUpdate/PrepareUpdateSystem.h"

#include "render/BeginEndFrameSystem.h"
#include "render/BoundingboxSubmitSystem.h"
#include "render/GUISubmitSystem.h"
#include "render/MetaballSubmitSystem.h"
#include "render/ModelSubmitSystem.h"
#include "render/ImGui/RenderImGuiSystem.h"
