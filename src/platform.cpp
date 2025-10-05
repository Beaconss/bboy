#include <platform.h>

Platform::Platform()
	: m_running{true}
    , m_window{}
    , m_event{}
    , m_renderer{}
    , m_screenTexture{}
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) std::cerr << "SDL failed to initialize " << SDL_GetError();
    
    m_window = SDL_CreateWindow("std-boy", 160 * 5, 144 * 5, SDL_WINDOW_RESIZABLE);
    if(!m_window) std::cerr << "SDL window failed to initialize " << SDL_GetError() << '\n';
    
    m_renderer = SDL_CreateRenderer(m_window, "opengl");
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';
    SDL_SetRenderVSync(m_renderer, 0);
    SDL_SetRenderLogicalPresentation(m_renderer, 160, 144, SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    
    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB565 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!m_screenTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';
    SDL_SetTextureScaleMode(m_screenTexture, SDL_SCALEMODE_NEAREST);
}

void Platform::mainLoop(Gameboy& gameboy)
{
    constexpr float cappedFrameTime{static_cast<float>(1000. / 60.0)};

    bool fpsLimit{true};
    uint64_t start{};
    uint64_t end{};
    float elapsedMs{};

    while(m_running)
    {
        while(SDL_PollEvent(&m_event))
        {
            switch(m_event.type)
            {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if(m_event.key.scancode == SDL_SCANCODE_SPACE) fpsLimit = !fpsLimit;
                else if(m_event.key.scancode == SDL_SCANCODE_N) gameboy.nextTest();
            }
        }

        start = SDL_GetPerformanceCounter();

        if(gameboy.hasRom()) gameboy.frame();
        updateScreen(gameboy.getLcdBuffer());
        render();

        if(fpsLimit)
        {
            end = SDL_GetPerformanceCounter();
            elapsedMs = (end - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.0f;
            SDL_Delay(static_cast<uint32>(floor(cappedFrameTime - elapsedMs)));
        }
    }
    
    SDL_DestroyTexture(m_screenTexture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Platform::updateScreen(const uint16* data)
{
    SDL_UpdateTexture(m_screenTexture, nullptr, data, SCREEN_WIDTH * sizeof(uint16));
}

void Platform::render() const
{
    SDL_RenderClear(m_renderer);
    SDL_RenderTexture(m_renderer, m_screenTexture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}