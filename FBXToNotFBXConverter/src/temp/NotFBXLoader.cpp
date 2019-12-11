#include "pch.h"
#include "NotFBXLoader.h"
#include <fstream>

#ifdef FBX_CONVERTER
#include "Animation.h"
#else
#include "Sail/graphics/geometry/Model.h"
#include "Sail/graphics/geometry/Animation.h"
#endif // !FBX_CONVERTER


void NotFBXLoader::Load(const std::string& filename, Mesh::Data* mdata, Shader* shader,  AnimationStack*& animationStack) {
	std::ifstream out(filename, std::fstream::binary);
	char m = 'm';
	char a = 'a';

	char type;



	while (!out.eof()) {
		out.read((char*)&type, sizeof(type));
		if (type == m && !mdata) {
			Mesh::Data& data = *mdata;

			//Mesh / Model
			out.read((char*)(data.numIndices), sizeof(data.numIndices));
			data.indices = SAIL_NEW unsigned long[data.numIndices];
			out.read((char*)data.indices, sizeof(*data.indices) * data.numIndices);
			out.read((char*)& data.numVertices, sizeof(data.numVertices));
			out.read((char*)& data.numInstances, sizeof(data.numInstances));

			data.positions		= SAIL_NEW Mesh::vec3[data.numVertices];
			data.normals		= SAIL_NEW Mesh::vec3[data.numVertices];
			data.texCoords		= SAIL_NEW Mesh::vec2[data.numVertices];
			data.tangents		= SAIL_NEW Mesh::vec3[data.numVertices];
			data.bitangents	= SAIL_NEW Mesh::vec3[data.numVertices];

			out.read((char*)data.positions,	sizeof(*data.positions) *	data.numVertices);
			out.read((char*)data.normals,		sizeof(*data.normals) *	data.numVertices);
			out.read((char*)data.texCoords,	sizeof(*data.texCoords) *	data.numVertices);
			out.read((char*)data.tangents,		sizeof(*data.tangents) *	data.numVertices);
			out.read((char*)data.bitangents,	sizeof(*data.bitangents) * data.numVertices);
						
		} else if (type == a && !animationStack) {
			animationStack = SAIL_NEW AnimationStack();
			

			//Animation stack
			unsigned int connS;
			AnimationStack::VertConnection* connections;
			out.read((char*)& connS, sizeof(connS));
			connections = SAIL_NEW AnimationStack::VertConnection[connS];
			out.read((char*)connections, sizeof(*connections) * connS);
			animationStack->setConnections(connections, connS);

			size_t nBones;

			//Bones
			out.read((char*)& nBones, sizeof(nBones));
			for (size_t i = 0; i < nBones; i++) {
				AnimationStack::Bone b;
				size_t size;
				out.read((char*)& b.uniqueID, sizeof(b.uniqueID));
				out.read((char*)& size, sizeof(size));
				b.name.resize(size + 1);
				out.read((char*)b.name.data(), size);
				out.read((char*)& b.parentIndex, sizeof(b.parentIndex));
				out.read((char*)& b.part, sizeof(b.part));
				out.read((char*)& size, sizeof(size));
				b.childIndexes.resize(size);
				out.read((char*)b.childIndexes.data(), sizeof(decltype(b.childIndexes)::value_type) * size);
				out.read((char*)& b.globalBindposeInverse, sizeof(b.globalBindposeInverse));

				animationStack->addBone(b);
			}

			//Animation Stack
			size_t nAnimations;
			Animation* animation;
			out.read((char*)& nAnimations, sizeof(nAnimations));
			for (size_t i = 0; i < nAnimations; i++) {
				std::string name;
				size_t stringLen;
				unsigned int maxFrame;

				out.read((char*)& stringLen, sizeof(stringLen));
				name.resize(stringLen + 1);
				out.read((char*)name.data(), stringLen);

				animation = SAIL_NEW Animation(name);
				
				out.read((char*)& maxFrame, sizeof(maxFrame));

				float timeAtFrame;
				glm::mat4* limbTransform;

				for (unsigned int fr = 0; fr < maxFrame; fr++) {
					limbTransform = SAIL_NEW glm::mat4[nBones];

					out.read((char*)& timeAtFrame, sizeof(timeAtFrame));
					out.read((char*)limbTransform, sizeof(glm::mat4) * nBones);

					animation->addFrame(fr, timeAtFrame, SAIL_NEW Animation::Frame(limbTransform, nBones));
				}

				animationStack->addAnimation(name, animation);
			}
		} else {
#ifndef FBX_CONVERTER
			SAIL_LOG_WARNING("Not FBX Loader found something strange!");
#endif // !FBX_CONVERTER
			break;
		}
	}

	out.close();
}

void NotFBXLoader::Save(const std::string& filename, Mesh::Data* mdata, AnimationStack* animationStack) {
	std::ofstream out(filename, std::fstream::binary);

	char m = 'm';
	char a = 'a';

	if (mdata) {
		Mesh::Data& data = *mdata;

		out.write((char*)&m, sizeof(m));
		//Mesh / Model
		out.write((char*)&data.numIndices, sizeof(data.numIndices));
		out.write((char*)data.indices, sizeof(*data.indices) * data.numIndices);
		out.write((char*)&data.numVertices, sizeof(data.numVertices));
		out.write((char*)&data.numInstances, sizeof(data.numInstances));
		out.write((char*)data.positions, sizeof(*data.positions) * data.numVertices);
		out.write((char*)data.normals, sizeof(*data.normals) * data.numVertices);
		out.write((char*)data.texCoords, sizeof(*data.texCoords) * data.numVertices);
		out.write((char*)data.tangents, sizeof(*data.tangents) * data.numVertices);
		out.write((char*)data.bitangents, sizeof(*data.bitangents) * data.numVertices);
	}

	if (animationStack) {
		out.write((char*)&a, sizeof(a));

		//Animation stack
		const unsigned int connS = animationStack->getConnectionSize();
		AnimationStack::VertConnection* con = animationStack->getConnections();
		size_t nBones = animationStack->boneCount();

		out.write((char*)&connS, sizeof(connS));
		out.write((char*)con, sizeof(*con) * connS);
		
		//Bones

		size_t nameLen;
		out.write((char*)&nBones, sizeof(nBones));
		for (size_t i = 0; i < nBones; i++) {
			AnimationStack::Bone& b = animationStack->getBone(i);
			size_t nChildren = b.childIndexes.size();
			nameLen = b.name.length();

			out.write((char*)&b.uniqueID, sizeof(b.uniqueID));
			out.write((char*)&nameLen, sizeof(nameLen));
			out.write((char*)b.name.data(), b.name.length());
			out.write((char*)&b.parentIndex, sizeof(b.parentIndex));
			out.write((char*)&b.part, sizeof(b.part));
			out.write((char*)&nChildren, sizeof(nChildren));
			out.write((char*)b.childIndexes.data(), sizeof(decltype(b.childIndexes)::value_type) * nChildren);
			out.write((char*)&b.globalBindposeInverse, sizeof(b.globalBindposeInverse));
		}

		//Animation Stack
		size_t nAnimations = animationStack->getAnimationCount();
		Animation* animation;
		out.write((char*)& nAnimations, sizeof(nAnimations));
		for (size_t i = 0; i < nAnimations; i++) {
			animation = animationStack->getAnimation(i);
			std::string name = animation->getName();
			size_t stringLen = name.length();
			unsigned int maxFrame = animation->getMaxAnimationFrame();

			out.write((char*)& stringLen, sizeof(stringLen));
			out.write((char*)name.data(), name.length());		

			float timeAtFrame;
			out.write((char*)&maxFrame, sizeof(maxFrame));
			for (unsigned int fr = 0; fr < maxFrame; fr++) {
				timeAtFrame = animation->getTimeAtFrame(fr);
				out.write((char*)&timeAtFrame, sizeof(timeAtFrame));
				out.write((char*)animation->getAnimationTransform(fr), sizeof(glm::mat4) * nBones);
			}
		}
	}

	out.close();
}
