#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "memory_bus.h"
#include "cpu.h"
#include "ppu/ppu.h"
#include "timers.h"

#include <array>
#include <fstream>

class Gameboy
{
public:
	Gameboy();
	void cycle();

private:
	friend MemoryBus;

	MemoryBus m_memoryBus;
	CPU m_cpu;
	PPU m_ppu;
	Timers m_timers;
};
