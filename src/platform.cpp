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
    m_renderer = SDL_CreateRenderer(m_window, "opengl");
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';

    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB565 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!m_screenTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';

    SDL_SetRenderVSync(m_renderer, 0);
    SDL_SetTextureScaleMode(m_screenTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(m_renderer, 160, 144, SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

void Platform::mainLoop(Gameboy& gameboy)
{
    constexpr int CYCLES_PER_FRAME{17556};

    using clock = std::chrono::steady_clock;
    using seconds = std::chrono::duration<double>;

    auto last_fps_print = clock::now();
    int frame_count = 0;

    while(m_running)
    {
        auto start = clock::now();

        while(SDL_PollEvent(&m_event))
        {
            if(m_event.type == SDL_EVENT_QUIT) 
            {
                m_running = false;
            }
            else if(m_event.type == SDL_EVENT_KEY_DOWN && m_event.key.scancode == SDL_SCANCODE_SPACE)
            {
                static bool a{};
                if(a) SDL_SetRenderVSync(m_renderer, 0);
                else SDL_SetRenderVSync(m_renderer, 1);
                a ^= 1;
            }
        }
     
        if(gameboy.hasRom())
        {
            for(int i{}; i < CYCLES_PER_FRAME; ++i)
            {
                gameboy.cycle();
            }
        }
        updateScreen(gameboy.getLcdBuffer());
        render();

        frame_count++;
        auto finish = clock::now();
        auto elapsed = std::chrono::duration_cast<seconds>(finish - last_fps_print).count();
        if(elapsed >= .4)
        {
            double fps = frame_count / elapsed;
            SDL_SetWindowTitle(m_window, std::to_string(fps).c_str());
            frame_count = 0;
            last_fps_print = finish;
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

void Platform::updateScreen(const uint16* data)
{
    SDL_UpdateTexture(m_screenTexture, nullptr, data, SCREEN_WIDTH * 2);
}
