#pragma once
#include "type_alias.h"
#include <utility>

namespace MemoryRegions //each pair has the start as first and the end as second
{
constexpr std::pair<uint16, uint16> romBank0{0, 0x3FFF};
constexpr std::pair<uint16, uint16> romBank1{0x4000, 0x7FFF};
constexpr std::pair<uint16, uint16> vram{0x8000, 0x9FFF};
constexpr std::pair<uint16, uint16> externalRam{0xA000, 0xBFFF};
constexpr std::pair<uint16, uint16> workRam0{0xC000, 0xCFFF};
constexpr std::pair<uint16, uint16> workRam1{0xD000, 0xDFFF};
constexpr std::pair<uint16, uint16> echoRam{0xE000, 0xFDFF};
constexpr std::pair<uint16, uint16> oam{0xFE00, 0xFE9F};
constexpr std::pair<uint16, uint16> notUsable{0xFEA0, 0xFEFF};
constexpr std::pair<uint16, uint16> hardwareRegisters{0xFF00, 0xFF7F}; //single registers are in hardware_registers.h
constexpr std::pair<uint16, uint16> highRam{0xFF80, 0xFFFE};
} //namespace MemoryRegions
