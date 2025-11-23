#include "config.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

static const std::string defaultConfig
{
    "volume=" + std::to_string(0.8f) +
    "\npalette=green" 
};

Config::Config()
    : m_volume{}
    , m_palette{}
{
    namespace fs = std::filesystem;
    if(!fs::exists(fileName))
    {
        std::ofstream configOut(fileName);
        configOut << defaultConfig;
    }

    std::ifstream configFile(fileName, std::ios::ate);
    if(configFile.fail())
    {
        std::cerr << "couldn't open config file\n";
        return;
    }

    auto size{configFile.tellg()};
    configFile.seekg(0);    
    std::string config;
    config.resize(size);
    configFile.read(config.data(), size);
    configFile.close();

    std::string volume{};
    int tokenParsed{};
    bool tokenFound{};
    for(auto c : config)
    {
        if(tokenFound)
        {
            if(c == '\n' || c == ' ')
            {
                ++tokenParsed;
                tokenFound = false;
                continue;
            }
            if(tokenParsed == 0) volume.push_back(c);
            else if(tokenParsed == 1) m_palette.push_back(c);
        }
        else if(c == '=') tokenFound = true;
    }

    m_volume = std::stof(volume);
}

float Config::getVolume() const
{
    return m_volume;
}

std::string_view Config::getPalette() const
{
    return m_palette;
}