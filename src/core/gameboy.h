#pragma once
#include "type_alias.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "core/apu/apu.h"
#include "core/timers.h"
#include "core/input.h"
#include "core/ppu/ppu.h"
#include <memory>

class Gameboy
{
public:
	Gameboy();
	~Gameboy();

	void reset();
	void frame(float frameTime);

	void loadCartridge(const std::filesystem::path& filePath);
	void hardReset();
	void nextCartridge();
	bool hasCartridge();
	uint16 currentCycle() const;

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