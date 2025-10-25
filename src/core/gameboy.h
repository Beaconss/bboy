#pragma once
#include <type_alias.h>
#include <hardware_registers.h>
#include <core/bus.h>
#include <core/cpu.h>
#include <core/ppu/ppu.h>
#include <core/apu/apu.h>
#include <core/timers.h>
#include <core/input.h>

#include <array>
#include <fstream>
#include <memory>

class Gameboy
{
public:
	Gameboy();
	~Gameboy();

	void reset();
	void frame(float frameTime);

	void loadCartridge(const std::filesystem::path& filePath);
	void nextCartridge();
	const bool hasCartridge() const;

	const PPU& getPPU() const;
private:
	friend class Bus;
	void mCycle();

	Bus m_bus;
	CPU m_cpu;
	std::unique_ptr<PPU> m_ppu;
	APU m_apu;

	Timers m_timers;
	Input m_input;

	uint16 m_currentCycle;
};