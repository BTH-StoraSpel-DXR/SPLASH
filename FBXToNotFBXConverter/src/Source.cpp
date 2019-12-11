#include <filesystem>
#include <iostream>
#include <vector>
#include "FBXLoader.h"
#include "NotFBXLoader.h"
#include "pch.h"

//Leave my "\\" be!
#define FBX_PATH "res\\models"
//#define NOT_FBX_PATH "..\\SPLASH\\res\\models"
#define NOT_FBX_PATH "res\\NOTFBX\\models"

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
	std::string root(FBX_PATH);

	int i = 0;
	int size = filesToConvert.size();
	for (auto f : filesToConvert) {
		i++;
		std::cout << "[ " << i << " / " << size << " ] " << f.parent_path().string() << std::endl;

		Mesh::Data data;
		AnimationStack* animationStack = nullptr;
		std::string sourcePath = f.relative_path().string();
		std::string destPath   = NOT_FBX_PATH + std::string("\\") + sourcePath.substr(root.length() + 1);
		destPath = destPath.substr(0, destPath.find_last_of('.')) + ".notfbx";
		loader.loadFBXFile(sourcePath, data, animationStack);
		
		std::filesystem::create_directories(std::filesystem::path(destPath).parent_path());
		NotFBXLoader::Save(destPath, &data, animationStack);

		if (animationStack) {
			delete animationStack;
		}

		//f.parent_path();
	}

	while (true) {

	}

	return 0;
}