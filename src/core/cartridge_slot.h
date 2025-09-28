#pragma once
#include <type_alias.h>
#include <memory_regions.h>

#include <array>
#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>

class CartridgeSlot
{
public:
	CartridgeSlot();
	
	void reset();
	void loadCartridge(const std::filesystem::path& filePath);
	bool hasCartridge() const;
	uint8 readRom(const uint16 addr) const;
	void writeRom(const uint16 addr, const uint8 value);
	uint8 readRam(const uint16 addr) const;
	void writeRam(const uint16 addr, const uint8 value);

private:
	enum MbcType
	{
		NONE,
		MBC1,
		MBC2,
		MBC3,
		MBC5,
	};

	struct CartridgeInfo
	{
		std::filesystem::path path{};
		MbcType mbc{};
		bool hasRam{};
		bool hasBattery{};
		bool hasClock{};
		bool hasRumble{};
		uint16 romBanks{};
		uint16 ramBanks{};
	};

	static constexpr std::array<const char*, 5> MBC_TYPES
	{
		"None",
		"Mbc1",
		"Mbc2",
		"Mbc3",
		"Mbc5",
	};

	static constexpr unsigned int KB_2{0x800};
	static constexpr unsigned int KB_8{0x2000};
	static constexpr unsigned int KB_16{0x4000};
	static constexpr unsigned int KB_32{0x8000};
	static constexpr unsigned int KB_64{0x10000};

	bool loadRom();
	void initializeRom();
	void initializeRam();
	void saveRam();
	void loadSave();

	bool m_hasCartridge;
	CartridgeInfo m_cartridgeInfo;
	std::vector<uint8> m_rom;
	std::vector<uint8> m_ram;

	uint32 m_romSize;
	uint32 m_ramSize;

	uint16 m_romBankIndex;
	uint16 m_romBankIndexMask;
	uint8 m_ramBankIndex;
	bool m_modeFlag;
	bool m_isExternalRamEnabled;
};

