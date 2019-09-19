#include "pch.h"
#include "Timer.h"
#include <time.h>

void Timer::startTimer() {
	LARGE_INTEGER frequencyCount;
	QueryPerformanceFrequency(&frequencyCount);

	m_countsPerSecond = static_cast<double>(frequencyCount.QuadPart);

	QueryPerformanceCounter(&frequencyCount);
	m_counterStart = frequencyCount.QuadPart;
}

double Timer::getTime() {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	return static_cast<double>(currentTime.QuadPart - m_counterStart / m_countsPerSecond);
}

double Timer::getFrameTime() {

	LARGE_INTEGER currentTime;
	INT64 elapsedTime;
	QueryPerformanceCounter(&currentTime);

	elapsedTime = currentTime.QuadPart - m_oldFrameTime;
	m_oldFrameTime = currentTime.QuadPart;

	if (elapsedTime < 0)
		elapsedTime = 0;

	return elapsedTime / m_countsPerSecond;

}
