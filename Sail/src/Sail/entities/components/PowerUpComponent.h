#pragma once
#pragma once

#include "Component.h"
#include "Sail/entities/Entity.h"

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

	enum PowerUps {
		RUNSPEED,
		STAMINA,
		SHOWER,
		POWERWASH,
		NUMPOWUPS
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