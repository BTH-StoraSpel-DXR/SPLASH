#pragma once

#ifdef _DEBUG
constexpr size_t TICKRATE = 32;
#else
constexpr size_t TICKRATE = 64;
#endif


//const float TIMESTEP = 1.0f / (5.0f*TICKRATE); // if you want slow-motion
constexpr float TIMESTEP = 1.0f / static_cast<float>(TICKRATE);