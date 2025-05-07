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

	uint8 readMemory(const uint16 addr) const;
	void writeMemory(const uint16 addr, const uint8 value);

private:
	CPU m_cpu;
	std::array<uint8, 0xFFFF> m_memory;
	Timers m_timers;

	int m_cycleCounter;
};
