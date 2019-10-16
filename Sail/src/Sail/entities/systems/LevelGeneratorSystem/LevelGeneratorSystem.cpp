#include "pch.h"
#include "LevelGeneratorSystem.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/components/MapComponent.h"
#include <random>

LevelGeneratorSystem::LevelGeneratorSystem():BaseComponentSystem() {
	registerComponent<MapComponent>(true,true,true);
	registerComponent<ModelComponent>(false, false, true);
	registerComponent<TransformComponent>(false, true, true);
	registerComponent<BoundingBoxComponent>(false, false, true);
	registerComponent<CollidableComponent>(false, false, true);
}

LevelGeneratorSystem::~LevelGeneratorSystem() {
}

//generates all necessary data for the world
void LevelGeneratorSystem::generateMap() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		srand(map->seed);
		std::vector<int> avaliableTiles;

		//create floor to split for map generation
		rect floor;
		floor.posx = 0;
		floor.posy = 0;
		floor.sizex = map->xsize;
		floor.sizey = map->ysize;
		map->chunks.emplace(floor);

		//create blocks and hallways
		splitChunk();

		//add hallways to type-layer
		while (!map->hallways.empty()) {
			rect tile;
			tile = map->hallways.front();
			map->hallways.pop();
			for (int i = 0; i < tile.sizex; i++) {
				for (int j = 0; j < tile.sizey; j++) {
					map->tileArr[tile.posx + i][tile.posy + j][1] = 0;
				}
			}
		}

		//create rooms from blocks
		splitBlock();
		//add rooms with individual type to type-layer
		while (!map->rooms.empty()) {
			rect tile;
			tile = map->rooms.front();
			map->rooms.pop();
			for (int i = 0; i < tile.sizex; i++) {
				for (int j = 0; j < tile.sizey; j++) {
					map->tileArr[tile.posx + i][tile.posy + j][1] = map->numberOfRooms;
				}
			}
			map->matched.emplace(tile);
			map->numberOfRooms++;
		}

		//adds doors to the layout
		addDoors();

		// Find spawn points
		addSpawnPoints();

		//creates tilemap for the level
		matchRoom();
	}
}

void LevelGeneratorSystem::createWorld(const std::vector<Model*>& tileModels, Model* bb) {
	
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();

		int worldWidth = map->xsize;
		int worldDepth = map->ysize;

		//traverse all positions to find which tile should be there
		for (int i = 0; i < worldWidth; i++) {
			for (int j = 0; j < worldDepth; j++) {
				int tileId = map->tileArr[i][j][0];
				int typeId = map->tileArr[i][j][1];
				int doors = map->tileArr[i][j][2];
				if (tileId<16 && tileId>-1) {
					addTile(tileId, typeId, doors, tileModels, map->tileSize, map->tileOffset, i, j, bb);
				}
			}
		}
	}
}

//chooses a random tile from all possible tiles that fit
int LevelGeneratorSystem::randomizeTileId(std::vector<int>* tiles) {
	if (tiles->size() > 0) {
		//srand(seed);
		return tiles->operator[](rand() % tiles->size());
	}
	else {
		return -1;
	}
}

//finds a matching tile in a labyrinth implementation
void LevelGeneratorSystem::findPossibleTiles(std::vector<int>* mapPointer, int posx, int posy) {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		
		int tiles[4];
		tiles[0]=map->tileArr[posx][posy + 1][0];
		tiles[1]= map->tileArr[posx+1][posy][0];
		tiles[2] = map->tileArr[posx][posy-1][0];
		tiles[3] = map->tileArr[posx - 1][posy][0];

		int nrOfRules = 0;
		std::vector<int> temp;
		int match = 0;
		for (int i = 0; i < 4; i++) {
			if (tiles[i] != -1) {
				nrOfRules++;
			}
		}
		for (int i = 0; i < 16; i++) {
			if (tiles[0] != -1) {
				if (((tiles[0] % 8) / 4) == (i % 2)) {
					temp.emplace_back(i);
				}
			}
			if (tiles[1] != -1) {
				if ((tiles[1] / 8)==((i % 4) / 2)) {
					temp.emplace_back(i);
				}
			}
			if (tiles[2] != -1) {
				if (((i%8)/4)==(tiles[2]%2)) {
					temp.emplace_back(i);
				}
			}
			if (tiles[3] != -1) {
				if (((tiles[3]% 4) / 2) == (i/ 8)) {
					temp.emplace_back(i);
				}
			}
		}
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < temp.size(); j++) {
				if (i == temp[j]) {
					match++;
				}
			}
			if (match >= nrOfRules) {
				mapPointer->emplace_back(i);
				//less t and cross
				if (i !=7 &&i!=11&&i!=13&&i!=14&&i!=15) {
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
				}
				//more flat
				if (i == 0) {
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
				}
				//more straight
				if(i==5||i==10){
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
					mapPointer->emplace_back(i);
				}
			}
			match = 0;
		}
	}
}

//splits chunks to make hallways and blocks
void LevelGeneratorSystem::splitChunk() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		bool ns = true;
		float hallwayArea = 0.f;
		while (hallwayArea / map->totalArea < map->hallwayThreshold && !map->chunks.empty()) {
			rect rekt,a,b,hall;
			rekt = map->chunks.front();
			map->chunks.pop();
			if (ns) {
				if (rekt.sizex > map->minSplitSize) {
					int newSize = rand() % (rekt.sizex - map->minSplitSize) + map->minSplitSize/2;
					a.posx = rekt.posx;
					a.posy = rekt.posy;
					a.sizex = newSize;
					a.sizey = rekt.sizey;
					map->chunks.emplace(a);
					b.posy = rekt.posy;
					b.posx = rekt.posx + newSize + 1;
					b.sizex = rekt.sizex - 1 - newSize;
					b.sizey = rekt.sizey;
					map->chunks.emplace(b);
					hall.posx = rekt.posx + newSize;
					hall.posy = rekt.posy;
					hall.sizex = 1;
					hall.sizey = rekt.sizey;
					map->hallways.emplace(hall);
					hallwayArea += hall.sizex * hall.sizey;
				}
				else if(rekt.sizey <= map->minSplitSize){
					map->blocks.emplace(rekt);
					ns = !ns;
				}
				else {
					map->chunks.emplace(rekt);
				}
				ns = !ns;
			}
			else {
				if (rekt.sizey > map->minSplitSize) {
					int newSize = rand() % (rekt.sizey - map->minSplitSize) + map->minSplitSize/2;
					a.posx = rekt.posx;
					a.posy = rekt.posy;
					a.sizex = rekt.sizex;
					a.sizey = newSize;
					map->chunks.emplace(a);
					b.posy = rekt.posy + newSize + 1;
					b.posx = rekt.posx;
					b.sizex = rekt.sizex;
					b.sizey = rekt.sizey - 1 - newSize;
					map->chunks.emplace(b);
					hall.posx = rekt.posx ;
					hall.posy = rekt.posy + newSize;
					hall.sizex = rekt.sizex;
					hall.sizey =1;
					map->hallways.emplace(hall);
					hallwayArea += hall.sizex * hall.sizey;
				}
				else if (rekt.sizex <= map->minSplitSize) {
					map->blocks.emplace(rekt);
					ns = !ns;
				}
				else {
					map->chunks.emplace(rekt);
				}
				ns = !ns;

			}
		}
		while (!map->chunks.empty()) {
			rect rekt = map->chunks.front();
			map->chunks.pop();
			map->blocks.emplace(rekt);
		}
	}
}

//splits blocks to make rooms
void LevelGeneratorSystem::splitBlock() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();

		while (!map->blocks.empty()) {
			bool isSplit = false,ns=true;
			rect rekt = map->blocks.front();
			if (rand() % 100 > map->roomSplitStop||rekt.sizex*rekt.sizey>map->roomMaxSize) {
				if (rekt.sizex > rekt.sizey) {
					ns = true;
				}
				else {
					ns = false;
				}
				isSplit = splitDirection(ns);
				if (isSplit) {
					map->blocks.pop();
				}
			}
			else {
				map->rooms.emplace(rekt);
				map->blocks.pop();
			}
		}
	}
}

//traverses every tile and matches it to the "areas" next to it, to see if there should be a wall.
void LevelGeneratorSystem::matchRoom() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		int tiles[4];
		for (int i = 0; i < map->xsize; i++) {
			for (int j = 0; j < map->ysize; j++) {
				//check typeID around current position to find tileID 
				if (i - 1 < 0) {
					tiles[0] = -1;
				}
				else {
					tiles[0] = map->tileArr[i - 1][j][1];
				}
				if (j + 1 >= map->ysize) {
					tiles[1] = -1;
				}
				else {
					tiles[1] = map->tileArr[i][j + 1][1];
				}
				if (i + 1 >= map->xsize) {
					tiles[2] = -1;
				}
				else {
					tiles[2] = map->tileArr[i + 1][j][1];
				}
				if (j - 1 < 0) {
					tiles[3] = -1;
				}
				else {
					tiles[3] = map->tileArr[i][j - 1][1];
				}
				//match and add value for walls
				if (tiles[0] != map->tileArr[i][j][1]) {
					map->tileArr[i][j][0] += 8;
				}
				if (tiles[1] != map->tileArr[i][j][1]) {
					map->tileArr[i][j][0] += 1;
				}
				if (tiles[2] != map->tileArr[i][j][1]) {
					map->tileArr[i][j][0] += 2;
				}
				if (tiles[3] != map->tileArr[i][j][1]) {
					map->tileArr[i][j][0] += 4;
				}
				if (map->tileArr[i][j][1] == -1) {
					map->tileArr[i][j][0] = -1;
				}
			}
		}
	
	}
}

//checks the borders of a rect to see if it borders a corridor.
//Returns an int which holds all directions in which to find a corridor
int LevelGeneratorSystem::checkBorder(rect rekt) {
	bool top = false; 
	bool bottom = false;
	bool left = false;
	bool right = false;
	int corridor = 0;
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		for (int i = 0; i < rekt.sizex; i++) {
			if (rekt.posx + i >= 0 && rekt.posx + i < map->xsize) {
				if (rekt.posy > 0) {
					if (map->tileArr[rekt.posx + i][rekt.posy - 1][1] == 0) {
						bottom = true;
					}
				}
				if (rekt.posy + rekt.sizey + 1 < map->ysize) {
					if (map->tileArr[rekt.posx + i][rekt.posy + rekt.sizey ][1] == 0) {
						top = true;
					}
				}
			}
		}

		for (int i = 0; i < rekt.sizey; i++) {
			if (rekt.posy + i >= 0 && rekt.posy + i < map->ysize) {
				if (rekt.posx > 0) {
					if (map->tileArr[rekt.posx - 1][rekt.posy + i][1] == 0) {
						left = true;
					}
				}
				if (rekt.posx + rekt.sizex + 1 < map->xsize) {
					if (map->tileArr[rekt.posx + rekt.sizex][rekt.posy + i][1] == 0) {
						right = true;
					}
				}
			}
		}
	}
	if (bottom) {
		corridor += 4;
	}
	if (top) {
		corridor += 1;
	}
	if (right) {
		corridor += 2;
	}
	if (left) {
		corridor += 8;
	}
	return corridor;
}

//Splits a block in a direction while making sure both new blocks are next to a corridor.
//If they're not, it tries to split the block the other way. If that also fails, the block is added to the rooms.
bool LevelGeneratorSystem::splitDirection(bool ns) {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		rect rekt, a, b;
		rekt = map->blocks.front();
		if (ns) {
			if (rekt.sizex >= map->minRoomSize * 2) {
				int newSize = 0;
				if (rekt.sizex == 2 * map->minRoomSize) {
					newSize = map->minRoomSize;
				}
				else if (rekt.sizex == 2 * map->minRoomSize + 1) {
					newSize = rand() % 2 + map->minRoomSize;
				}
				else {
					newSize = rand() % (rekt.sizex - 2 * map->minRoomSize) + map->minRoomSize;
				}
				a.posx = rekt.posx;
				a.posy = rekt.posy;
				a.sizex = newSize;
				a.sizey = rekt.sizey;
				b.posx = rekt.posx + newSize;
				b.posy = rekt.posy;
				b.sizex = rekt.sizex - newSize;
				b.sizey = rekt.sizey;
				if (checkBorder(a)>0 && checkBorder(b)>0) {
					map->blocks.emplace(a);
					map->blocks.emplace(b);
					return true;
				}
				else {
					if (rekt.sizey >= map->minRoomSize * 2) {
						newSize = 0;
						if (rekt.sizey == 2 * map->minRoomSize) {
							newSize = map->minRoomSize;
						}
						else if (rekt.sizey == 2 * map->minRoomSize + 1) {
							newSize = rand() % 2 + map->minRoomSize;
						}
						else {
							newSize = rand() % (rekt.sizey - 2 * map->minRoomSize) + map->minRoomSize;
						}
						a.posx = rekt.posx;
						a.posy = rekt.posy;
						a.sizex = rekt.sizex;
						a.sizey = newSize;
						b.posx = rekt.posx;
						b.posy = rekt.posy+newSize;
						b.sizex = rekt.sizex;
						b.sizey = rekt.sizey - newSize;
						if (checkBorder(a)>0 && checkBorder(b)>0) {
							map->blocks.emplace(a);
							map->blocks.emplace(b);
							return true;
						}
						else {
							map->rooms.emplace(rekt);
							return true;
						}
					}
					else {
						map->rooms.emplace(rekt);
						return true;
					}
				}

			}
			else {
				if (rekt.sizey >= map->minRoomSize * 2) {
					int newSize = 0;
					if (rekt.sizey == 2 * map->minRoomSize) {
						newSize = map->minRoomSize;
					}
					else if (rekt.sizey == 2 * map->minRoomSize + 1) {
						newSize = rand() % 2 + map->minRoomSize;
					}
					else {
						newSize = rand() % (rekt.sizey - 2 * map->minRoomSize) + map->minRoomSize;
					}
					a.posx = rekt.posx;
					a.posy = rekt.posy;
					a.sizex = rekt.sizex;
					a.sizey = newSize;
					b.posx = rekt.posx;
					b.posy = rekt.posy + newSize;
					b.sizex = rekt.sizex;
					b.sizey = rekt.sizey - newSize;
					if (checkBorder(a)>0 && checkBorder(b)>0) {
						map->blocks.emplace(a);
						map->blocks.emplace(b);
						return true;
					}
					else {
						map->rooms.emplace(rekt);
						return true;
					}
				}
				else {
					map->rooms.emplace(rekt);
					return true;
				}

			}
		}
		if (!ns) {
			if (rekt.sizey >= map->minRoomSize * 2) {
				int newSize = 0;
				if (rekt.sizey == 2*map->minRoomSize) {
					newSize = map->minRoomSize;
				}
				else if (rekt.sizey == 2*map->minRoomSize+1) {
					newSize = rand() % 2 + map->minRoomSize;
				}
				else {
					newSize = rand() % (rekt.sizey - 2 * map->minRoomSize) + map->minRoomSize;
				}
				a.posx = rekt.posx;
				a.posy = rekt.posy;
				a.sizex = rekt.sizex;
				a.sizey = newSize;
				b.posx = rekt.posx;
				b.posy = rekt.posy + newSize;
				b.sizex = rekt.sizex ;
				b.sizey = rekt.sizey - newSize;
				if (checkBorder(a)>0 && checkBorder(b)>0) {
					map->blocks.emplace(a);
					map->blocks.emplace(b);
					return true;
				}
				else {
					if (rekt.sizex >= map->minRoomSize * 2) {
						newSize = 0;
						if (rekt.sizex == 2 * map->minRoomSize) {
							newSize = map->minRoomSize;
						}
						else if (rekt.sizex == 2 * map->minRoomSize + 1) {
							newSize = rand() % 2 + map->minRoomSize;
						}
						else {
							newSize = rand() % (rekt.sizex - 2 * map->minRoomSize) + map->minRoomSize;
						}
						a.posx = rekt.posx;
						a.posy = rekt.posy;
						a.sizex = newSize;
						a.sizey = rekt.sizey;
						b.posx = rekt.posx+newSize;
						b.posy = rekt.posy;
						b.sizex = rekt.sizex-newSize;
						b.sizey = rekt.sizey;
						if (checkBorder(a)>0 && checkBorder(b)>0) {
							map->blocks.emplace(a);
							map->blocks.emplace(b);
							return true;
						}
						else {
							map->rooms.emplace(rekt);
							return true;
						}
					}
					else {
						map->rooms.emplace(rekt);
						return true;
					}
				}

			}
			else {
				if (rekt.sizex >= map->minRoomSize * 2) {
					int newSize = 0;
					if (rekt.sizex == 2 * map->minRoomSize) {
						newSize = map->minRoomSize;
					}
					else if (rekt.sizex == 2 * map->minRoomSize + 1) {
						newSize = rand() % 2 + map->minRoomSize;
					}
					else {
						newSize = rand() % (rekt.sizex - 2 * map->minRoomSize) + map->minRoomSize;
					}
					a.posx = rekt.posx;
					a.posy = rekt.posy;
					a.sizex = newSize;
					a.sizey = rekt.sizey;
					b.posx = rekt.posx+newSize;
					b.posy = rekt.posy;
					b.sizex = rekt.sizex-newSize;
					b.sizey = rekt.sizey;
					if (checkBorder(a)>0 && checkBorder(b)>0) {
						map->blocks.emplace(a);
						map->blocks.emplace(b);
						return true;
					}
					else {
						map->rooms.emplace(rekt);
						return true;
					}
				}
				else {
					map->rooms.emplace(rekt);
					return true;
				}

			}

		}
	}
	return false;
}

void LevelGeneratorSystem::addDoors() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		
		size_t maxSize = map->matched.size();

		//add one door to each room, which leads to a corridor
		for (int c = 0; c < maxSize; c++) {
			rect currentTile = map->matched.front();
			std::vector<rect> possibleDoors;
			if (currentTile.doors == 0) {
				for (int i = 0; i < currentTile.sizex; i++) {
					for (int j = 0; j < currentTile.sizey; j++) {
						rect doorTile;
						doorTile.posx = currentTile.posx + i;
						doorTile.posy = currentTile.posy + j;
						doorTile.sizex = 1;
						doorTile.sizey = 1;
						doorTile.sizex = checkBorder(doorTile);
						if (doorTile.sizex > 0) {
							if (hasDoor(Direction::LEFT, doorTile.sizex)) {
								doorTile.sizey = Direction::LEFT;
								possibleDoors.emplace_back(doorTile);
							}
							if (hasDoor(Direction::DOWN, doorTile.sizex)) {
								doorTile.sizey = Direction::DOWN;
								possibleDoors.emplace_back(doorTile);
							}
							if (hasDoor(Direction::RIGHT, doorTile.sizex)) {
								doorTile.sizey = Direction::RIGHT;
								possibleDoors.emplace_back(doorTile);
							}
							if (hasDoor(Direction::UP, doorTile.sizex)) {
								doorTile.sizey = Direction::UP;
								possibleDoors.emplace_back(doorTile);
							}
						}
					}
				}
				// Pick one of the possible doors at random
				if (possibleDoors.size() > 0) {
					rect door = possibleDoors[rand() % possibleDoors.size()];
					possibleDoors.clear();
					if (door.sizey == Direction::UP) {
						map->tileArr[door.posx][door.posy][2] += Direction::UP;
						map->tileArr[door.posx][door.posy + 1][2] += Direction::DOWN;
						currentTile.doors += Direction::UP;
					}
					else if (door.sizey == Direction::RIGHT) {
						map->tileArr[door.posx][door.posy][2] += Direction::RIGHT;
						map->tileArr[door.posx+1][door.posy][2] += Direction::LEFT;
						currentTile.doors += Direction::RIGHT;
					}
					else if (door.sizey == Direction::DOWN) {
						map->tileArr[door.posx][door.posy][2] += Direction::DOWN;
						map->tileArr[door.posx][door.posy-1][2] += Direction::UP;
						currentTile.doors += Direction::DOWN;
					}
					else if (door.sizey == Direction::LEFT) {
						map->tileArr[door.posx][door.posy][2] += Direction::LEFT;
						map->tileArr[door.posx - 1][door.posy][2] += Direction::RIGHT;
						currentTile.doors += Direction::LEFT;
					}
				}
			}
			map->matched.pop();
			map->matched.emplace(currentTile);
		}

		// Adds a second door to each room
		for (int c = 0; c < maxSize; c++) {
			std::vector<rect> possibleDoors;
			rect currentTile = map->matched.front();
			int doorCounter = 0;
			bool up = false, right = false, down = false, left = false;
			// Check each room which walls have doors, and how many
			for (int i = 0; i < currentTile.sizex; i++) {
				for (int j = 0; j < currentTile.sizey; j++) {
					int doors = map->tileArr[currentTile.posx + i][currentTile.posy + j][2];
					if (hasDoor(Direction::UP, doors)) {
						up = true;
						doorCounter++;
					}
					if(hasDoor(Direction::RIGHT, doors)){
						right = true;
						doorCounter++;
					}
					if (hasDoor(Direction::DOWN, doors)) {
						down = true;
						doorCounter++;
					}
					if (hasDoor(Direction::LEFT, doors)) {
						left = true;
						doorCounter++;
					}
				}
			}
			// If the room does not have atleast 2 doors, we add one more on a wall that does not have a door already
			if (doorCounter < 2) {
				if (currentTile.posx - 1 >= 0 && !left) {
					for (int i = 0; i < currentTile.sizey; i++) {
						rect doorTile;
						doorTile.posx = currentTile.posx;
						doorTile.posy = currentTile.posy + i;
						doorTile.sizex = 1;
						doorTile.sizey = 8;
						possibleDoors.emplace_back(doorTile);
					}
				}
				if (currentTile.posx + currentTile.sizex + 1 < map->xsize && !right) {
					for (int i = 0; i < currentTile.sizey; i++) {
						rect doorTile;
						doorTile.posx = currentTile.posx + currentTile.sizex - 1;
						doorTile.posy = currentTile.posy + i;
						doorTile.sizex = 1;
						doorTile.sizey = 2;
						possibleDoors.emplace_back(doorTile);
					}
				}
				if (currentTile.posy - 1 >= 0 && !down) {
					for (int i = 0; i < currentTile.sizex; i++) {
						rect doorTile;
						doorTile.posx = currentTile.posx + i;
						doorTile.posy = currentTile.posy;
						doorTile.sizex = 1;
						doorTile.sizey = 4;
						possibleDoors.emplace_back(doorTile);
					}
				}
				if (currentTile.posy + currentTile.sizey + 1 < map->ysize && !up) {
					for (int i = 0; i < currentTile.sizex; i++) {
						rect doorTile;
						doorTile.posx = currentTile.posx + i;
						doorTile.posy = currentTile.posy + currentTile.sizey - 1;
						doorTile.sizex = 1;
						doorTile.sizey = 1;
						possibleDoors.emplace_back(doorTile);
					}
				}
				// Pick one of the possible doors at random
				if (possibleDoors.size() > 0) {
					rect door = possibleDoors[rand() % possibleDoors.size()];
					possibleDoors.clear();
					if (door.sizey == Direction::UP) {
						map->tileArr[door.posx][door.posy][2] += Direction::UP;
						map->tileArr[door.posx][door.posy + 1][2] += Direction::DOWN;
						currentTile.doors += Direction::UP;
					}
					if (door.sizey == Direction::RIGHT) {
						map->tileArr[door.posx][door.posy][2] += Direction::RIGHT;
						map->tileArr[door.posx + 1][door.posy][2] += Direction::LEFT;
						currentTile.doors += Direction::RIGHT;
					}
					if (door.sizey == Direction::DOWN) {
						map->tileArr[door.posx][door.posy][2] += Direction::DOWN;
						map->tileArr[door.posx][door.posy - 1][2] += Direction::UP;
						currentTile.doors += Direction::DOWN;
					}
					if (door.sizey == Direction::LEFT) {
						map->tileArr[door.posx][door.posy][2] += Direction::LEFT;
						map->tileArr[door.posx - 1][door.posy][2] += Direction::RIGHT;
						currentTile.doors += Direction::LEFT;
					}
				}
			}
			map->matched.pop();
			map->matched.push(currentTile);
		}
	}
}

bool LevelGeneratorSystem::hasDoor(Direction dir, int doors) {
	if (dir == Direction::UP && doors % 2 == Direction::UP) {
		return true;
	}
	else if(dir == Direction::DOWN && doors % 8 >= Direction::DOWN){
		return true;
	}
	else if (dir == Direction::RIGHT && doors % 4 >= Direction::RIGHT) {
		return true;
	}
	else if (dir == Direction::LEFT && doors >= Direction::LEFT) {
		return true;
	}
	return false;
}

void LevelGeneratorSystem::addMapModel(Direction dir, int typeID, int doors, const std::vector<Model*>& tileModels, float tileSize, int tileOffset, int i, int j, Model* bb) {
	if (dir == Direction::RIGHT) {
		if (hasDoor(Direction::RIGHT, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));

			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));

			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
		}
	}
	else if (dir == Direction::UP) {
		if (hasDoor(Direction::UP, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
				}
		}
	}
	else if (dir == Direction::DOWN) {
		if (hasDoor(Direction::DOWN, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
				}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
		}
	}
	else if (dir == Direction::LEFT) {
		if (hasDoor(Direction::LEFT, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			}
		}
	}
	else
	{
		if (typeID == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_FLOOR], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CEILING], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
		}
		else {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_FLOOR], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_FLOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, 1.0f, tileSize / 10.f));
		}
	}
}

void LevelGeneratorSystem::addTile(int tileId, int typeId, int doors,const std::vector<Model*>& tileModels, float tileSize, float tileOffset, int i, int j, Model* bb) {

	switch (tileId)
	{
	case 1:
		/*
		
		Adding tile type:
		x---------x




		x         x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 2:	
		/*
		
		Adding tile type:
		x         x
				  |
				  |
				  |
				  |
		x         x
		
		*/
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 3:
		/*
		
		Adding tile type:
		x---------x
				  |
				  |
				  |
				  |
		x         x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 4:
		/*
		
		Adding tile type:
		x         x




		x---------x
		
		*/
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 5:
		/*
		
		Adding tile type:
		x---------x




		x---------x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 6:
		/*
		
		Adding tile type:
		x         x
				  |
				  |
				  |
				  |
		x---------x
		
		*/
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 7:
		/*
		
		Adding tile type:
		x---------x
				  |
				  |
				  |
				  |
		x---------x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 8:
		/*
		
		Adding tile type:
		x         x
		|
		|
		|
		|
		x         x
		
		*/
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 9:
		/*
		
		Adding tile type:
		x---------x
		|
		|
		|
		|
		x         x

		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 10:
		/*
		
		Adding tile type:
		x         x
		|		  |
		|		  |
		|		  |
		|		  |
		x         x

		*/
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 11:
		/*
		
		Adding tile type:
		x---------x
		|		  |
		|		  |
		|		  |
		|		  |
		x         x

		*/
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 12:
		/*
		
		Adding tile type:
		x         x
		|
		|
		|
		|
		x---------x

		*/
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 13:
		/*
		
		Adding tile type:
		x---------x
		|
		|
		|
		|
		x---------x

		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 14:
		/*
	
		Adding tile type:
		x         x
		|		  |
		|		  |
		|		  |
		|		  |
		x---------x

		*/
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	case 15:
		/*

		Adding tile type:
		x---------x
		|		  |
		|		  |
		|		  |
		|		  |
		x---------x

		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);
		break;
	default:
		break;
	}
	// Attaching floor
	addMapModel(Direction::NONE, typeId, doors, tileModels, tileSize, tileOffset, i, j, bb);

}

void LevelGeneratorSystem::addSpawnPoints() {
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();

		std::vector<glm::vec3> availableSpawnPoints;

		// Room IDs
		int roomBottomLeft = map->tileArr[0][0][1];
		int roomTopLeft = map->tileArr[0][map->ysize - 1][1];
		int roomBottomRight = map->tileArr[map->xsize - 1][0][1];
		int roomTopRight = map->tileArr[map->xsize - 1][map->ysize - 1][1];
		int roomsLeftEdge = 0;
		int roomsBottomEdge = 0;
		int roomsRightEdge = 0;
		int roomsTopEdge = 0;

		// Adding all corner spawn location first
		map->spawnPoints.push_back(glm::vec3(0.f, 0.f, 0.f));
		map->spawnPoints.push_back(glm::vec3(((map->xsize - 1) * map->tileSize), 0.f, ((map->ysize - 1) * map->tileSize)));
		map->spawnPoints.push_back(glm::vec3(0.f, 0.f, ((map->ysize - 1) * map->tileSize)));
		map->spawnPoints.push_back(glm::vec3(((map->xsize - 1) * map->tileSize), 0.f, 0.f));

		// Find all available spawn locations, one in each room
		for (int x = 0; x < map->xsize; x++) {
			// Get all rooms on the bottom edge of the map
			if (map->tileArr[x][0][1] > 0 && map->tileArr[x][0][1] != roomsBottomEdge && map->tileArr[x][0][1] != roomBottomLeft && map->tileArr[x][0][1] != roomBottomRight) {
				availableSpawnPoints.push_back(glm::vec3(x * map->tileSize, 0.f, 0.f));
				roomsBottomEdge = map->tileArr[x][0][1];
			}
			// Get all rooms on the top edge of the map
			if (map->tileArr[x][map->ysize - 1][1] > 0 && map->tileArr[x][map->ysize - 1][1] != roomsTopEdge && map->tileArr[x][map->ysize - 1][1] != roomTopLeft && map->tileArr[x][map->ysize - 1][1] != roomTopRight) {
				availableSpawnPoints.push_back(glm::vec3((x * map->tileSize), 0.f, ((map->ysize - 1) * map->tileSize)));
				roomsTopEdge = map->tileArr[x][map->ysize - 1][1];
			}
		}

		// Get all rooms for the right and left edge of the map, except for the corner rooms
		for (int y = 0; y < map->ysize; y++) {
			if (map->tileArr[0][y][1] > 0 && map->tileArr[0][y][1] != roomsLeftEdge && map->tileArr[0][y][1] != roomBottomLeft && map->tileArr[0][y][1] != roomTopLeft){
				availableSpawnPoints.push_back(glm::vec3(0.f, 0.f, (y * map->tileSize)));
				roomsLeftEdge = map->tileArr[0][y][1];
			}
			if (map->tileArr[map->xsize-1][y][1] > 0 && map->tileArr[map->xsize - 1][y][1] != roomsRightEdge && map->tileArr[map->xsize - 1][y][1] != roomBottomRight && map->tileArr[map->xsize - 1][y][1] != roomTopRight) {
				availableSpawnPoints.push_back(glm::vec3((map->xsize * map->tileSize), 0.f, (y * map->tileSize)));
				roomsRightEdge = map->tileArr[map->xsize - 1][y][1];
			}
		}

		std::default_random_engine generator;
		
		// Add the rest of the spawn points in a randomized order
		while(availableSpawnPoints.size() > 0){
			std::uniform_int_distribution<int> distribution(0, availableSpawnPoints.size() - 1);
			int randomRoom = distribution(generator);
			map->spawnPoints.push_back(availableSpawnPoints[randomRoom]);
			availableSpawnPoints.erase(availableSpawnPoints.begin() + randomRoom);
		}

	}
}


glm::vec3 LevelGeneratorSystem::getSpawnPoint() {
	// Gets the spawn points with the 4 corners first, then randomized spawn points around the edges of the map
	glm::vec3 spawnLocation = glm::vec3(-1000.f);
	for (auto& e : entities) {
		MapComponent* map = e->getComponent<MapComponent>();
		
		if (map->spawnPoints.size() > 0) {
			spawnLocation = map->spawnPoints.front();
			map->spawnPoints.erase(map->spawnPoints.begin());
		}
		else {
			Logger::Error("No more spawn locations available.");
		}

	}

	return spawnLocation;
}