#include "platform.h"
#include "core/gameboy.h"
#include <SDL3/SDL_opengl.h>
#include <iostream>
#include <future>

Platform::Platform()
	: m_running{true}
    , m_window{}
    , m_event{}
    , m_renderer{}
    , m_screenTexture{}
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) std::cerr << "SDL failed to initialize " << SDL_GetError() << '\n';
    
    m_window = SDL_CreateWindow("std-boy", PPU::lcdWidth * 5, PPU::lcdHeight * 5, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if(!m_window) std::cerr << "SDL window failed to initialize " << SDL_GetError() << '\n';
    
    m_renderer = SDL_CreateRenderer(m_window, "opengl");
    if(!m_renderer) std::cerr << "SDL renderer failed to initialize " << SDL_GetError() << '\n';
    SDL_SetRenderVSync(m_renderer, 0);
    SDL_SetRenderLogicalPresentation(m_renderer, PPU::lcdWidth, PPU::lcdHeight, SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    
    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PixelFormat::SDL_PIXELFORMAT_RGB565 , SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, PPU::lcdWidth, PPU::lcdHeight);
    if(!m_screenTexture) std::cerr << "SDL texture failed to initialize " << SDL_GetError() << '\n';
    SDL_SetTextureScaleMode(m_screenTexture, SDL_SCALEMODE_NEAREST);
}

void Platform::mainLoop(Gameboy& gameboy)
{
    constexpr float TARGET_FPS{59.7f};
    constexpr float CAPPED_FRAME_TIME{1000.f / TARGET_FPS};

    bool fpsLimit{true};
    uint64_t start{};
    uint64_t end{};
    float frametime{CAPPED_FRAME_TIME};

    auto fpsStart{std::chrono::steady_clock::now()};
    while(m_running)
    {
        while(SDL_PollEvent(&m_event))
        {
            switch(m_event.type)
            {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;
            case SDL_EVENT_DROP_FILE:
                gameboy.openRom(m_event.drop.data);
                break;
            case SDL_EVENT_KEY_DOWN:
                if(m_event.key.scancode == SDL_SCANCODE_SPACE) fpsLimit = !fpsLimit;
                else if(m_event.key.scancode == SDL_SCANCODE_BACKSPACE) gameboy.hardReset();
            }
        }

        start = SDL_GetPerformanceCounter();
  
        if(gameboy.hasRom()) gameboy.frame();
        updateScreen(gameboy.getPPU().getLcdBuffer());
        
        render();
            
        end = SDL_GetPerformanceCounter();

        if(fpsLimit) 
        {
            frametime = (SDL_GetPerformanceCounter() - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.f;
            if(frametime < CAPPED_FRAME_TIME) SDL_Delay(static_cast<uint32>(CAPPED_FRAME_TIME - frametime));           
        }
        frametime = (SDL_GetPerformanceCounter() - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.f;
        
        auto fpsEnd{std::chrono::steady_clock::now()};
        if(std::chrono::duration<double, std::milli>(fpsEnd - fpsStart) > std::chrono::duration<double, std::milli>(700))
        {
            fpsStart = std::chrono::steady_clock::now();
            SDL_SetWindowTitle(m_window, std::string(gameboy.getRomName() + ' ' + std::to_string(1000.f / frametime)).c_str());
        }
    }
    
    SDL_DestroyTexture(m_screenTexture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Platform::updateScreen(const uint16* data)
{
    SDL_UpdateTexture(m_screenTexture, nullptr, data, PPU::lcdWidth * sizeof(uint16));
}

void Platform::render() const
{
    SDL_RenderClear(m_renderer);
    SDL_RenderTexture(m_renderer, m_screenTexture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}