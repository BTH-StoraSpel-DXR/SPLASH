#include "pch.h"
#include "AudioData.h"

void AllAudioData::init() {
#pragma region DEFINING playerEntity SOUNDS

	Audio::SoundInfo_General* soundGeneralInfo = nullptr;
	Audio::SoundInfo_Unique soundUniqueInfo = {};

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Metal		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::RUN_METAL];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, -1.6f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::RUN_METAL].push_back(soundUniqueInfo);

		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::RUN_METAL].push_back(soundUniqueInfo);

		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::RUN_METAL].push_back(soundUniqueInfo);

		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_metal_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::RUN_METAL].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Tiles		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::RUN_TILE];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, -1.6f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_TILE].push_back(soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_TILE].push_back(soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_TILE].push_back(soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_tile_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_TILE].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Water_Metal		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::RUN_WATER_METAL];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, -1.6f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_METAL].push_back(soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_METAL].push_back(soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_METAL].push_back(soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_water_metal_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_METAL].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		 Running, Water_Tiles		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::RUN_WATER_TILE];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, -1.6f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_1.wav";
		soundUniqueInfo.soundEffectLength = 0.919f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_TILE].push_back(soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_2.wav";
		soundUniqueInfo.soundEffectLength = 0.941f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_TILE].push_back(soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_3.wav";
		soundUniqueInfo.soundEffectLength = 0.921f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_TILE].push_back(soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/footsteps_water_tile_4.wav";
		soundUniqueInfo.soundEffectLength = 0.936f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::RUN_WATER_TILE].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Start		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::SHOOT_START];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_start.wav";
		soundUniqueInfo.soundEffectLength = 0.578f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::SHOOT_START].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Loop		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::SHOOT_LOOP];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_loop.wav";
		soundUniqueInfo.soundEffectLength = 1.425f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::SHOOT_LOOP].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, End		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::SHOOT_END];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_end.wav";
		soundUniqueInfo.soundEffectLength = 0.722f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::SHOOT_END].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Watergun, Reload		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::RELOAD];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/watergun_reload.wav";
		soundUniqueInfo.soundEffectLength = 1.246f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::RELOAD].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, Environment	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::WATER_IMPACT_ENVIRONMENT];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_drip_1.wav";
		soundUniqueInfo.soundEffectLength = 0.26f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/water_drip_2.wav";
		soundUniqueInfo.soundEffectLength = 0.299f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/water_drip_3.wav";
		soundUniqueInfo.soundEffectLength = 0.26f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/water_drip_4.wav";
		soundUniqueInfo.soundEffectLength = 0.207f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #5
		soundUniqueInfo.fileName = "../Audio/water_drip_5.wav";
		soundUniqueInfo.soundEffectLength = 0.406f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #6
		soundUniqueInfo.fileName = "../Audio/water_drip_6.wav";
		soundUniqueInfo.soundEffectLength = 0.463f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
		//	• Sample #7
		soundUniqueInfo.fileName = "../Audio/water_drip_7.wav";
		soundUniqueInfo.soundEffectLength = 0.593f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENVIRONMENT].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, Enemy Candle		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::WATER_IMPACT_ENEMY_CANDLE];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_impact_enemy_candle.wav";
		soundUniqueInfo.soundEffectLength = 0.523f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_ENEMY_CANDLE].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Water Impact, My Candle		     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::WATER_IMPACT_MY_CANDLE];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/water_impact_my_candle.wav";
		soundUniqueInfo.soundEffectLength = 1.364f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::WATER_IMPACT_MY_CANDLE].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+//
	//		   Jump	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::JUMP];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/jump.wav";
		soundUniqueInfo.soundEffectLength = 0.806f;
		soundUniqueInfo.volume = 0.7f;
		AllAudioData::m_soundsUnique[Audio::JUMP].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Landing, Ground		 //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::LANDING_GROUND];
	soundGeneralInfo->playOnce = false;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/landing_ground.wav";
		soundUniqueInfo.soundEffectLength = 0.602f;
		soundUniqueInfo.volume = 0.4f;
		AllAudioData::m_soundsUnique[Audio::LANDING_GROUND].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Death, Me	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::DEATH_ME];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/death_me_1.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
		//	• Sample #2
		soundUniqueInfo.fileName = "../Audio/death_me_2.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
		//	• Sample #3
		soundUniqueInfo.fileName = "../Audio/death_me_3.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
		//	• Sample #4
		soundUniqueInfo.fileName = "../Audio/death_me_4.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
		//	• Sample #5
		soundUniqueInfo.fileName = "../Audio/death_me_5.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
		//	• Sample #6
		soundUniqueInfo.fileName = "../Audio/death_me_6.wav";
		soundUniqueInfo.soundEffectLength = 4.013f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_ME].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------

	// +-+-+-+-+-+-+-+-+-+-+-+-+-+//
	//		   Death, Other	     //   FIN_1.0
	// +-+-+-+-+-+-+-+-+-+-+-+-+//
	soundGeneralInfo = &AllAudioData::m_sounds[Audio::DEATH_OTHER];
	soundGeneralInfo->playOnce = true;
	soundGeneralInfo->positionalOffset = { 0.0f, 0.0f, 0.0f };
	//-----------------------------------------------------------------------------
	{
		//	• Sample #1
		soundUniqueInfo.fileName = "../Audio/death_enemy.wav";
		soundUniqueInfo.soundEffectLength = 3.578f;
		soundUniqueInfo.volume = 1.0f;
		AllAudioData::m_soundsUnique[Audio::DEATH_OTHER].push_back(soundUniqueInfo);
	}
	//-----------------------------------------------------------------------------
#pragma endregion
}
