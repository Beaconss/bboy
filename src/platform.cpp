#include "platform.h"

Platform::Platform()
	: m_isRunning{true}
    , m_window{}
    , m_event{}
    , m_renderer{}
    , m_displayTexture{}
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) std::cerr << "SDL failed to initialize " << SDL_GetError();

    m_window = SDL_CreateWindow("std-boy", 800, 600, SDL_WINDOW_RESIZABLE);
    if(!m_window) std::cerr << "SDL window failed to initialize " << SDL_GetError() << '\n';
    m_renderer = SDL_CreateRenderer(m_window, SDL_GetRenderDriver(0));
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';
    m_displayTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB332 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if(!m_displayTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';

    SDL_SetTextureScaleMode(m_displayTexture, SDL_SCALEMODE_NEAREST);
}

void Platform::mainLoop()
{
    int buffer[160 * 144];
    std::fill(buffer, buffer + 160 * 144, 0b11111);
    SDL_UpdateTexture(m_displayTexture, nullptr, buffer, 160 * sizeof(uint8));

    while(m_isRunning)
    {
        while(SDL_PollEvent(&m_event))
        {
            if(m_event.type == SDL_EventType::SDL_EVENT_QUIT) m_isRunning = false;
        }

        SDL_RenderClear(m_renderer);
        SDL_RenderTexture(m_renderer, m_displayTexture, nullptr, nullptr);
        SDL_RenderPresent(m_renderer);
    }
    
    SDL_DestroyTexture(m_displayTexture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}