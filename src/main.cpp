#include "Core/gameboy.h"
#include "platform.h"

#include <SDL3/SDL.h>

#include <iostream>
#include <chrono>

int main()
{
	Platform& platform = Platform::getInstance();
	Gameboy gameboy;
	platform.mainLoop();
	return 0;
}