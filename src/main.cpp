#include <core/gameboy.h>
#include <platform.h>

int main(int argc, char** argv)
{
	Platform& platform = Platform::getInstance();
	Gameboy* gameboy{new Gameboy};
	if(argc == 2) gameboy->loadCartridge(argv[1]);
	platform.mainLoop(*gameboy);
	delete gameboy;
	return 0;
}