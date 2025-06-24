#include "Core/gameboy.h"
#include "platform.h"

#include <SDL3/SDL.h>

#include <iostream>
#include <chrono>

int main()
{
	Platform& platform = Platform::getInstance(); //this is a singleton
	platform.mainLoop();
	return 0;
}

/*
constexpr int test{114 * 154};
Gameboy gb;
for(int i{}; i <= test; ++i)
{
	gb.cycle();
}
std::cout << '\n' << timer.elapsedMilliseconds();
*/