#pragma once
#include <type_alias.h>
#include <hardware_registers.h>
#include <core/bus.h>
#include <core/cpu.h>
#include <core/ppu/ppu.h>
#include <core/apu.h>
#include <core/timers.h>
#include <core/input.h>

#include <array>
#include <fstream>

class Gameboy
{
public:
	Gameboy();
	~Gameboy();
	void frame();
	void mCycle();
	void loadCartridge(const std::filesystem::path& filePath);
	void nextTest();
	void reset();
	bool hasRom() const;
	const uint16* getLcdBuffer() const;

private:
	friend class Bus;

	Bus m_bus;
	CPU m_cpu;
	PPU m_ppu;
	APU m_apu;
	Timers m_timers;
	Input m_input;
};
