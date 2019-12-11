#pragma once
#include <glm/glm.hpp>
#include <string>
#include <random>

#define BIT(x) 1 << x

// Break on failed HRESULT return
#define ThrowIfFailed(result)	\
	if (FAILED(result))			\
		throw std::exception();
// Show message box and break on failed blob creation
#define ThrowIfBlobError(hr, blob) { \
	if (FAILED(hr)) { \
		MessageBoxA(0, (char*)blob->GetBufferPointer(), "", 0); \
		OutputDebugStringA((char*)blob->GetBufferPointer()); \
		throw std::exception(); \
	} \
}

// Macro to easier track down memory leaks
#ifdef _DEBUG
#define SAIL_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define SAIL_NEW new
#endif

// Inherit this to make a class non-inheritable
template<typename T>
class MakeFinal {
private:
	~MakeFinal() { };
	friend T;
};

class Memory {

public:
	template <typename T>
	static void SafeDelete(T& t) {
		if (t) {
			delete t;
			t = nullptr;
		}
	}

	template <typename T>
	static void SafeDeleteArr(T& t) {
		if (t) {
			delete[] t;
			t = nullptr;
		}
	}

	template<typename T>
	static void SafeRelease(T& t) {
		if (t) {
			t->Release();
			t = nullptr;
		}
	}
	template<typename T>
	static void SafeRelease(T*& t) {
		if (t) {
			t->Release();
			t = nullptr;
		}
	}
	template<typename T>
	static void SafeRelease(T** t) {
		if (*t) {
			(*t)->Release();
			(*t) = nullptr;
		}
	}

};

class Logger {
public:
	static void Log(const std::string& msg, const std::string& file = "", const std::string& line = "");
	static void Warning(const std::string& msg, const std::string& file = "", const std::string& line = "");
	static void Error(const std::string& msg, const std::string& file = "", const std::string& line = "");
};

#define SAIL_LOG( message ) Logger::Log(message, __FILE__, std::to_string(__LINE__))
#define SAIL_LOG_WARNING( message ) Logger::Warning(message, __FILE__, std::to_string(__LINE__))
#define SAIL_LOG_ERROR( message ) Logger::Error(message, __FILE__, std::to_string(__LINE__))

namespace Utils {
	std::string readFile(const std::string& filepath);
	bool writeFileTrunc(const std::string& filepath, const std::string& content);
	bool writeFileAppend(const std::string& filepath, const std::string& content);
	std::wstring toWStr(const glm::vec3& vec);
	std::string toStr(const glm::vec4& vec);
	std::string toStr(const glm::vec3& vec);
	std::string toStr(const glm::vec2& vec);
	float rnd();
	int fastrand();
	void setfastrandSeed(int seed);
	glm::vec4 getRandomColor();
	float clamp(float val, float min, float max);
	float smootherstep(float edge0, float edge1, float x);
	float wrapValue(float value, float lowerBound, float upperBound);
	inline int to1D(const glm::i32vec3& ind, int xMax, int yMax) {
		return ind.x + xMax * (ind.y + yMax * ind.z);
	}
	inline glm::i32vec3 to3D(int ind, int xMax, int yMax) {
		glm::i32vec3 ind3d;
		ind3d.z = ind / (xMax * yMax);
		ind -= (ind3d.z * xMax * yMax);
		ind3d.y = ind / xMax;
		ind3d.x = ind % xMax;
		return ind3d;
	}
	inline int testSign(float x) {
		if (x > 0.f) {
			return 1;
		} else if (x < 0.f) {
			return -1;
		} else {
			return 0;
		}
	}

	uint32_t packQuarterFloat(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
	uint32_t unpackQuarterFloat(uint32_t in, unsigned int index);
	glm::vec2 getRotations(const glm::vec3& dir);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);

	namespace String {
		std::string getBlockStartingFrom(const char* source);
		const char* findToken(const std::string& token, const char* source);
		int findNextIntOnLine(const char* source);
		const char* nextLine(const char* source);
		std::string nextToken(const char* source);
		const char* removeBeginningWhitespaces(const char* source);
		std::string removeComments(const std::string& source);
		bool startsWith(const char* source, const std::string& prefix);
	};

};