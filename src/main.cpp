#include "Core/gameboy.h"
#include "platform.h"

#include <iostream>

int main()
{
	Platform& platform = Platform::getInstance();
    Gameboy gameboy;
	platform.mainLoop(gameboy);
}