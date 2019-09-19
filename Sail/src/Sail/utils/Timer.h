#pragma once

#include <time.h>


class Timer {

public:

	void startTimer() {

		LARGE_INTEGER frequencyCount;
		QueryPerformanceFrequency(&frequencyCount);

		m_countsPerSecond = static_cast<double>(frequencyCount.QuadPart);

		QueryPerformanceCounter(&frequencyCount);
		m_counterStart = frequencyCount.QuadPart;

	}

	INT64 getStartTime() const {
		return m_counterStart;
	}

	double getTime() {

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		return static_cast<double>((currentTime.QuadPart - m_counterStart) / m_countsPerSecond);

	}

	// returns time in seconds from time
	double getTimeSince(const INT64 time) {
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		return static_cast<double>((currentTime.QuadPart - time) / m_countsPerSecond);
	}

	double getFrameTime() {

		LARGE_INTEGER currentTime;
		INT64 elapsedTime;
		QueryPerformanceCounter(&currentTime);

		elapsedTime = currentTime.QuadPart - m_oldframeTime;
		m_oldframeTime = currentTime.QuadPart;

		if (elapsedTime < 0)
			elapsedTime = 0;

		return elapsedTime / m_countsPerSecond;

	}

private:
	double m_countsPerSecond = 0.0;
	INT64 m_counterStart = 0;

	INT64 m_oldframeTime = 0;

};

