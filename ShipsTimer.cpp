#include "ShipsTimer.h"

#define ConsoleWriteLine( fmt, ... ) {ConsoleWrite( fmt, ##__VA_ARGS__  );OutputDebugStringA("\r\n");}
#define ConsoleWrite( fmt, ... ) {char szBuf[MAX_PATH]; sprintf_s(szBuf,fmt, ##__VA_ARGS__ ); OutputDebugStringA(szBuf);}


ShipsTimer::ShipsTimer()	//��ʱ�����캯��
{
	Now = steady_clock::now();

	currentTick = Now.time_since_epoch().count();		//�����ظ�����
	createdTick = currentTick;
	previousTick = currentTick;

	currDealtTick = 1;
	timeExists = 0;
	count = 0;
	realTimeExists = 0;
	pausedTick = currentTick;	//��ʼ��,�����ظ�����Now.time_since_epoch().count����
	startedTick = currentTick;
	paused = false;
}


void ShipsTimer::Pause()
{
	if (!paused)
	{
		Now = steady_clock::now();
		pausedTick = Now.time_since_epoch().count();
		paused = true;	//paused �� updateTimer����������,��ʱ���������
	}
}

void ShipsTimer::Start()
{
	if (paused)
	{
		Now = steady_clock::now();
		startedTick = Now.time_since_epoch().count();		//��ʱ��֮ǰ�����Ѿ�ֹͣ����,������»�ȡ��ǰtick
		paused = false;
		UpdateTimer();
	}
}

//֡���¼�ʱ
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





//����1��Ϊ��ͣ,����0��Ϊ��ʱ״̬
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


//n_frame ����֡������
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
		return -1;//δ��ɻ��������
	}
	else
	{
		double sum = 0;
		index = 0; //��������


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



