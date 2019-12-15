#pragma once
#pragma once

#include "../Component.h"
#include "Sail/entities/Entity.h"
enum PowerUps {
	RUNSPEED,
	STAMINA,
	SHOWER,
	POWERWASH,
	NUMPOWUPS
};

// For colors
namespace PowerUpColorTeams {
	static const int teams[] = {
		2, // RUNSPEED  (green)
		3, // STAMINA   (yellow)
		0, // SHOWER    (blue)
		1  // POWERWASH (red)
	};
}


class PowerUpComponent : public Component<PowerUpComponent> {
public:
	struct PowerUp {
		float time = 0;
		float maxTime = 60;
		std::string name = "";
		void addTime(const float _time) {
			time += _time;
			if (time > maxTime) {
				time = maxTime;
			}
		}
	};



public:
	PowerUpComponent();
	~PowerUpComponent();

#ifdef DEVELOPMENT
	const unsigned int getByteSize() const override {
		int size = sizeof(*this);
		for (auto& pow : powerUps) {
			size += sizeof(PowerUp);
			size += sizeof(char)*pow.name.size();
		}
		return size;
	}
	void imguiRender(Entity** selected);
#endif

public:


	std::vector<PowerUp> powerUps;




};