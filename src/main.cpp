#include "Core/gameboy.h"
#include "platform.h"

#include <SDL3/SDL.h>

#include <iostream>
#include <chrono>
#include <memory>

class Timer
{
public:
    void start()
    {
        m_StartTime = std::chrono::system_clock::now();
        m_bRunning = true;
    }

    void stop()
    {
        m_EndTime = std::chrono::system_clock::now();
        m_bRunning = false;
    }

    double elapsedMilliseconds()
    {
        std::chrono::time_point<std::chrono::system_clock> endTime;

        if(m_bRunning)
        {
            endTime = std::chrono::system_clock::now();
        }
        else
        {
            endTime = m_EndTime;
        }

        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count());
    }

    double elapsedSeconds()
    {
        return elapsedMilliseconds() / 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> m_StartTime;
    std::chrono::time_point<std::chrono::system_clock> m_EndTime;
    bool                                               m_bRunning = false;
};

int main()
{
	Platform& platform = Platform::getInstance();

    std::unique_ptr<Gameboy> gb {std::make_unique<Gameboy>()};

    //Timer timer;
    //timer.start();

    //std::cout << "\n\n" << timer.elapsedMilliseconds();

	platform.mainLoop(*gb);

	return 0;
}

