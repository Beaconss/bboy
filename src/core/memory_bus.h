#pragma once
#include "../hardware_registers.h"
#include "timers.h"
#include "ppu/ppu.h"

#include <array>
#include <fstream>

class Gameboy;

class MemoryBus
{
public:
	MemoryBus(Gameboy& gb);

	void cycle();
	uint8 read(const uint16 addr) const;
	void write(const uint16 addr, const uint8 value);
	void loadRom(char const* filePath);

private:
	void loadBootRom();

	Gameboy& m_gameboy;
	std::array<uint8, 0xFFFF + 1> m_memory;

	uint16 m_dmaTransferCurrentAddress;
	bool m_dmaTransferInProcess;
	bool m_dmaTransferEnableNextCycle;
};

