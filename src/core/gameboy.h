#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "bus.h"
#include "cpu.h"
#include "ppu/ppu.h"
#include "timers.h"
#include "input.h"

#include <array>
#include <fstream>

class Gameboy
{
public:
	Gameboy();
	~Gameboy();
	void cycle();
	void reset();
	bool hasRom() const;
	const uint16* getLcdBuffer() const;

private:
	friend class Bus;

	Bus m_bus;
	CPU m_cpu;
	PPU m_ppu;
	Timers m_timers;
	Input m_input;
};
