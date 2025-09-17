#pragma once
#include <iostream>
#include "type_alias.h"

namespace MemoryRegions //each pair has the start as first and the end as second
{
	constexpr std::pair<uint16, uint16> ROM_BANK_0{0, 0x3FFF};
	constexpr std::pair<uint16, uint16> ROM_BANK_1{0x4000, 0x7FFF};
	constexpr std::pair<uint16, uint16> VRAM{0x8000, 0x9FFF};
	constexpr std::pair<uint16, uint16> EXTERNAL_RAM{0xA000, 0xBFFF};
	constexpr std::pair<uint16, uint16> WORK_RAM_0{0xC000, 0xCFFF};
	constexpr std::pair<uint16, uint16> WORK_RAM_1{0xD000, 0xDFFF};
	constexpr std::pair<uint16, uint16> ECHO_RAM{0xE000, 0xFDFF};
	constexpr std::pair<uint16, uint16> OAM{0xFE00, 0xFE9F};
	constexpr std::pair<uint16, uint16> NOT_USABLE{0xFEA0, 0xFEFF};
	constexpr std::pair<uint16, uint16> HARDWARE_REGISTERS{0xFF00, 0xFF7F}; //single registers are in hardware_registers.h
	constexpr std::pair<uint16, uint16> HIGH_RAM{0xFF80, 0xFFFE};
	
}
