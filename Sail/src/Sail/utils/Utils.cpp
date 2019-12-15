#include "pch.h"
#include "Utils.h"
#include "Sail/Application.h"

#include <filesystem>

std::string Utils::readFile(const std::string& filepath) {
	std::ifstream t(filepath);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

bool Utils::writeFileTrunc(const std::string& filepath, const std::string& content) {
	std::ofstream myfile;
	myfile.open(filepath, std::ofstream::trunc);
	myfile << content;
	myfile.close();

	return false;
}
bool Utils::writeFileAppend(const std::string& filepath, const std::string& content) {
	std::ofstream myfile;
	myfile.open(filepath, std::ofstream::app);
	myfile << content;
	myfile.close();

	return false;
}
std::wstring Utils::toWStr(const glm::vec3& vec) {
	std::wstringstream ss;
	ss << "[X: " << vec.x << ", Y: " << vec.y << ", Z: " << vec.z << "]";
	return ss.str();
}

std::string Utils::toStr(const glm::vec4& vec) {
	std::stringstream ss;
	ss.precision(2);
	ss << "[X: " << vec.x << ", Y: " << vec.y << ", Z: " << vec.z << ", W: " << vec.w << "]";
	return ss.str();
}

std::string Utils::toStr(const glm::vec3& vec) {
	std::stringstream ss;
	ss.precision(2);
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

static int g_seed = 123123;
int Utils::fastrand() {
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}
void Utils::setfastrandSeed(int seed) {
	g_seed = seed;
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

float Utils::wrapValue(float value, float lowerBound, float upperBound) {
	float val = value;
	value = std::fmodf(value - lowerBound, upperBound - lowerBound) + lowerBound;
	if ( value < lowerBound ) {
		value += (upperBound - lowerBound);
	}
	return value;
}

// Returns the pitch and yaw from a normalized direction vector: x = pitch, y = yaw
glm::vec2 Utils::getRotations(const glm::vec3& dir) {
	float yaw = atan2(dir.z, dir.x != 0.f ? dir.x : 0.01f);
	float pitch = asin(-dir.y);
	return glm::vec2(pitch, yaw);
}

uint32_t Utils::packQuarterFloat(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	uint32_t out = (a << 24) | (b << 16) | (c << 8) | d;
	return out;
}

uint32_t Utils::unpackQuarterFloat(uint32_t in, unsigned int index) {
	uint32_t out = (in >> ((3-index) * 8)) & 255;
	return out;
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
	size_t offset = 0;
	while (true) {
		if (match = strstr(source + offset, token.c_str())) {
			bool left = match == source || isspace((match - 1)[0]);
			match += token.size();
			bool right = match != '\0' || isspace(match[0]); // might need to be match + 1

			// Ignore match if line contains "SAIL_IGNORE"
			const char* newLine = strchr(match, '\n');
			size_t lineLength = newLine - match;
			char* lineCopy = (char*)malloc(lineLength + 1);
			memset(lineCopy, '\0', lineLength + 1);
			strncpy_s(lineCopy, lineLength + 1, match, lineLength);
			if (strstr(lineCopy, "SAIL_IGNORE")) {
				free(lineCopy);
				offset += (match - source) + lineLength;
				continue;
			} else {
				free(lineCopy);
				return match;
			}
		} else {
			break;
		}
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

void Logger::Log(const std::string& msg, const std::string& file, const std::string& line) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0x0F);
	std::string message = ("LOG: " + msg);
#ifdef _DEBUG
	message = std::filesystem::path(file).filename().string() + "@" + line + " - " + message;
#endif
	std::cout << message << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);

	// Print to in-game console
	Application::getInstance()->getConsole().addLog(message);
}

void Logger::Warning(const std::string& msg, const std::string& file, const std::string& line) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0xE0);
	std::string message = ("WARNING: " + msg);
#ifdef _DEBUG
	message = std::filesystem::path(file).filename().string() + "@" + line + " - " + message;
#endif
	std::cout << message << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);

	// Print to in-game console
	Application::getInstance()->getConsole().addLog(message, ConsoleCommands::WARNING_COLOR);

#ifdef _SAIL_BREAK_ON_WARNING
	__debugbreak();
#endif
}

void Logger::Error(const std::string& msg, const std::string& file, const std::string& line) {
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save currently set color
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hstdout, &csbi);

	SetConsoleTextAttribute(hstdout, 0xC0);
	std::string message = ("ERROR: " + msg);
#ifdef _DEBUG
	message = std::filesystem::path(file).filename().string() + "@" + line + " - " + message;
#endif
	std::cout << message << std::endl;

	// Revert color
	SetConsoleTextAttribute(hstdout, csbi.wAttributes);

	// Print to in-game console
	Application::getInstance()->getConsole().addLog(message, ConsoleCommands::ERROR_COLOR);

#ifdef _SAIL_BREAK_ON_ERROR
	__debugbreak();
#endif
}
