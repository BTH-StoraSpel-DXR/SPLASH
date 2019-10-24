#pragma once

#include "..//BaseComponentSystem.h"

class Scene;
class Model;
struct Rect;
struct Clutter;

enum TileModel {
	ROOM_FLOOR,
	ROOM_WALL,
	ROOM_DOOR,
	ROOM_CEILING,
	ROOM_CORNER,
	CORRIDOR_FLOOR,
	CORRIDOR_WALL,
	CORRIDOR_DOOR,
	CORRIDOR_CEILING,
	CORRIDOR_CORNER,
	NUMBOFMODELS
};

enum ClutterModel {
	CLUTTER_LO,
	CLUTTER_MO,
	CLUTTER_SO,
	NUMBOFCLUTTER
};

enum Direction {
	NONE = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 4,
	LEFT = 8
};

class LevelGeneratorSystem final: public BaseComponentSystem {
public:
	LevelGeneratorSystem();
	~LevelGeneratorSystem();

	void generateMap();
	void createWorld(const std::vector<Model*>& tileModels, Model* bb);
	void addClutterModel(const std::vector<Model*>& clutterModels, Model* bb);

	glm::vec3 getSpawnPoint();

private:
	int randomizeTileId(std::vector<int>* tiles);
	void findPossibleTiles(std::vector<int>* mapPointer,int posx, int posy);
	void splitChunk();
	void splitBlock();
	void matchRoom();
	int checkBorder(Rect rekt);
	bool splitDirection(bool ns);
	void addSpawnPoints();
	void addDoors();
	void addMapModel(Direction dir, int typeID, int doors, const std::vector<Model*>& tileModels, float tileSize,float tileHeight, int tileOffset, int i, int j, Model* bb);
	void addTile(int tileId, int typeId, int doors,const std::vector<Model*>& tileModels, float tileSize,float tileHeight, float tileOffset, int i, int j, Model* bb);
	bool hasDoor(Direction dir, int doors);
	void generateClutter();
};