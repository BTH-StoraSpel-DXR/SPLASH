#pragma once
#include "Sail/utils/Utils.h"
#include <map>
#define SAIL_BONES_PER_VERTEX 5

class Animation {
public:
	class Frame {
	public:
		Frame();
		Frame(const unsigned int size);
		~Frame();
		void setTransform(const unsigned int index, const glm::mat4& transform);
		const unsigned int getTransformListSize();
		const glm::mat4* getTransformList();
	private:
		unsigned int m_transformSize;
		glm::mat4* m_limbTransform;
	};


	

	// TODO: rename behind variable to better describe it. (if frame behind or infront of time should be returned.)
	enum FindType {
		BEHIND,
		CLOSEST,
		INFRONT
	};

	Animation();
	Animation(const std::string& name);
	~Animation();
	const float getMaxAnimationTime();
	const unsigned int getMaxAnimationFrame();
	const glm::mat4* getAnimationTransform(const float time, const FindType type = CLOSEST);
	const glm::mat4* getAnimationTransform(const unsigned int frame);
	const unsigned int getAnimationTransformSize(const float time);
	const unsigned int getAnimationTransformSize(const unsigned int frame);
	const float getTimeAtFrame(const unsigned int frame);
	const unsigned int getFrameAtTime(float time, const FindType type = CLOSEST);
	void addFrame(const unsigned int frame, const float time, Animation::Frame* data);

	void setName(const std::string& name);
	const std::string getName();
	
private:
	
	std::string m_name;
	float m_maxFrameTime;
	unsigned int m_maxFrame;
	unsigned int m_numFrames;


	inline const bool exists(const unsigned int frame);

	std::map<unsigned int, float> m_frameTimes;
	std::map<unsigned int, Animation::Frame*> m_frames;
};



class AnimationStack {
public:
	class VertConnection {
	public:
		VertConnection();
		void addConnection(const unsigned int _transform, const float _weight);
		const float checkWeights();
		void normalizeWeights();

		unsigned int count;
		unsigned int transform[SAIL_BONES_PER_VERTEX];
		float weight[SAIL_BONES_PER_VERTEX];
	};
	class Bone {
	public:
		enum BodyPart {
			LOWER,
			UPPER
		};

		Bone() { name = ""; parentIndex = -1; uniqueID = 0; globalBindposeInverse = glm::identity<glm::mat4>(); part = BodyPart::LOWER; };
		~Bone() {};
		unsigned int uniqueID;
		std::string name;
		int parentIndex;
		BodyPart part;
		std::vector<unsigned int> childIndexes;
		glm::mat4 globalBindposeInverse;
	};
	AnimationStack();
	AnimationStack(const unsigned int vertCount);
	~AnimationStack();

	void reSizeConnections(const unsigned int vertCount);

	void addAnimation(const std::string& animationName, Animation* animation);
	void setConnectionData(const unsigned int vertexIndex, const unsigned int boneIndex, float weight);

	void addBone(AnimationStack::Bone& bone);
	const unsigned int boneCount();
	Bone& getBone(const unsigned int index);

	unsigned int getByteSize();

	Animation* getAnimation(const std::string& name);
	Animation* getAnimation(const unsigned int index);
	const unsigned int getAnimationIndex(const std::string& name);
	const std::string getAnimationName(const unsigned int);
	const unsigned int getAnimationCount();

	const glm::mat4* getTransform(const std::string& name, const float time);
	const glm::mat4* getTransform(const std::string& name, const unsigned int frame);
	const glm::mat4* getTransform(const unsigned int index, const float time);
	const glm::mat4* getTransform(const unsigned int index, const unsigned int frame);

	VertConnection* getConnections();
	const unsigned int getConnectionSize();

	void checkWeights();
	void normalizeWeights();
private:

	unsigned int m_connectionSize;
	VertConnection* m_connections;
	
	std::map<unsigned int, std::string> m_names;
	std::map<std::string, unsigned int> m_indexes;
	std::map<std::string, Animation*> m_stack;
	std::vector<AnimationStack::Bone> m_bones;

};

