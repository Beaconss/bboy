#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "cpu.h"
#include "timers.h"

#include <array>

class Gameboy
{
public:
	Gameboy();

	void cycle();

	uint8 readMemory(uint16 addr);
	void writeMemory(uint16 addr, uint8 valueHL);

private:
	CPU m_cpu;
	std::array<uint8, 0xFFFF> m_memory;
	Timers m_timers;
};
