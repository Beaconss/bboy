#pragma once
#include "type_alias.h"

#include <SDL3/SDL.h>

#include <iostream>

class Platform //this is a singleton
{
public:
	static Platform& getInstance()
	{
		static Platform platform;
		return platform;
	}

	void mainLoop();

private:
	Platform();

	Platform(const Platform&) = delete;
	Platform& operator=(const Platform&) = delete;

	bool m_isRunning;
	SDL_Window* m_window;
	SDL_Event m_event;
	SDL_Renderer* m_renderer;
	SDL_Texture* m_displayTexture;
};