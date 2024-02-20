#pragma once

#include <Windows.h>
#include <time.h>
#include <vector>
#include <chrono>
#include <thread>

using namespace std::chrono;

class ShipsTimer
{
public:

	time_point<steady_clock> Now;

	int count;

	ShipsTimer();

	void Pause();

	void Start();

	void UpdateTimer();

	inline long long GetTimeExists()
	{
		return timeExists;
	}

	inline long long GetRealTimeExists()
	{
		return realTimeExists;
	}

	inline long long GetCurrentDealtTick()
	{
		return currDealtTick;
	}



	BOOL GetTimerStatus();

protected:
	long long timeExists;
	long long currentTick;
	long long previousTick;
	long long createdTick;
	long long realTimeExists;
	long long currDealtTick;
	long long pausedTick;
	long long startedTick;
	BOOL paused;


};

class FrameTimer :public ShipsTimer
{
public:

	int sample_nframe;

	FrameTimer();

	void UpdateTimer();

	inline double GetFps()
	{
		return fps;
	}

	inline double GetAverageFps()//外部访问平均FPS方法
	{
		return aver_fps;
	}
	void FrameSync(int sync_fps);

private:
	double fps;
	double prev_fps;//fps of previous frame
	double aver_fps;
	long long time_wait;
	std::vector<double> frame_buffer;
	int index;//采样时,帧数缓冲区当前索引

	double GetAverageFps(int n_frame);//内部计算平均Fps方法

};