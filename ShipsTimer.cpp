#include "ShipsTimer.h"

#define ConsoleWriteLine( fmt, ... ) {ConsoleWrite( fmt, ##__VA_ARGS__  );OutputDebugStringA("\r\n");}
#define ConsoleWrite( fmt, ... ) {char szBuf[MAX_PATH]; sprintf_s(szBuf,fmt, ##__VA_ARGS__ ); OutputDebugStringA(szBuf);}


ShipsTimer::ShipsTimer()	//计时器构造函数
{
	Now = steady_clock::now();

	currentTick = Now.time_since_epoch().count();		//避免重复调用
	createdTick = currentTick;
	previousTick = currentTick;

	currDealtTick = 1;
	timeExists = 0;
	count = 0;
	realTimeExists = 0;
	pausedTick = currentTick;	//初始化,避免重复调用Now.time_since_epoch().count函数
	startedTick = currentTick;
	paused = false;
}


void ShipsTimer::Pause()
{
	if (!paused)
	{
		Now = steady_clock::now();
		pausedTick = Now.time_since_epoch().count();
		paused = true;	//paused 后 updateTimer将不起作用,但时间继续流逝
	}
}

void ShipsTimer::Start()
{
	if (paused)
	{
		Now = steady_clock::now();
		startedTick = Now.time_since_epoch().count();		//计时器之前可能已经停止工作,因此重新获取当前tick
		paused = false;
		UpdateTimer();
	}
}

//帧更新计时
void ShipsTimer::UpdateTimer()
{
	if (!paused)
	{
		Now = steady_clock::now();
		currentTick = Now.time_since_epoch().count();
		timeExists = currentTick - createdTick - (startedTick - pausedTick);
		realTimeExists = currentTick - createdTick;
		currDealtTick = currentTick - previousTick;
		previousTick = currentTick;
		count++;
	}
	else
	{
		Now = steady_clock::now();
		currentTick = Now.time_since_epoch().count();
		realTimeExists = currentTick - createdTick;
	}
}





//返回1则为暂停,返回0则为计时状态
BOOL ShipsTimer::GetTimerStatus()
{
	return paused;
}


////////////////////////////FrameTimer/////////////////////////////////////////////////////

FrameTimer::FrameTimer()
{
	ShipsTimer();
	fps = 1;
	prev_fps = -1;
	index = 0;
	time_wait = 0;
}


//n_frame 采样帧的数量
double FrameTimer::GetAverageFps(int n_frame)
{
	if (frame_buffer.size() != n_frame)
	{
		frame_buffer.resize(n_frame);
	}

	if (index < n_frame)
	{
		frame_buffer[index] = GetFps();
		index += 1;
		return -1;//未完成缓冲区填充
	}
	else
	{
		double sum = 0;
		index = 0; //重置索引


		for (int i = 0; i < n_frame; i++)
		{
			sum += frame_buffer[i];
		}

		return sum / n_frame;
	}



}

void FrameTimer::UpdateTimer()
{
	ShipsTimer::UpdateTimer();
	if (currDealtTick != 0)
	{
		fps = 1e09 / currDealtTick;
	}
	aver_fps = GetAverageFps(sample_nframe);

}

void FrameTimer::FrameSync(int sync_fps)
{
	if (aver_fps != -1)
	{
		prev_fps = aver_fps;
	}

	if (sync_fps < prev_fps)
	{
		time_wait = 1e09 * (prev_fps - sync_fps) / (sync_fps * prev_fps);
		Sleep(time_wait);
	}
	else
	{
	}

}



