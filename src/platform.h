#pragma once
#include "type_alias.h"
#include <SDL3/SDL.h>

#include <iostream>
#include <array>
#include <string>

constexpr int SCREEN_WIDTH{160};
constexpr int SCREEN_HEIGHT{144};

class Gameboy;

class Platform //this is a singleton
{
public:
	static Platform& getInstance()
	{
		static Platform platform;
		return platform;
	}

	void mainLoop(Gameboy& gb);
	void updateScreen(uint16* data);
	void render() const;

private:
	Platform();

	Platform(const Platform&) = delete;
	Platform& operator=(const Platform&) = delete;

	bool m_running;
	SDL_Window* m_window;
	SDL_Event m_event;
	SDL_Renderer* m_renderer;
	SDL_Texture* m_screenTexture;
};