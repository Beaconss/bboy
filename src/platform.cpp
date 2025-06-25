#include "platform.h"

Platform::Platform()
	: m_running{true}
    , m_window{}
    , m_event{}
    , m_renderer{}
    , m_screenTexture{}
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) std::cerr << "SDL failed to initialize " << SDL_GetError();

    m_window = SDL_CreateWindow("std-boy", 800, 600, SDL_WINDOW_RESIZABLE);
    if(!m_window) std::cerr << "SDL window failed to initialize " << SDL_GetError() << '\n';
    m_renderer = SDL_CreateRenderer(m_window, SDL_GetRenderDriver(0));
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';
    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB332 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if(!m_screenTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';

    SDL_SetTextureScaleMode(m_screenTexture, SDL_SCALEMODE_NEAREST);
}

void Platform::mainLoop()
{
    while(m_running)
    {
        while(SDL_PollEvent(&m_event))
        {
            if(m_event.type == SDL_EventType::SDL_EVENT_QUIT) m_running = false;
        }

        render();
    }
    
    SDL_DestroyTexture(m_screenTexture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Platform::render() const
{
    SDL_RenderClear(m_renderer);
    SDL_RenderTexture(m_renderer, m_screenTexture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void Platform::updateScreen(uint8* data)
{
    SDL_UpdateTexture(m_screenTexture, nullptr, data, SCREEN_HEIGHT * sizeof(uint8));
}
