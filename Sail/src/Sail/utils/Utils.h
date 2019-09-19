#pragma once

#include <exception>
#include <string>
#include <random>
//#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#define BIT(x) 1 << x

// Break on failed HRESULT return
#define ThrowIfFailed(result)	\
	if (FAILED(result))			\
		throw std::exception(); \

// Macro to easier track down memory leaks
#ifdef _DEBUG
#define SAIL_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define SAIL_NEW new
#endif

// Inherit this to make a class non-inheritable
//template<typename T>		(this was not used anywhere, and you can just write "final" after the class name instead)
//class MakeFinal {
//private:
//	~MakeFinal() { };
//	friend T;
//};

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
	/*inline static void Log(const std::string& msg);
	inline static void Warning(const std::string& msg);
	inline static void Error(const std::string& msg);*/
	static void Log(const std::string& msg);
	static void Warning(const std::string& msg);
	static void Error(const std::string& msg);
};

namespace Utils {
	std::string readFile(const std::string& filepath);
	std::wstring toWStr(const glm::vec3& vec);
	std::string toStr(const glm::vec3& vec);
	std::string toStr(const glm::vec2& vec);
	float rnd();
	glm::vec4 getRandomColor();
	float clamp(float val, float min, float max);
	float smootherstep(float edge0, float edge1, float x);

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