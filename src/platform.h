#pragma once
#include "type_alias.h"
#include <SDL3/SDL.h>

class Gameboy;
class Platform
{
public:
	static Platform& getInstance()
	{
		static Platform platform;
		return platform;
	}

	uint16* getLcdTexturePtr() const;
	void mainLoop(Gameboy& gameboy);
private:
	Platform();
	Platform(const Platform&) = delete;
	Platform& operator=(const Platform&) = delete;

	void render() const;
	bool m_running;
	SDL_Window* m_window;
	SDL_Event m_event;
	SDL_Renderer* m_renderer;
	SDL_Texture* m_lcdTexture;
	void* m_lcdTexturePtr;
	int m_lcdTexturePitch;
};