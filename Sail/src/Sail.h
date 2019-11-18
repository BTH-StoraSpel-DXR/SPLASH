#pragma once

// Settings
//#define _SAIL_BREAK_ON_WARNING
#define _SAIL_BREAK_ON_ERROR

// Disable "Variable uninitialized" warning
// 218 of these in the fbx sdk
#pragma warning(disable:26495)

// TODO: only define GLM_FORCE_DEPTH_ZERO_TO_ONE if directx or vulkan (not opengl)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Sail/KeyCodes.h"
#include "Sail/MouseButtonCodes.h"
#include "Sail/KeyBinds.h"
#include "Sail/Application.h"
#include "Sail/Utils/Utils.h"
#include "Sail/graphics/text/Text.h"
#include "Sail/graphics/text/SailFont.h"
#include "Sail/graphics/camera/PerspectiveCamera.h"
#include "Sail/graphics/camera/OrthographicCamera.h"
#include "Sail/graphics/camera/FlyingCameraController.h"
#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/light/LightSetup.h"
#include "Sail/graphics/shader/material/MaterialShader.h"
#include "Sail/graphics/shader/material/WireframeShader.h"
#include "Sail/graphics/shader/dxr/GBufferOutShader.h"
#include "Sail/graphics/shader/dxr/GBufferWireframe.h"
#include "Sail/graphics/shader/gui/GuiShader.h"
#include "Sail/graphics/geometry/factory/CubeModel.h"
#include "Sail/graphics/geometry/factory/ConeModel.h"
#include "Sail/graphics/geometry/factory/PlaneModel.h"
#include "Sail/states/StateStack.h"
#include "Sail/events/Events.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/graphics/geometry/Transform.h"
#include "Sail/utils/SailImGui/ConsoleCommands.h"
#include "Sail/utils/SailImGui/Profiler.h"
#include "Sail/utils/SailImGui/RenderSettingsWindow.h"
#include "Sail/utils/SailImGui/LightDebugWindow.h"
#include "Sail/utils/SailImGui/PlayerInfoWindow.h"
#include "Sail/utils/SailImGui/WasDroppedWindow.h"
#include "Sail/utils/SailImGui/KillFeedWindow.h"
#include "Sail/utils/SailImGui/ECS_SystemInfoImGuiWindow.h"
#include "Sail/utils/SailImGui/InGameGui.h"
#include "Sail/utils/SailImGui/NetworkInfoWindow.h"
#include "Sail/utils/SailImGui/WaitingForPlayersWindow.h"