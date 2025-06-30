#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "cpu.h"
#include "ppu/ppu.h"
#include "timers.h"

#include <array>
#include <fstream>

class Gameboy
{
public:
	Gameboy();

	void loadRom(char const* filePath);
	void cycle();

	uint8 readMemory(const uint16 addr) const;
	void writeMemory(const uint16 addr, const uint8 value);


private:
	void loadBootRom();

	std::array<uint8, 0xFFFF + 1> m_memory;
	CPU m_cpu;
	PPU m_ppu;
	Timers m_timers;

	bool m_dmaTransferInProcess;
	bool m_dmaTransferEnableNextCycle;
	uint16 m_dmaTransferCurrentAddress;
};
