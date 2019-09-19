#pragma once

class Timer {
public:
	void startTimer();
	double getTime();
	double getFrameTime();

private:
	double m_countsPerSecond = 0.0;
	signed long long m_counterStart = 0;
	signed long long m_oldFrameTime = 0;
};

