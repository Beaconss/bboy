#include "platform.h"
#include "Core/gameboy.h"

Platform::Platform()
	: m_running{true}
    , m_window{}
    , m_event{}
    , m_renderer{}
    , m_screenTexture{}
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) std::cerr << "SDL failed to initialize " << SDL_GetError();

    m_window = SDL_CreateWindow("std-boy", 160 * 4, 144 * 4, SDL_WINDOW_RESIZABLE);
    if(!m_window) std::cerr << "SDL window failed to initialize " << SDL_GetError() << '\n';
    m_renderer = SDL_CreateRenderer(m_window, SDL_GetRenderDriver(0));
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';

    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB565 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!m_screenTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';

    SDL_SetRenderVSync(m_renderer, 0);
    SDL_SetTextureScaleMode(m_screenTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(m_renderer, 160, 144, SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

void Platform::mainLoop(Gameboy& gb)
{
    constexpr int CYCLES_PER_FRAME{17556};

    while(m_running)
    {
        while(SDL_PollEvent(&m_event))
        {
            if(m_event.type == SDL_EVENT_QUIT) m_running = false;
        }

        for(int i{}; i < CYCLES_PER_FRAME; ++i)
        {
            gb.cycle();
        }
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

void Platform::updateScreen(uint16* data)
{
    SDL_UpdateTexture(m_screenTexture, nullptr, data, SCREEN_WIDTH * sizeof(uint16));
}
