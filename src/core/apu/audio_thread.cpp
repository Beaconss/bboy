#include <core/apu/audio_thread.h>
#include <core/apu/apu.h>

AudioThread::AudioThread(APU& apu)
	: m_apu{apu}
	, m_thread{&AudioThread::threadLoop, this}
	, m_mutex{}
	, m_mutexCondition{}
	, m_shouldExecute{}
{
	m_thread.detach();
}

void AudioThread::unlock()
{
	const std::lock_guard<std::mutex> lock(m_mutex);
	assert(!m_shouldExecute);
	m_shouldExecute = true;
	m_mutexCondition.notify_one();
}

void AudioThread::threadLoop()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_mutexCondition.wait(lock, [this]{return m_shouldExecute;});
		m_apu.finishFrame();
		m_shouldExecute = false;
	}
}