#include <core/gameboy.h>
#include <platform.h>
#include <iostream>

int main(int argc, char** argv)
{
	//SDL_InitSubSystem(SDL_INIT_AUDIO);
	//static SDL_AudioSpec spec{SDL_AudioFormat::SDL_AUDIO_U8, 1, 44100};
	//SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	//SDL_ResumeAudioStreamDevice(stream);
	//
	//constexpr int a{44100 / 60};
	//std::array<float, a> buf;
	//for(int i{}; i < a; ++i) buf[i] = float(sin(2 * M_PI * 1000 * i / 44100));
	//for(int i{}; i < 60; ++i) SDL_PutAudioStreamData(stream, buf.data(), sizeof(float) * 735);
	//std::this_thread::sleep_for(std::chrono::duration<int, std::ratio<1, 60>>(1));

	Platform& platform = Platform::getInstance();
	Gameboy* gameboy{new Gameboy};
	if(argc == 2) gameboy->loadCartridge(argv[1]);
	platform.mainLoop(*gameboy);
	delete gameboy;
	return 0;
}