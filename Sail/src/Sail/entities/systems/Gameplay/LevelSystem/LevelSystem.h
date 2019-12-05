#pragma once

#include "Sail/entities/systems/BaseComponentSystem.h"

class Scene;
class Model;

struct Rect {
	int posx=0;
	int posy=0;
	int sizex=0;
	int sizey=0;
	int doors = 0;
	bool isCloning = false;
};
struct Clutter {
	float posx;
	float posy;
	float height;
	float rot;
	int size;
};
struct RoomInfo {
	glm::vec3 center;
	glm::vec2 size;
};

enum TileModel {
	ROOM_FLOOR,
	ROOM_WALL,
	ROOM_DOOR,
	ROOM_CEILING,
	ROOM_CORNER,
	ROOM_SERVER,
	CORRIDOR_FLOOR,
	CORRIDOR_WALL,
	CORRIDOR_DOOR,
	CORRIDOR_CEILING,
	CORRIDOR_CORNER,
	NUMBOFMODELS
};

enum ClutterModel {
	SAFTBLANDARE,
	TABLE,
	BOXES,
	MEDIUMBOX,
	SQUAREBOX,
	BOOKS1,
	BOOKS2,
	SCREEN,
	NOTEPAD,
	MICROSCOPE,
	CLONINGVATS,
	CONTROLSTATION,
	NUMBOFCLUTTER
};

enum Direction {
	NONE = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 4,
	LEFT = 8
};

class LevelSystem final: public BaseComponentSystem {
public:
	LevelSystem();
	~LevelSystem();

	void generateMap();
	void createWorld(const std::vector<Model*>& tileModels, Model* bb);
	void destroyWorld();
	void addClutterModel(const std::vector<Model*>& clutterModels, Model* bb);
	glm::vec3 getPowerUpPosition(int index);
	glm::vec3 getSpawnPoint();
	void stop();
	const int getAreaType(float posX, float posY);
	const int getRoomIDFromWorldPos(float posX, float posY);
	const int getRoomID(int posX, int posY);
	const RoomInfo getRoomInfo(int ID);

	int *** getTiles();

#ifdef DEVELOPMENT
	unsigned int getByteSize() const override;
#endif

	int xsize;
	int ysize;
	int*** tileArr; //0 is tileID, 1 is typeID, 2 is door
	float hallwayThreshold; // percentage of level that can be corridors
	int minSplitSize; //minimum size for splitting chunks
	int minRoomSize; //minimum side of a room
	int roomMaxSize;//maximum area of a room
	int roomSplitStop ;//percentage to stop a room from being split into smaller ones
	int doorModifier;//percentage to spawn a door
	int clutterModifier;//percentage to add clutter
	int seed;//seed for generation
	
	int totalArea;
	int numberOfRooms;
	int tileSize;
	float tileHeight;
	int tileOffset;

	std::vector<glm::vec3> spawnPoints;
	std::vector<glm::vec3> extraSpawnPoints;
	std::vector<glm::vec3> powerUpSpawnPoints;
private:
	std::queue<Rect> chunks;
	std::queue<Rect> blocks;
	std::queue<Rect> hallways;
	std::queue<Rect> rooms;
	std::vector<Rect> matched;
	std::queue<Clutter>largeClutter;
	std::queue<Clutter>mediumClutter;
	std::queue<Clutter>smallClutter;
	std::queue<Clutter>specialClutter;
	int randomizeTileId(std::vector<int>* tiles);
	void findPossibleTiles(std::vector<int>* mapPointer,int posx, int posy);
	void splitChunk();
	void splitBlock();
	void matchRoom();
	int checkBorder(Rect rekt);
	bool splitDirection(bool ns);
	void addSpawnPoints();
	void addDoors();
	void addMapModel(Direction dir, int typeID, int doors, const std::vector<Model*>& tileModels, int i, int j, Model* bb);
	void addTile(int tileId, int typeId, int doors,const std::vector<Model*>& tileModels, int i, int j, Model* bb);
	bool hasDoor(Direction dir, int doors);
	void generateClutter();
};