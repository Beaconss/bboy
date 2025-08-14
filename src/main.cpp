#include "Core/gameboy.h"
#include "platform.h"

#include <iostream>
#include <chrono>
#include <memory>

int main()
{
	Platform& platform = Platform::getInstance();

    std::unique_ptr<Gameboy> gameboy {std::make_unique<Gameboy>()};

	platform.mainLoop(*gameboy);
}

