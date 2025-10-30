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
private:
	void threadLoop();
	
	APU& m_apu;
	std::thread m_thread;
	std::mutex m_mutex;
	std::condition_variable m_mutexCondition;
	bool m_shouldExecute;
};

