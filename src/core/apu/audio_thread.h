#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

class APU;
class AudioThread
{
public:
  AudioThread(APU& apu);

  void unlock();
  void waitToFinish();
  void shutdown();

private:
  void threadLoop();

  APU& m_apu;
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::atomic<bool> m_executing;
  bool m_shutdown;
};
