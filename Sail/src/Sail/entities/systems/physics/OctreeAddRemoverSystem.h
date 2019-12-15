#pragma once
#include "..//BaseComponentSystem.h"

class Octree;
class Camera;

template <typename T>
class OctreeAddRemoverSystem final : public BaseComponentSystem
{
public:
	OctreeAddRemoverSystem();
	~OctreeAddRemoverSystem();

	void provideOctree(Octree* octree);

	bool addEntity(Entity* entity) override;

	void removeEntity(Entity* entity) override;

	void update(float dt) override;
	void updatePerFrame(float dt);

	void setCulling(bool activated, Camera* camera);

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif

private:
	Octree* m_octree;
	bool m_doCulling;
	Camera* m_cullCamera;
};