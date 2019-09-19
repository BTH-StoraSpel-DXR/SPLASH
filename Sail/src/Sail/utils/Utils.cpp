#include "pch.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::string Utils::readFile(const std::string& filepath) {
	std::ifstream t(filepath);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

std::wstring Utils::toWStr(const glm::vec3& vec) {
	std::wstringstream ss;
	ss << "[X: " << vec.x << ", Y: " << vec.y << ", Z: " << vec.z << "]";
	return ss.str();
}

std::string Utils::toStr(const glm::vec3& vec) {
	std::stringstream ss;
	ss << "[X: " << vec.x << ", Y: " << vec.y << ", Z: " << vec.z << "]";
	return ss.str();
}

std::string Utils::toStr(const glm::vec2& vec) {
	std::stringstream ss;
	ss << "[X: " << vec.x << ", Y: " << vec.y << "]";
	return ss.str();
}

float Utils::rnd() {
	return dis(gen);
}

float Utils::clamp(float val, float min, float max) {

	if (val > max) return max;
	if (val < min) return min;
	return val;

}

float Utils::smootherstep(float edge0, float edge1, float x) {
	// Scale, and clamp x to 0..1 range
	x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	// Evaluate polynomial
	return x * x * x * (x * (x * 6 - 15) + 10);
}

glm::vec4 Utils::getRandomColor() {
	return glm::vec4(Utils::rnd(), Utils::rnd(), Utils::rnd(), 1);
}

std::string Utils::String::getBlockStartingFrom(const char* source) {
	const char* end = strstr(source, "}");
	if (!end)
		return std::string(source);
	std::string str(source, strnlen(source, end - source));
	return str;
}

const char* Utils::String::findToken(const std::string& token, const char* source) {
	const char* match;
	if (match = strstr(source, token.c_str())) {
		bool left = match == source || isspace((match - 1)[0]);
		match += token.size();
		bool right = match != '\0' || isspace(match[0]); // might need to be match + 1
		return match;
	}
	return nullptr;
}

int Utils::String::findNextIntOnLine(const char* source) {
	const char* p = source;
	while (*p) {
		if (*p == '\n') {
			break;
		}
		if (isdigit(*p)) {
			char* end;
			long val = strtol(p, &end, 10);
			return val;
		}
		else {
			p++;
		}
	}
	return -1;
}

const char* Utils::String::nextLine(const char* source) {
	const char* p = source;
	while (*p != '\n' && *p != '\0') p++;
	p++;
	p = removeBeginningWhitespaces(p);
	return p;
}

std::string Utils::String::nextToken(const char* source) {
	const char* start = source;
	while (isspace(*start)) start++;
	int size = 0;
	while (!isspace(start[size])) size++;
	return std::string(start, strnlen(start, size));
}

const char* Utils::String::removeBeginningWhitespaces(const char* source) {
	const char* p = source;
	while (isspace(*p)) p++;
	return p;
}

std::string Utils::String::removeComments(const std::string& source) {
	std::string result = source;
	std::string::size_type start;
	while ((start = result.find("//")) != std::string::npos) {
		std::string::size_type size = result.substr(start).find('\n');
		if (size == std::string::npos) size = result.size();
		result.erase(start, size);
	}
	while ((start = result.find("/*")) != std::string::npos) {
		size_t size = result.substr(start).find("*/");
		if (size == std::string::npos) size = result.size();
		else size += 2;
		result.erase(start, size);
	}
	return result;
}

bool Utils::String::startsWith(const char* source, const std::string& prefix) {
	return strncmp(source, prefix.c_str(), prefix.size()) == 0;
}

void Logger::Log(const std::string& msg) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0x0F);
	std::cout << "LOG: " << msg << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);
}

void Logger::Warning(const std::string& msg) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0xE0);
	std::cout << "WARNING: " << msg << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);

#ifdef _SAIL_BREAK_ON_WARNING
	__debugbreak();
#endif
}

void Logger::Error(const std::string& msg) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0xC0);
	std::cout << "ERROR: " << msg << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);

#ifdef _SAIL_BREAK_ON_ERROR
	__debugbreak();
#endif
}