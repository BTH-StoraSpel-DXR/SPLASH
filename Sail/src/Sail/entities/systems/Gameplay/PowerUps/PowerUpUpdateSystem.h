#pragma once
#include "../../BaseComponentSystem.h"


class PowerUpUpdateSystem final : public BaseComponentSystem {
public:
	PowerUpUpdateSystem();
	~PowerUpUpdateSystem();

	void update(float dt) override;

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif
private:

};