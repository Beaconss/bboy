#pragma once
#include <type_alias.h>
#include <hardware_registers.h>
#include <memory_regions.h>
#include <core/cartridge_slot.h>

#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <filesystem>

class Gameboy;

class Bus
{
public:
	Bus(Gameboy& gb);

	enum class Component
	{
		CPU,
		PPU,
		BUS,
		TIMERS,
	};

	void reset();
	void handleDmaTransfer();
	void loadCartridge(const std::filesystem::path& filePath);
	bool hasRom() const;
	void nextCartridge();
	uint8 read(const uint16 addr, const Component component) const;
	void write(const uint16 addr, const uint8 value, const Component component);

	template<typename T>
	void fillSprite(uint16 oamAddr, T& sprite) const;

private:
	bool isInExternalBus(const uint16 addr) const;

	Gameboy& m_gameboy;
	std::vector<uint8> m_memory;
	CartridgeSlot m_cartridgeSlot;

	bool m_externalBusBlocked;
	bool m_vramBusBlocked;
	uint16 m_dmaTransferCurrentAddress;
	bool m_dmaTransferInProcess;
	int m_dmaTransferEnableDelay;
};