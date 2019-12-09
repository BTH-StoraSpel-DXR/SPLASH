#include "pch.h"
#include "LevelSystem.h"
#include "Sail/Application.h"
#include "Sail/entities/ECS.h"
#include "Sail/entities/components/Components.h"
#include "Sail/entities/components/MapComponent.h"
#include <random>

LevelSystem::LevelSystem():BaseComponentSystem() {
	registerComponent<MapComponent>(true,true,true);
	registerComponent<ModelComponent>(false, false, true);
	registerComponent<TransformComponent>(false, true, true);
	registerComponent<BoundingBoxComponent>(false, false, true);
	registerComponent<CollidableComponent>(false, false, true);
	xsize = 1;
	ysize = 1;
	tileSize = 7;
	hallwayThreshold = 0.3f;
	minSplitSize = 5;
	minRoomSize = 1;
	roomMaxSize = 36;
	roomSplitStop = 25;
	doorModifier = 15;
	clutterModifier = 85;
	seed = 0;
	tileArr = nullptr;

	numberOfRooms = 0;
	tileHeight = 0.8f;
	tileOffset = 0;
}

LevelSystem::~LevelSystem() {
	destroyWorld();
}

//generates all necessary data for the world
void LevelSystem::generateMap() {
	numberOfRooms = 0;
	totalArea = xsize * ysize;
	if (!tileArr) {
		tileArr = SAIL_NEW int** [xsize]();
		for (int i = 0; i < xsize; i++) {
			tileArr[i] = SAIL_NEW int* [ysize]();
			for (int j = 0; j < ysize; j++) {
				tileArr[i][j] = SAIL_NEW int[3]();
				tileArr[i][j][1] = -1;
			}
		}
	}

	srand(seed);
	std::vector<int> avaliableTiles;

	//create floor to split for map generation
	Rect floor;
	floor.posx = 0;
	floor.posy = 0;
	floor.sizex = xsize;
	floor.sizey = ysize;
	chunks.emplace(floor);

	//create blocks and hallways
	splitChunk();

	//add hallways to type-layer
	while (!hallways.empty()) {
		Rect tile;
		tile = hallways.front();
		hallways.pop();
		for (int i = 0; i < tile.sizex; i++) {
			for (int j = 0; j < tile.sizey; j++) {
				tileArr[tile.posx + i][tile.posy + j][1] = 0;
			}
		}
	}

	//create rooms from blocks
	splitBlock();
	//add rooms with individual type to type-layer
	while(!rooms.empty()){
		Rect tile;
		tile = rooms.front();
		rooms.pop();
		numberOfRooms++;
		for (int i = 0; i < tile.sizex; i++) {
			for (int j = 0; j < tile.sizey; j++) {
				tileArr[tile.posx + i][tile.posy + j][1] = numberOfRooms;
			}
		}
		matched.push_back(tile);
	}

	//adds doors to the layout
	addDoors();


	//creates tilemap for the level
	matchRoom();
	
}

void LevelSystem::createWorld(const std::vector<Model*>& tileModels, Model* bb) {
	//traverse all positions to find which tile should be there
	for (int i = 0; i < xsize; i++) {
		for (int j = 0; j < ysize; j++) {
			int tileId = tileArr[i][j][0];
			int typeId = tileArr[i][j][1];
			int doors = tileArr[i][j][2];
			if (tileId<16 && tileId>-1) {
				addTile(tileId, typeId, doors, tileModels, i, j, bb);
			}
		}
	}

	//fills rooms with stuff
	generateClutter();

	// Find spawn points
	addSpawnPoints();
}

void LevelSystem::destroyWorld() {
	if (tileArr) {
		for (int i = 0; i < xsize; i++) {
			for (int j = 0; j < ysize; j++) {
				Memory::SafeDeleteArr(tileArr[i][j]);
			}
			Memory::SafeDeleteArr(tileArr[i]);
		}
		Memory::SafeDeleteArr(tileArr);
	}
	spawnPoints.clear();
	extraSpawnPoints.clear();
	powerUpSpawnPoints.clear();
	while(chunks.size()>0){
		chunks.pop();
	}
	while (blocks.size() > 0) {
		blocks.pop();
	}

	while (hallways.size() > 0) {
		hallways.pop();
	}

	while (rooms.size() > 0) {
		rooms.pop();
	}

	while (matched.size() > 0) {
		matched.clear();
	}

	while (largeClutter.size() > 0) {
		largeClutter.pop();
	}

	while (mediumClutter.size() > 0) {
		mediumClutter.pop();
	}

	while (smallClutter.size() > 0) {
		smallClutter.pop();
	}
}

//chooses a random tile from all possible tiles that fit
int LevelSystem::randomizeTileId(std::vector<int>* tiles) {
	if (tiles->size() > 0) {
		//srand(seed);
		return tiles->operator[](rand() % tiles->size());
	}
	else {
		return -1;
	}
}

//finds a matching tile in a labyrinth implementation
void LevelSystem::findPossibleTiles(std::vector<int>* mapPointer, int posx, int posy) {

		
	int tiles[4];
	tiles[0]=tileArr[posx][posy + 1][0];
	tiles[1]= tileArr[posx+1][posy][0];
	tiles[2] = tileArr[posx][posy-1][0];
	tiles[3] = tileArr[posx - 1][posy][0];

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

//splits chunks to make hallways and blocks
void LevelSystem::splitChunk() {

	bool ns = true;
	float hallwayArea = 0.f;
	while (hallwayArea / totalArea < hallwayThreshold && !chunks.empty()) {
		Rect rekt, a, b, hall;
		rekt = chunks.front();
		chunks.pop();
#ifdef _PERFORMANCE_TEST
		if (ns) {
#else
		if (rekt.sizex >= rekt.sizey) {
#endif
			if (rekt.sizex > minSplitSize) {
				int newSize = rand() % (rekt.sizex - minSplitSize) + minSplitSize / 2;
				a.posx = rekt.posx;
				a.posy = rekt.posy;
				a.sizex = newSize;
				a.sizey = rekt.sizey;
				chunks.emplace(a);
				b.posy = rekt.posy;
				b.posx = rekt.posx + newSize + 1;
				b.sizex = rekt.sizex - 1 - newSize;
				b.sizey = rekt.sizey;
				chunks.emplace(b);
				hall.posx = rekt.posx + newSize;
				hall.posy = rekt.posy;
				hall.sizex = 1;
				hall.sizey = rekt.sizey;
				hallways.emplace(hall);
				hallwayArea += hall.sizex * hall.sizey;
			}
			else if (rekt.sizey <= minSplitSize) {
				blocks.emplace(rekt);
				ns = !ns;
			}
			else {
				chunks.emplace(rekt);
			}
			ns = !ns;
		}
		else {
			if (rekt.sizey > minSplitSize) {
				int newSize = rand() % (rekt.sizey - minSplitSize) + minSplitSize / 2;
				a.posx = rekt.posx;
				a.posy = rekt.posy;
				a.sizex = rekt.sizex;
				a.sizey = newSize;
				chunks.emplace(a);
				b.posy = rekt.posy + newSize + 1;
				b.posx = rekt.posx;
				b.sizex = rekt.sizex;
				b.sizey = rekt.sizey - 1 - newSize;
				chunks.emplace(b);
				hall.posx = rekt.posx;
				hall.posy = rekt.posy + newSize;
				hall.sizex = rekt.sizex;
				hall.sizey = 1;
				hallways.emplace(hall);
				hallwayArea += hall.sizex * hall.sizey;
			}
			else if (rekt.sizex <= minSplitSize) {
				blocks.emplace(rekt);
				ns = !ns;
			}
			else {
				chunks.emplace(rekt);
			}
			ns = !ns;

		}

		if (hall.sizex > 1 || hall.sizey > 1) {
			glm::vec3 powerPos = glm::vec3((hall.posx + hall.sizex / 2.f - 0.5f) * tileSize, 0.5f, (hall.posy + hall.sizey / 2.f - 0.5f) * tileSize);
			powerUpSpawnPoints.push_back(powerPos);
		}
	}
	while (!chunks.empty()) {
		Rect rekt = chunks.front();
		chunks.pop();
		blocks.emplace(rekt);
	}
}

//splits blocks to make rooms
void LevelSystem::splitBlock() {

	while (!blocks.empty()) {
		bool isSplit = false,ns=true;
		Rect rekt = blocks.front();
		if (rand() % 100 > roomSplitStop||rekt.sizex*rekt.sizey>roomMaxSize) {
			if (rekt.sizex > rekt.sizey) {
				ns = true;
			}
			else {
				ns = false;
			}
			isSplit = splitDirection(ns);
			if (isSplit) {
				blocks.pop();
			}
		}
		else {
			rooms.emplace(rekt);
			blocks.pop();
		}
	}
}

//traverses every tile and matches it to the "areas" next to it, to see if there should be a wall.
void LevelSystem::matchRoom() {
	int tiles[4];
	for (int i = 0; i < xsize; i++) {
		for (int j = 0; j < ysize; j++) {
			//check typeID around current position to find tileID 
			if (i - 1 < 0) {
				tiles[0] = -1;
			}
			else {
				tiles[0] = tileArr[i - 1][j][1];
			}
			if (j + 1 >= ysize) {
				tiles[1] = -1;
			}
			else {
				tiles[1] = tileArr[i][j + 1][1];
			}
			if (i + 1 >= xsize) {
				tiles[2] = -1;
			}
			else {
				tiles[2] = tileArr[i + 1][j][1];
			}
			if (j - 1 < 0) {
				tiles[3] = -1;
			}
			else {
				tiles[3] = tileArr[i][j - 1][1];
			}
			//match and add value for walls
			if (tiles[0] != tileArr[i][j][1]) {
				tileArr[i][j][0] += 8;
			}
			if (tiles[1] != tileArr[i][j][1]) {
				tileArr[i][j][0] += 1;
			}
			if (tiles[2] != tileArr[i][j][1]) {
				tileArr[i][j][0] += 2;
			}
			if (tiles[3] != tileArr[i][j][1]) {
				tileArr[i][j][0] += 4;
			}
			if (tileArr[i][j][1] == -1) {
				tileArr[i][j][0] = -1;
			}
		}
	}
}

//checks the borders of a rect to see if it borders a corridor.
//Returns an int which holds all directions in which to find a corridor
int LevelSystem::checkBorder(Rect rekt) {
	bool top = false; 
	bool bottom = false;
	bool left = false;
	bool right = false;
	int corridor = 0;
	for (int i = 0; i < rekt.sizex; i++) {
		if (rekt.posx + i >= 0 && rekt.posx + i < xsize) {
			if (rekt.posy > 0) {
				if (tileArr[rekt.posx + i][rekt.posy - 1][1] == 0) {
					bottom = true;
				}
			}
			if (rekt.posy + rekt.sizey + 1 < ysize) {
				if (tileArr[rekt.posx + i][rekt.posy + rekt.sizey ][1] == 0) {
					top = true;
				}
			}
		}
	}

	for (int i = 0; i < rekt.sizey; i++) {
		if (rekt.posy + i >= 0 && rekt.posy + i < ysize) {
			if (rekt.posx > 0) {
				if (tileArr[rekt.posx - 1][rekt.posy + i][1] == 0) {
					left = true;
				}
			}
			if (rekt.posx + rekt.sizex + 1 < xsize) {
				if (tileArr[rekt.posx + rekt.sizex][rekt.posy + i][1] == 0) {
					right = true;
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
bool LevelSystem::splitDirection(bool ns) {
	Rect rekt, a, b;
	rekt = blocks.front();
	if (ns) {
		if (rekt.sizex >= minRoomSize * 2) {
			int newSize = 0;
			if (rekt.sizex == 2 * minRoomSize) {
				newSize = minRoomSize;
			}
			else if (rekt.sizex == 2 * minRoomSize + 1) {
				newSize = rand() % 2 + minRoomSize;
			}
			else {
				newSize = rand() % (rekt.sizex - 2 * minRoomSize) + minRoomSize;
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
				blocks.emplace(a);
				blocks.emplace(b);
				return true;
			}
			else {
				if (rekt.sizey >= minRoomSize * 2) {
					newSize = 0;
					if (rekt.sizey == 2 * minRoomSize) {
						newSize = minRoomSize;
					}
					else if (rekt.sizey == 2 * minRoomSize + 1) {
						newSize = rand() % 2 + minRoomSize;
					}
					else {
						newSize = rand() % (rekt.sizey - 2 * minRoomSize) + minRoomSize;
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
						blocks.emplace(a);
						blocks.emplace(b);
						return true;
					}
					else {
						rooms.emplace(rekt);
						return true;
					}
				}
				else {
					rooms.emplace(rekt);
					return true;
				}
			}

		}
		else {
			if (rekt.sizey >= minRoomSize * 2) {
				int newSize = 0;
				if (rekt.sizey == 2 * minRoomSize) {
					newSize = minRoomSize;
				}
				else if (rekt.sizey == 2 * minRoomSize + 1) {
					newSize = rand() % 2 + minRoomSize;
				}
				else {
					newSize = rand() % (rekt.sizey - 2 * minRoomSize) + minRoomSize;
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
					blocks.emplace(a);
					blocks.emplace(b);
					return true;
				}
				else {
					rooms.emplace(rekt);
					return true;
				}
			}
			else {
				rooms.emplace(rekt);
				return true;
			}

		}
	}
	if (!ns) {
		if (rekt.sizey >= minRoomSize * 2) {
			int newSize = 0;
			if (rekt.sizey == 2*minRoomSize) {
				newSize = minRoomSize;
			}
			else if (rekt.sizey == 2*minRoomSize+1) {
				newSize = rand() % 2 + minRoomSize;
			}
			else {
				newSize = rand() % (rekt.sizey - 2 * minRoomSize) + minRoomSize;
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
				blocks.emplace(a);
				blocks.emplace(b);
				return true;
			}
			else {
				if (rekt.sizex >= minRoomSize * 2) {
					newSize = 0;
					if (rekt.sizex == 2 * minRoomSize) {
						newSize = minRoomSize;
					}
					else if (rekt.sizex == 2 * minRoomSize + 1) {
						newSize = rand() % 2 + minRoomSize;
					}
					else {
						newSize = rand() % (rekt.sizex - 2 * minRoomSize) + minRoomSize;
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
						blocks.emplace(a);
						blocks.emplace(b);
						return true;
					}
					else {
						rooms.emplace(rekt);
						return true;
					}
				}
				else {
					rooms.emplace(rekt);
					return true;
				}
			}

		}
		else {
			if (rekt.sizex >= minRoomSize * 2) {
				int newSize = 0;
				if (rekt.sizex == 2 * minRoomSize) {
					newSize = minRoomSize;
				}
				else if (rekt.sizex == 2 * minRoomSize + 1) {
					newSize = rand() % 2 + minRoomSize;
				}
				else {
					newSize = rand() % (rekt.sizex - 2 * minRoomSize) + minRoomSize;
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
					blocks.emplace(a);
					blocks.emplace(b);
					return true;
				}
				else {
					rooms.emplace(rekt);
					return true;
				}
			}
			else {
				rooms.emplace(rekt);
				return true;
			}

		}

	}
	return false;
}

void LevelSystem::addDoors() {
		
	size_t maxSize = matched.size();

	//add one door to each room, which leads to a corridor
	for (int c = 0; c < maxSize; c++) {
		Rect currentTile = matched.at(c);
		std::vector<Rect> possibleDoors;
		if (currentTile.doors == 0) {
			for (int i = 0; i < currentTile.sizex; i++) {
				for (int j = 0; j < currentTile.sizey; j++) {
					Rect doorTile;
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
				Rect door = possibleDoors[rand() % possibleDoors.size()];
				possibleDoors.clear();
				if (door.sizey == Direction::UP) {
					tileArr[door.posx][door.posy][2] += Direction::UP;
					tileArr[door.posx][door.posy + 1][2] += Direction::DOWN;
					currentTile.doors += Direction::UP;
				}
				else if (door.sizey == Direction::RIGHT) {
					tileArr[door.posx][door.posy][2] += Direction::RIGHT;
					tileArr[door.posx+1][door.posy][2] += Direction::LEFT;
					currentTile.doors += Direction::RIGHT;
				}
				else if (door.sizey == Direction::DOWN) {
					tileArr[door.posx][door.posy][2] += Direction::DOWN;
					tileArr[door.posx][door.posy-1][2] += Direction::UP;
					currentTile.doors += Direction::DOWN;
				}
				else if (door.sizey == Direction::LEFT) {
					tileArr[door.posx][door.posy][2] += Direction::LEFT;
					tileArr[door.posx - 1][door.posy][2] += Direction::RIGHT;
					currentTile.doors += Direction::LEFT;
				}
			}
		}
	}

	// Adds a second door to each room
	for (int c = 0; c < maxSize; c++) {
		std::vector<Rect> possibleDoors;
		Rect currentTile = matched.at(c);
		int doorCounter = 0;
		bool up = false, right = false, down = false, left = false;
		// Check each room which walls have doors, and how many
		for (int i = 0; i < currentTile.sizex; i++) {
			for (int j = 0; j < currentTile.sizey; j++) {
				int doors = tileArr[currentTile.posx + i][currentTile.posy + j][2];
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
					Rect doorTile;
					doorTile.posx = currentTile.posx;
					doorTile.posy = currentTile.posy + i;
					doorTile.sizex = 1;
					doorTile.sizey = 8;
					possibleDoors.emplace_back(doorTile);
				}
			}
			if (currentTile.posx + currentTile.sizex + 1 < xsize && !right) {
				for (int i = 0; i < currentTile.sizey; i++) {
					Rect doorTile;
					doorTile.posx = currentTile.posx + currentTile.sizex - 1;
					doorTile.posy = currentTile.posy + i;
					doorTile.sizex = 1;
					doorTile.sizey = 2;
					possibleDoors.emplace_back(doorTile);
				}
			}
			if (currentTile.posy - 1 >= 0 && !down) {
				for (int i = 0; i < currentTile.sizex; i++) {
					Rect doorTile;
					doorTile.posx = currentTile.posx + i;
					doorTile.posy = currentTile.posy;
					doorTile.sizex = 1;
					doorTile.sizey = 4;
					possibleDoors.emplace_back(doorTile);
				}
			}
			if (currentTile.posy + currentTile.sizey + 1 < ysize && !up) {
				for (int i = 0; i < currentTile.sizex; i++) {
					Rect doorTile;
					doorTile.posx = currentTile.posx + i;
					doorTile.posy = currentTile.posy + currentTile.sizey - 1;
					doorTile.sizex = 1;
					doorTile.sizey = 1;
					possibleDoors.emplace_back(doorTile);
				}
			}
			// Pick one of the possible doors at random
			if (possibleDoors.size() > 0) {
				Rect door = possibleDoors[rand() % possibleDoors.size()];
				possibleDoors.clear();
				if (door.sizey == Direction::UP) {
					tileArr[door.posx][door.posy][2] += Direction::UP;
					tileArr[door.posx][door.posy + 1][2] += Direction::DOWN;
					currentTile.doors += Direction::UP;
				}
				if (door.sizey == Direction::RIGHT) {
					tileArr[door.posx][door.posy][2] += Direction::RIGHT;
					tileArr[door.posx + 1][door.posy][2] += Direction::LEFT;
					currentTile.doors += Direction::RIGHT;
				}
				if (door.sizey == Direction::DOWN) {
					tileArr[door.posx][door.posy][2] += Direction::DOWN;
					tileArr[door.posx][door.posy - 1][2] += Direction::UP;
					currentTile.doors += Direction::DOWN;
				}
				if (door.sizey == Direction::LEFT) {
					tileArr[door.posx][door.posy][2] += Direction::LEFT;
					tileArr[door.posx - 1][door.posy][2] += Direction::RIGHT;
					currentTile.doors += Direction::LEFT;
				}
			}
		}

	}
	
}

bool LevelSystem::hasDoor(Direction dir, int doors) {
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

void LevelSystem::addMapModel(Direction dir, int typeID, int doors, const std::vector<Model*>& tileModels, int i, int j, Model* bb) {
	if (dir == Direction::RIGHT) {
		if (hasDoor(Direction::RIGHT, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));

			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));

			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				if (rand() % 4 == 0) {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_SERVER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
				else {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
			}
		}
	}
	else if (dir == Direction::UP) {
		if (hasDoor(Direction::UP, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				if (rand() % 4 == 0) {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_SERVER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
				else {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
			}
		}
	}
	else if (dir == Direction::DOWN) {
		if (hasDoor(Direction::DOWN, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				if (rand() % 4 == 0) {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_SERVER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
				else {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
			}
		}
	}
	else if (dir == Direction::LEFT) {
		if (hasDoor(Direction::LEFT, doors)) {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_DOOR], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
		}
		else {
			if (typeID == 0) {
				EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			}
			else {
				if (rand() % 4 == 0) {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_SERVER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
				else {
					EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_WALL], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
				}
			}
		}
	}
	else
	{
		if (typeID == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_FLOOR], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CEILING], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_FLOOR], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CEILING], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
	}
}


void LevelSystem::addTile(int tileId, int typeId, int doors, const std::vector<Model*>& tileModels, int i, int j, Model* bb) {

	addMapModel(Direction::NONE, typeId, doors, tileModels, i, j, bb);
	switch (tileId)
	{
	case 0:
		/*
		
		Adding tile type:
		x         x




		x         x
		
		*/
		if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));

		}
		break;
	case 1:
		/*
		
		Adding tile type:
		x---------x




		x         x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f,glm::radians(270.f),0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}else if(typeId==0){
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f,glm::radians(90.f),0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
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
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

		break;
	case 4:
		/*
		
		Adding tile type:
		x         x




		x---------x
		
		*/
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

		break;
	case 5:
		/*
		
		Adding tile type:
		x---------x




		x---------x
		
		*/
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
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
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(270.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize* i + tileOffset, 0.f, tileSize* j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(180.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}



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
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}
		else if (typeId == 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::CORRIDOR_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}


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
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(90.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}

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
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		if (typeId != 0) {
			EntityFactory::CreateStaticMapObject("Map_tile", tileModels[TileModel::ROOM_CORNER], bb, glm::vec3(tileSize * i + tileOffset, 0.f, tileSize * j + tileOffset), glm::vec3(0.f, glm::radians(0.f), 0.f), glm::vec3(tileSize / 10.f, tileHeight, tileSize / 10.f));
		}


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
		addMapModel(Direction::UP, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::RIGHT, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::DOWN, typeId, doors, tileModels, i, j, bb);
		addMapModel(Direction::LEFT, typeId, doors, tileModels, i, j, bb);
		break;
	default:
		break;
	}
}

void LevelSystem::addSpawnPoints() {
	
	std::vector<glm::vec3> availableSpawnPoints;
	for (int i = 0; i < numberOfRooms; i++) {
		Rect room = matched.at(i);
		if (!room.isCloning) {
			float posx = (room.sizex / 2.f + room.posx) * tileSize + tileOffset - tileSize / 2.f;
			float posy = (room.sizey / 2.f + room.posy) * tileSize + tileOffset - tileSize / 2.f;
			availableSpawnPoints.push_back(glm::vec3(posx, 0.f, posy));
		}
	}
	std::default_random_engine generator (seed);
	
	// Add the rest of the spawn points in a randomized order
	while(availableSpawnPoints.size() > 0){
		std::uniform_int_distribution<int> distribution(0, availableSpawnPoints.size() - 1);
		int randomRoom = distribution(generator);
		spawnPoints.push_back(availableSpawnPoints[randomRoom]);
		availableSpawnPoints.erase(availableSpawnPoints.begin() + randomRoom);
	}
	while (extraSpawnPoints.size() > 0) {
		availableSpawnPoints.push_back(extraSpawnPoints.at(0));
		extraSpawnPoints.erase(extraSpawnPoints.begin());
	}
	while (availableSpawnPoints.size() > 0) {
		std::uniform_int_distribution<int> distribution(0, availableSpawnPoints.size() - 1);
		int randomRoom = distribution(generator);
		extraSpawnPoints.push_back(availableSpawnPoints[randomRoom]);
		availableSpawnPoints.erase(availableSpawnPoints.begin() + randomRoom);
	}
}

glm::vec3 LevelSystem::getPowerUpPosition(int index) {
	if (index < 0 || index >= powerUpSpawnPoints.size()) {
		SAIL_LOG_ERROR("Invalid powerup position.");
		return glm::vec3(-30.f, -30.f, -30.f);
	}
	else {
		return powerUpSpawnPoints.at(index);
	}
}

glm::vec3 LevelSystem::getSpawnPoint() {
	// Gets the spawn points with the 4 corners first, then randomized spawn points around the edges of the map
	glm::vec3 spawnLocation;
	if (spawnPoints.size() > 0) {
		spawnLocation = spawnPoints.front();
		spawnPoints.erase(spawnPoints.begin());
	}
	else if (extraSpawnPoints.size() > 0) {
		spawnLocation = extraSpawnPoints.front();
		extraSpawnPoints.erase(extraSpawnPoints.begin());
	}
	else {
		spawnLocation = glm::vec3(((tileSize / 2.f) + ((Utils::rnd() * (tileSize - 1.f) * 2.f) - (tileSize - 1.f))) * (xsize - 1), 1.f, ((tileSize / 2.f) + ((Utils::rnd() * (tileSize - 1.f) * 2.f) - (tileSize - 1.f))) * (ysize - 1));
		SAIL_LOG_ERROR("No more spawn locations available.");
	}
	return spawnLocation;
}

const int LevelSystem::getAreaType(float posX, float posY) {

	AreaType returnValue;


	int roomValue = getRoomIDFromWorldPos(posX, posY);

	if (roomValue == 0) {
		returnValue = AreaType::CORRIDOR;
	}
	else if (roomValue > 0) {
		returnValue = AreaType::ROOM;
	}

	return static_cast<int>(returnValue);
}

const int LevelSystem::getRoomIDFromWorldPos(float posX, float posY) {
	posX += (0.5f * (float)tileSize);
	posY += (0.5f * (float)tileSize);
	posX /= (float)tileSize;
	posY /= (float)tileSize;

	return getRoomID(static_cast<int>(posX), static_cast<int>(posY));
}

const int LevelSystem::getRoomID(int posX, int posY) {
	posX = posX >= xsize ? xsize - 1 : posX;
	posX = posX < 0 ? 0 : posX;
	posY = posY >= ysize ? ysize - 1 : posY;
	posY = posY < 0 ? 0 : posY;
	return tileArr[posX][posY][1];
	
}

const RoomInfo LevelSystem::getRoomInfo(int ID) {
	RoomInfo info;
	Rect room;
	for (int i = 0; i < matched.size(); i++) {
		room = matched[i];
		if (tileArr[room.posx][room.posy][1] == ID) {
			break;
		}
	}

	info.center = glm::vec3((room.posx + (room.sizex / 2.f) - 0.5f) * tileSize, 0.f, (room.posy + (room.sizey / 2.f) - 0.5f) * tileSize);
	info.size = glm::vec2(room.sizex, room.sizey);

	return info;
}

int *** LevelSystem::getTiles() {
	return tileArr;
}


#ifdef DEVELOPMENT
unsigned int LevelSystem::getByteSize() const {
	unsigned int size = BaseComponentSystem::getByteSize() + sizeof(*this);
	
	size += spawnPoints.size() * sizeof(glm::vec3);
	size += chunks.size() * sizeof(Rect);
	size += blocks.size() * sizeof(Rect);
	size += hallways.size() * sizeof(Rect);
	size += rooms.size() * sizeof(Rect);
	size += matched.size() * sizeof(Rect);
	size += largeClutter.size() * sizeof(Clutter);
	size += mediumClutter.size() * sizeof(Clutter);
	size += smallClutter.size() * sizeof(Clutter);
	size += specialClutter.size() * sizeof(Clutter);

	size += xsize * ysize * sizeof(int) * 3;
	return size;
}
#endif

void LevelSystem::stop() {
	destroyWorld();
	spawnPoints.clear();
	extraSpawnPoints.clear();
	powerUpSpawnPoints.clear();
}

void LevelSystem::generateClutter() {

	//adds clutter for each tile in each room if rand() is over threshold value
	for (int i = 0; i < numberOfRooms; i++) {
		Rect room = matched.at(i);
		int makeRoomCloningChance = rand() % 100;
		if (false) {}
#ifndef _PERFORMANCE_TEST
		else if (room.sizex > 2 && makeRoomCloningChance > 60 && room.sizex>room.sizey) {
			Clutter vats;
			vats.height = 0;
			vats.rot = 90;
			vats.size = 0;
			for (int j = 0; j < room.sizey; j++) {
				vats.posy = (room.posy + 0.5f + j) * tileSize + tileOffset - tileSize / 2.f;
				for (float k = 1.f; k < room.sizex-1; k+=2) {
					vats.posx = (room.posx + 1 + k) * tileSize + tileOffset - tileSize / 2.f;
					specialClutter.push(vats);
				}
			}
			vats.size = 1;
			vats.rot = 270;
			vats.posx = (room.posx+0.5f )  * tileSize + tileOffset - tileSize / 2.f;
			vats.posy = (room.posy + (room.sizey)/2.0f ) * tileSize + tileOffset - tileSize / 2.f;
			extraSpawnPoints.push_back(glm::vec3(vats.posx, 0.5f, vats.posy));
			specialClutter.push(vats);
			matched.at(i).isCloning = true;
		}
		else if (room.sizey > 2 && makeRoomCloningChance > 60) {
			Clutter vats;
			vats.height = 0;
			vats.rot = 0;
			vats.size = 0;
			for (int j = 0; j < room.sizex; j++) {
				vats.posx = (room.posx + 0.5f + j) * tileSize + tileOffset - tileSize / 2.f;
				for (float k = 1.f; k < room.sizey - 1; k += 2) {
					vats.posy = (room.posy + 1 + k) * tileSize + tileOffset - tileSize / 2.f;
					specialClutter.push(vats);
				}
			}
			vats.size = 1;
			vats.rot = 180;
			vats.posy = (room.posy + 0.5f) * tileSize + tileOffset - tileSize / 2.f;
			vats.posx = (room.posx + (room.sizex) / 2.0f) * tileSize + tileOffset - tileSize / 2.f;
			extraSpawnPoints.push_back(glm::vec3(vats.posx, 0.5f, vats.posy));
			specialClutter.push(vats);
			matched.at(i).isCloning = true;
		}
#endif
		else {
			for (int x = 0; x < room.sizex; x++) {
				for (int y = 0; y < room.sizey; y++) {
					if ((x+0.5f)!=room.sizex/2.f || (y + 0.5f) != room.sizey / 2.f) {
						extraSpawnPoints.push_back(glm::vec3((room.posx + x) * tileSize, 0.5f, (room.posy + y) * tileSize));
					}
					if (tileArr[x + room.posx][y + room.posy][2] < 17) {
						if (rand() % 100 < clutterModifier) {
							float xmax = tileSize * 0.95f;
							float ymax = tileSize * 0.95f;
							float xmin = tileSize * 0.05f;
							float ymin = tileSize * 0.05f;

							//move spawnpoints further from walls
							int tile = tileArr[x + room.posx][y + room.posy][0];
							if (tile % 2 == 1) {
								ymax -= 1.5f * tileSize / 10.f;
							}
							if ((tile % 4) / 2 == 1) {
								xmax -= 1.5f * tileSize / 10.f;
							}
							if ((tile % 8) / 4 == 1) {
								ymin += 1.5f * tileSize / 10.f;
							}
							if (tile / 8 == 1) {
								xmin += 1.5f * tileSize / 10.f;
							}

							//move spawnpoints further from doors
							tile = tileArr[x + room.posx][y + room.posy][2];
							if (tile % 2 == 1) {
								ymax -= 2.f * tileSize / 10.f;
							}
							if ((tile % 4) / 2 == 1) {
								xmax -= 2.f * tileSize / 10.f;
							}
							if ((tile % 8) / 4 == 1) {
								ymin += 2.f * tileSize / 10.f;
							}
							if (tile / 8 == 1) {
								xmin += 2.f * tileSize / 10.f;
							}

							float clutterPosX = ((rand() % 100) / 100.f) * (xmax - xmin) + xmin + (x + room.posx) * tileSize + tileOffset - tileSize / 2.f;
							float clutterPosY = ((rand() % 100) / 100.f) * (ymax - ymin) + ymin + (y + room.posy) * tileSize + tileOffset - tileSize / 2.f;
							int rot = (rand() % 4) * 90;
							Clutter clutterLarge;
							clutterLarge.posx = clutterPosX;
							clutterLarge.posy = clutterPosY;
							clutterLarge.height = 0;
							clutterLarge.rot = rot / 1.f;
							clutterLarge.size = 0;
							largeClutter.push(clutterLarge);
						}
					}
				}
			}
		}
	}
	//adds clutter on top of large objects
	size_t amount = largeClutter.size();
	for (size_t i = 0; i < amount; i++) {
		Clutter clutterLarge = largeClutter.front();
		largeClutter.pop();
		if (rand() % 100 < clutterModifier) {

			//adds either one or two objects that are either medium or small
			if(rand()%2==0){
				//add on first half
				{
					Clutter clutterToAdd;
					clutterToAdd.size = 1 + rand() % 2;
					clutterToAdd.height = 1 + clutterLarge.height;
					clutterToAdd.rot = (rand() % 360) / 1.f;
					float angleToRotate = glm::radians(clutterLarge.rot);
					float clutterPosX = (rand() % 40) / 100.f - 0.7f;
					float clutterPosY = (rand() % 40) / 100.f - 0.2f;
					clutterToAdd.posx = clutterPosX * cos(angleToRotate) + clutterPosY * sin(angleToRotate) + clutterLarge.posx;
					clutterToAdd.posy = -clutterPosX * sin(angleToRotate) + clutterPosY * cos(angleToRotate) + clutterLarge.posy;
					if (clutterToAdd.size == 1) {
						mediumClutter.push(clutterToAdd);
					}
					else {
						smallClutter.push(clutterToAdd);
					}
				}
				//add on second half
				{
					Clutter clutterToAdd;
					clutterToAdd.size = 1 + rand() % 2;
					clutterToAdd.height = 1 + clutterLarge.height;
					clutterToAdd.rot = (rand() % 360) / 1.f;
					float angleToRotate = glm::radians(clutterLarge.rot);
					float clutterPosX = (rand() % 40) / 100.f + 0.3f;
					float clutterPosY = (rand() % 40) / 100.f - 0.2f;
					clutterToAdd.posx = clutterPosX * cos(angleToRotate) + clutterPosY * sin(angleToRotate) + clutterLarge.posx;
					clutterToAdd.posy = -clutterPosX * sin(angleToRotate) + clutterPosY * cos(angleToRotate) + clutterLarge.posy;
					if (clutterToAdd.size == 1) {
						mediumClutter.push(clutterToAdd);
					}
					else {
						smallClutter.push(clutterToAdd);
					}
				}

			}
			else {
				float clutterPosX = (rand() % 140) / 100.f - 0.7f;
				float clutterPosY = (rand() % 40) / 100.f - 0.2f;
				Clutter clutterToAdd;
				float angleToRotate = glm::radians(clutterLarge.rot);
				clutterToAdd.posx = clutterPosX * cos(angleToRotate) + clutterPosY * sin(angleToRotate) + clutterLarge.posx;
				clutterToAdd.posy = -clutterPosX * sin(angleToRotate) + clutterPosY * cos(angleToRotate) + clutterLarge.posy;
				clutterToAdd.size = 1 + rand() % 2;
				clutterToAdd.height = 1 + clutterLarge.height;
				clutterToAdd.rot = (rand() % 360)/1.f;
				if (clutterToAdd.size == 1) {
					mediumClutter.push(clutterToAdd);
				}
				else {
					smallClutter.push(clutterToAdd);
				}
			}
		}
		largeClutter.push(clutterLarge);
	}

	//stacks up to two medium objects on each other
	std::queue<Clutter> doubleStackedMediumClutter;
	amount = mediumClutter.size();
	for (size_t i = 0; i < amount; i++) {
		Clutter clutterMedium = mediumClutter.front();
		mediumClutter.pop();
		if (rand() % 100 < clutterModifier) {
			float clutterPosX = (rand() % 20) / 100.f - 0.1f;
			float clutterPosY = (rand() % 20) / 100.f - 0.1f;
			float angleToRotate = glm::radians(clutterMedium.rot);
			Clutter mediumStacked;
			mediumStacked.posx = clutterPosX * cos(angleToRotate) + clutterPosY * sin(angleToRotate) + clutterMedium.posx;
			mediumStacked.posy = -clutterPosX * sin(angleToRotate) + clutterPosY * cos(angleToRotate) + clutterMedium.posy;
			mediumStacked.size = 2;
			mediumStacked.height = 0.25f + clutterMedium.height;
			mediumStacked.rot = (rand() % 360) / 1.f;
			mediumClutter.push(mediumStacked);
			doubleStackedMediumClutter.push(clutterMedium);
		}
		else {
			mediumClutter.push(clutterMedium);	
		}
	}


	//adds small clutter on top of medium clutter
	amount = mediumClutter.size();
	for (size_t i = 0; i < amount; i++) {
		Clutter clutterMedium = mediumClutter.front();
		mediumClutter.pop();
		if (rand() % 100 < clutterModifier) {
			float clutterPosX = (rand() % 40) / 100.f - 0.2f;
			float clutterPosY = (rand() % 40) / 100.f - 0.2f;
			float angleToRotate = glm::radians(clutterMedium.rot);
			Clutter clutterSmall;
			clutterSmall.posx = clutterPosX * cos(angleToRotate) + clutterPosY * sin(angleToRotate) + clutterMedium.posx;
			clutterSmall.posy = -clutterPosX * sin(angleToRotate) + clutterPosY * cos(angleToRotate) + clutterMedium.posy;
			clutterSmall.size = 2;
			clutterSmall.height = 0.25f + clutterMedium.height;
			clutterSmall.rot = (rand() % 360)/1.f;
			smallClutter.push(clutterSmall);
		}
		mediumClutter.push(clutterMedium);
	}
	while (!doubleStackedMediumClutter.empty()) {
		mediumClutter.push(doubleStackedMediumClutter.front());
		doubleStackedMediumClutter.pop();
	}
}

void LevelSystem::addClutterModel(const std::vector<Model*>& clutterModels, Model* bb) {
	while (largeClutter.size() > 0) {
		Clutter clut = largeClutter.front();
		largeClutter.pop();
		if (rand() % 2 == 0) {
			EntityFactory::CreateStaticMapObject("ClutterLarge", clutterModels[ClutterModel::TABLE], bb, glm::vec3(clut.posx, 0.f, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
		}
		else {
			EntityFactory::CreateStaticMapObject("ClutterLarge", clutterModels[ClutterModel::BOXES], bb, glm::vec3(clut.posx, 0.f, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
		}
	}
	while (mediumClutter.size() > 0) {
		Clutter clut = mediumClutter.front();
		mediumClutter.pop();
		switch (rand() % 4) {
		case 0:	EntityFactory::CreateStaticMapObject("ClutterMedium", clutterModels[ClutterModel::MEDIUMBOX], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
			break;
		case 1:	EntityFactory::CreateStaticMapObject("ClutterMedium", clutterModels[ClutterModel::BOOKS1], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
			break;
		case 2:	EntityFactory::CreateStaticMapObject("ClutterMedium", clutterModels[ClutterModel::BOOKS2], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
			break;
		case 3:	EntityFactory::CreateStaticMapObject("ClutterMedium", clutterModels[ClutterModel::SQUAREBOX], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
			break;
		}
	}
	while (smallClutter.size() > 0) {
		Clutter clut = smallClutter.front();
		smallClutter.pop();
		switch (rand() % 3) {
		case 0: EntityFactory::CreateStaticMapObject("ClutterSmall", clutterModels[ClutterModel::NOTEPAD], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1.f, 1, 1.f));
			break;
		case 1: EntityFactory::CreateStaticMapObject("ClutterSmall", clutterModels[ClutterModel::SCREEN], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1.f, 1, 1.f));
			break;
		case 2: EntityFactory::CreateStaticMapObject("ClutterSmall", clutterModels[ClutterModel::MICROSCOPE], bb, glm::vec3(clut.posx, clut.height, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1.f, 1, 1.f));
			break;
		}
	}
	while (specialClutter.size() > 0) {
		Clutter clut = specialClutter.front();
		specialClutter.pop();
		if (clut.size == 0) {
			EntityFactory::CreateStaticMapObject("ClutterSpecial", clutterModels[ClutterModel::CLONINGVATS], bb, glm::vec3(clut.posx, 0.f, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
		}
		else {
			EntityFactory::CreateStaticMapObject("ClutterSpecial", clutterModels[ClutterModel::CONTROLSTATION], bb, glm::vec3(clut.posx, 0.f, clut.posy), glm::vec3(0.f, glm::radians(clut.rot), 0.f), glm::vec3(1, 1, 1));
		}
	}

#ifdef _DEBUG
	for (int i = 0; i < spawnPoints.size(); i++) {
		EntityFactory::CreateStaticMapObject("Spawnpoint", clutterModels[ClutterModel::CONTROLSTATION], bb, spawnPoints[i], glm::vec3(0.f,0, 0.f), glm::vec3(0.1f, 0.1f, 0.1f));
	}
	for (int i = 0; i < extraSpawnPoints.size(); i++) {
		EntityFactory::CreateStaticMapObject("Spawnpoint", clutterModels[ClutterModel::NOTEPAD], bb, extraSpawnPoints[i], glm::vec3(0.f, 0, 0.f), glm::vec3(1,1, 1));
	}
	for (int i = 0; i < powerUpSpawnPoints.size(); i++) {
		EntityFactory::CreateStaticMapObject("Spawnpoint", clutterModels[ClutterModel::CLONINGVATS], bb, getPowerUpPosition(i), glm::vec3(0.f, 0, 0.f), glm::vec3(0.1f, 0.1f, 0.1f));
	}
#endif

	for (int i = 0; i < numberOfRooms; i++) {
		Rect room = matched.at(i);
		auto e2 = EntityFactory::CreateStaticMapObject("Saftblandare", clutterModels[ClutterModel::SAFTBLANDARE], bb, glm::vec3((room.posx + (room.sizex / 2.f)-0.5f)*tileSize, 0, (room.posy + (room.sizey / 2.f)-0.5f)*tileSize),glm::vec3(0.f),glm::vec3(1.f,tileHeight,1.f));

		MovementComponent* mc = e2->addComponent<MovementComponent>();
		AudioComponent* ac = e2->addComponent<AudioComponent>();
		SpotlightComponent* sc = e2->addComponent<SpotlightComponent>();
		sc->light.setColor(glm::vec3(1.0f, 0.2f, 0.0f));
		sc->light.setPosition(glm::vec3(0, tileHeight * 5 - 0.05, 0));
		sc->light.setRadius(30.f);
		sc->light.setDirection(glm::vec3(1, 0, 0));
		sc->light.setAngle(0.5);
		sc->roomID = getRoomID(room.posx, room.posy);
		sc->isOn = false;
#ifdef _PERFORMANCE_TEST
		sc->isOn = true;
#endif

#ifdef DEVELOPMENT
		auto* particleEmitterComp = e2->addComponent<ParticleEmitterComponent>();

		float sprinklerXspread = room.sizex * tileSize * 1.3f;
		float sprinklerZspread = room.sizey * tileSize * 1.3f;

		particleEmitterComp->size = 0.1f;
		particleEmitterComp->offset = { 0, tileHeight * 6, 0 };
		particleEmitterComp->constantVelocity = { 0.0f, -0.5f, 0.0f };
		particleEmitterComp->acceleration = { 0.0f, -30.0f, 0.0f };
		particleEmitterComp->spread = { sprinklerXspread, 0.5f, sprinklerZspread };
		particleEmitterComp->spawnRate = 1.f / (100.f * room.sizex * room.sizey);
		particleEmitterComp->lifeTime = 0.5f;
		particleEmitterComp->atlasSize = glm::uvec2(8U, 3U);
		particleEmitterComp->drag = 30.0f;
		particleEmitterComp->maxNumberOfParticles = (int)glm::ceil((1.0f / particleEmitterComp->spawnRate) * particleEmitterComp->lifeTime);
		std::string particleTextureName = "particles/animSmoke.dds";
		if (!Application::getInstance()->getResourceManager().hasTexture(particleTextureName)) {
			Application::getInstance()->getResourceManager().loadTexture(particleTextureName);
		}
		particleEmitterComp->setTexture(particleTextureName);
#endif
	}
}