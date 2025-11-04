#include "core/apu/audio_thread.h"
#include "core/apu/apu.h"

AudioThread::AudioThread(APU& apu)
	: m_apu{apu}
	, m_thread{&AudioThread::threadLoop, this}
	, m_mutex{}
	, m_condition{}
	, m_executing{}
{
	m_thread.detach();
}

void AudioThread::unlock()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_condition.wait(lock, [this]{return !m_executing;});
	m_executing = true;
	m_condition.notify_one();
}

void AudioThread::waitToFinish()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_condition.wait(lock, [this]{return !m_executing;});
	#ifdef _DEBUG
	if(m_executing) throw std::exception();
	#endif
}

void AudioThread::threadLoop()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condition.wait(lock, [this]{return m_executing;});
		m_apu.finishFrame();
		m_executing = false;
		m_condition.notify_one();
	}
}