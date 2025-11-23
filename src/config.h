#pragma once
#include <string>

class Config
{
public:
    static Config& getInstance()
    {
        static Config config;
        return config;
    }

    float getVolume() const;
    std::string_view getPalette() const;
private:
    Config();    
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static constexpr std::string fileName{"config.ini"};

    float m_volume;
    std::string m_palette;
};