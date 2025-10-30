#pragma once
#include "type_alias.h"
#include "memory_regions.h"
#include "core/cartridge_slot.h"
#include "core/ppu/ppu.h"
#include <vector>

class Gameboy;
class std::filesystem::path;
class Bus
{
	public:
	Bus(Gameboy& gb);
	
	enum class Component
	{
		cpu,
		ppu,
		bus,
		timers,
	};
	
	void reset();
	void handleDmaTransfer();

	void loadCartridge(const std::filesystem::path& filePath);
	const bool hasCartridge() const;
	void nextCartridge();
	uint8 read(const uint16 addr, const Component component) const;
	void write(const uint16 addr, const uint8 value, const Component component);
	
	void fillSprite(uint16 oamAddr, PPU::Sprite& sprite) const;
private:
	bool isInExternalBus(const uint16 addr) const;
	
	static constexpr int echoRamOffset{MemoryRegions::echoRam.first - MemoryRegions::workRam0.first};
	
	Gameboy& m_gameboy;
	std::vector<uint8> m_memory;
	CartridgeSlot m_cartridgeSlot;
	
	bool m_externalBusBlocked;
	bool m_vramBusBlocked;
	uint16 m_dmaTransferCurrentAddress;
	bool m_dmaTransferInProcess;
	uint8 m_dmaTransferEnableDelay;
};