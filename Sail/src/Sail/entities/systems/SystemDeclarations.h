#pragma once

#include "../components/RenderInActiveGameComponent.h"
#include "../components/RenderInReplayComponent.h"

// Ordered by name

class AnimationChangerSystem;
class AudioSystem;
class AiSystem;
class BeginEndFrameSystem;
class BoundingboxSubmitSystem;
class CandleHealthSystem;
class CandlePlacementSystem;
class CandleReignitionSystem;
class CollisionSystem;
class EndFrameSystem;
class EntityAdderSystem;
class EntityRemovalSystem;
class GameInputSystem;
class GUISubmitSystem;
class GunSystem;
class HostSendToSpectatorSystem;
class LevelGeneratorSystem;
class KillCamReceiverSystem;
class LevelSystem;
class LifeTimeSystem;
class LightListSystem;
class NetworkReceiverSystem;
class NetworkSenderSystem;
class OctreeAddRemoverSystem;
class ParticleSystem;
class PlayerSystem;
class PrepareUpdateSystem;
class ProjectileSystem;
class RenderImGuiSystem;
class SpectateInputSystem;
class SanitySoundSystem;
class SanitySystem;
class SpeedLimitSystem;
class SprinklerSystem;
class UpdateBoundingBoxSystem;
class SpotLightSystem;
class SprintingSystem;
class TeamColorSystem;
class CandleThrowingSystem;
class CrosshairSystem;


// Systems that need duplicate versions for the killcam (due to slow motion and other reasons)
template <typename T>
class AnimationSystem;
template <typename T>
class LightSystem;
template <typename T>
class MetaballSubmitSystem;
template <typename T>
class ModelSubmitSystem;
template <typename T>
class MovementSystem;
template <typename T>
class MovementPostCollisionSystem;

struct Systems {
	AnimationChangerSystem*    animationChangerSystem    = nullptr;
	AudioSystem*               audioSystem               = nullptr;
	AiSystem*				   aiSystem					 = nullptr;
	BeginEndFrameSystem*       beginEndFrameSystem       = nullptr;
	BoundingboxSubmitSystem*   boundingboxSubmitSystem   = nullptr;
	CandleHealthSystem*        candleHealthSystem        = nullptr;
	CandlePlacementSystem*     candlePlacementSystem     = nullptr;
	CandleReignitionSystem*    candleReignitionSystem    = nullptr;
	CandleThrowingSystem*      candleThrowingSystem      = nullptr;
	CollisionSystem*           collisionSystem           = nullptr;
	CrosshairSystem*           crosshairSystem           = nullptr;
	EndFrameSystem*            endFrameSystem            = nullptr;
	EntityAdderSystem*         entityAdderSystem         = nullptr;
	EntityRemovalSystem*       entityRemovalSystem       = nullptr;
	GameInputSystem*           gameInputSystem           = nullptr;
	GUISubmitSystem*           guiSubmitSystem           = nullptr;
	GunSystem*                 gunSystem                 = nullptr;
	HostSendToSpectatorSystem* hostSendToSpectatorSystem = nullptr;
	KillCamReceiverSystem*     killCamReceiverSystem     = nullptr;
	LevelSystem*               levelSystem               = nullptr;
	LifeTimeSystem*            lifeTimeSystem            = nullptr;
	LightListSystem*           lightListSystem           = nullptr;
	NetworkReceiverSystem*     networkReceiverSystem     = nullptr;
	NetworkSenderSystem*       networkSenderSystem       = nullptr;
	OctreeAddRemoverSystem*    octreeAddRemoverSystem    = nullptr;
	ParticleSystem*            particleSystem            = nullptr;
	PlayerSystem*              playerSystem              = nullptr;
	PrepareUpdateSystem*       prepareUpdateSystem       = nullptr;
	ProjectileSystem*          projectileSystem          = nullptr;
	RenderImGuiSystem*         renderImGuiSystem         = nullptr;
	SpectateInputSystem*       spectateInputSystem       = nullptr;
	SanitySoundSystem*         sanitySoundSystem         = nullptr;
	SanitySystem*              sanitySystem              = nullptr;
	SpeedLimitSystem*          speedLimitSystem          = nullptr;
	SpotLightSystem*           spotLightSystem           = nullptr;
	TeamColorSystem*           teamColorSystem           = nullptr;
	SprinklerSystem*           sprinklerSystem           = nullptr;
	UpdateBoundingBoxSystem*   updateBoundingBoxSystem   = nullptr;
	SprintingSystem*           sprintingSystem           = nullptr;


	// Systems that need duplicate versions for the killcam
	AnimationSystem<RenderInActiveGameComponent>*             animationSystem                    = nullptr;
	AnimationSystem<RenderInReplayComponent>*                 killCamAnimationSystem             = nullptr;
	LightSystem<RenderInActiveGameComponent>*                 lightSystem                        = nullptr;
	LightSystem<RenderInReplayComponent>*                     killCamLightSystem                 = nullptr;
	MetaballSubmitSystem<RenderInActiveGameComponent>*        metaballSubmitSystem               = nullptr;
	MetaballSubmitSystem<RenderInReplayComponent>*            killCamMetaballSubmitSystem        = nullptr;
	ModelSubmitSystem<RenderInActiveGameComponent>*           modelSubmitSystem                  = nullptr;
	ModelSubmitSystem<RenderInReplayComponent>*               killCamModelSubmitSystem           = nullptr;
	MovementSystem<RenderInActiveGameComponent>*              movementSystem                     = nullptr;
	MovementSystem<RenderInReplayComponent>*                  killCamMovementSystem              = nullptr;
	MovementPostCollisionSystem<RenderInActiveGameComponent>* movementPostCollisionSystem        = nullptr;
	MovementPostCollisionSystem<RenderInReplayComponent>*     killCamMovementPostCollisionSystem = nullptr;
};