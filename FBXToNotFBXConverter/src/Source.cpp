#include <filesystem>
#include <iostream>
#include <vector>
#include "FBXLoader.h"
#include "NotFBXLoader.h"

//Leave my "\\" be!
#define FBX_PATH "res\\models"
#define NOT_FBX_PATH "..\\SPLASH\\res\\models"

FBXLoader2 loader;
std::vector<std::filesystem::path> filesToConvert;

void ListDir(std::filesystem::path path) {
	for (auto file : std::filesystem::directory_iterator(path)) {
		if (file.is_directory()) {
			ListDir(file);
			continue;
		}

		if (file.path().extension() != ".fbx") {
			continue;
		}

		filesToConvert.push_back(file.path());
	}
}

int main() {

	std::filesystem::create_directories(FBX_PATH);
	std::filesystem::create_directories(NOT_FBX_PATH);
	
	ListDir(FBX_PATH);

	int i = 0;
	int size = filesToConvert.size();
	for (auto f : filesToConvert) {
		i++;
		std::cout << "[ " << i << " / " << size << " ] " << f.string() << std::endl;

		Mesh::Data data;
		AnimationStack* animationStack = nullptr;
		loader.loadFBXFile(f.filename().string(), data, animationStack);
	}

	while (true) {

	}

	return 0;
}