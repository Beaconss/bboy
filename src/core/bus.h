#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"
#include "cartridge_slot.h"

#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <filesystem>

namespace MemoryRegions //each pair has the start as first and the end as second
{
	constexpr std::pair<uint16, uint16> ROM_BANK_0{0, 0x3FFF};
	constexpr std::pair<uint16, uint16> ROM_BANK_1{0x4000, 0x7FFF};
	constexpr std::pair<uint16, uint16> VRAM{0x8000, 0x9FFF};
	constexpr std::pair<uint16, uint16> EXTERNAL_RAM{0xA000, 0xBFFF};
	constexpr std::pair<uint16, uint16> WORK_RAM_0{0xC000, 0xCFFF};
	constexpr std::pair<uint16, uint16> WORK_RAM_1{0xD000, 0xDFFF};
	constexpr std::pair<uint16, uint16> ECHO_RAM{0xE000, 0xFDFF};
	constexpr std::pair<uint16, uint16> OAM{0xFE00, 0xFE9F};
	constexpr std::pair<uint16, uint16> NOT_USABLE{0xFEA0, 0xFEFF};
	//here would be hardware registers which are in a different header
	constexpr std::pair<uint16, uint16> HIGH_RAM{0xFF80, 0xFFFE};
}

class Gameboy;

class Bus
{
public:
	Bus(Gameboy& gb);

	enum class Component
	{
		CPU,
		PPU,
		OTHER,
	};

	void reset();
	void cycle();
	bool hasRom() const;
	uint8 read(const uint16 addr, const Component component) const;
	void write(const uint16 addr, const uint8 value, const Component component);

	template<typename T>
	void fillSprite(uint16 addr, T& sprite) const;

private:
	bool isInExternalBus(const uint16 addr) const;

	Gameboy& m_gameboy;
	std::vector<uint8> m_memory;
	CartridgeSlot m_cartridgeSlot;

	bool m_isExternalBusBlocked;
	bool m_isVramBusBlocked;
	uint16 m_dmaTransferCurrentAddress;
	bool m_dmaTransferInProcess;
	bool m_dmaTransferEnableNextCycle;
};