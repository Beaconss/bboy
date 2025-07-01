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

	static constexpr std::array<uint16, 16> CGB_ONLY_ADDRESSES
	{
		0xFF4D, 0xFF4F, 0xFF51, 0xFF52,
		0xFF53, 0xFF54, 0xFF55, 0xFF56,
		0xFF68, 0xFF69, 0xFF6A, 0xFF6B,
		0xFF6C, 0xFF70, 0xFF76, 0xFF77,
	};

	Gameboy& m_gameboy;
	std::array<uint8, 0xFFFF + 1> m_memory;

	uint16 m_dmaTransferCurrentAddress;
	bool m_dmaTransferInProcess;
	bool m_dmaTransferEnableNextCycle;
};

