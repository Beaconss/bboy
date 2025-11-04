#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

class APU;
class AudioThread
{
public:
	AudioThread(APU& apu);

	void unlock();
	void waitToFinish();
private:
	void threadLoop();
	
	APU& m_apu;
	std::jthread m_thread;
	std::mutex m_mutex;
	std::condition_variable m_condition;
	bool m_executing;
};