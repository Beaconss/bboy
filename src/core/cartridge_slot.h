#pragma once
#include "../type_alias.h"

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
	bool isValid() const;
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

	bool m_isValid;
	std::filesystem::path m_cartridgePath;

	std::vector<uint8> m_rom;
	std::vector<uint8> m_ram;
	MbcType m_mbc;
	bool m_hasRam;
	bool m_hasBattery;
	bool m_hasClock;

	uint16 m_romBanks;
	uint16 m_ramBanks;
	uint32 m_romSize;
	uint32 m_ramSize;

	uint8 m_romBankIndex;
	uint16 m_zeroBankIndex;
	uint16 m_highBankIndex;
	uint16 m_romBankIndexMask;
	uint8 m_ramBankIndex;
	bool m_modeFlag;
	bool m_isExternalRamEnabled;
};

