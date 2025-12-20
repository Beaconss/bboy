#include "core/apu/audio_thread.h"
#include "core/apu/apu.h"

AudioThread::AudioThread(APU& apu)
  : m_apu{apu}
  , m_thread{&AudioThread::threadLoop, this}
  , m_mutex{}
  , m_condition{}
  , m_executing{}
  , m_shutdown{}
{
  m_thread.detach();
}

void AudioThread::unlock()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait(lock, [this] { return !m_executing; });
  m_executing = true;
  m_condition.notify_one();
}

void AudioThread::waitToFinish()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait(lock, [this] { return !m_executing; });
}

void AudioThread::shutdown()
{
  waitToFinish();
  m_shutdown = true;
  m_condition.notify_one();
  if(m_thread.joinable()) m_thread.join();
}

void AudioThread::threadLoop()
{
  while(true)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return m_executing || m_shutdown; });
    if(m_shutdown) break;
    m_apu.finishFrame();
    m_executing = false;
    m_condition.notify_one();
  }
}
