#include "cartridge_slot.h"

CartridgeSlot::CartridgeSlot()
	: m_isValid{}
	, m_cartridgePath{}
	, m_rom{}
	, m_ram{}
	, m_mbc{}
	, m_hasRam{}
	, m_hasBattery{}
	, m_hasClock{}
	, m_romBanks{}
	, m_ramBanks{}
	, m_romSize{}
	, m_ramSize{}
	, m_romBankIndex{}
	, m_romBankIndexMask{}
	, m_ramBankIndex{}
	, m_modeFlag{}
	, m_isExternalRamEnabled{}
{
	reset();
}

void CartridgeSlot::reset()
{
	if(m_hasBattery) saveRam();
	m_isValid = false;
	std::ranges::fill(m_rom, 0);
	std::ranges::fill(m_ram, 0);
	m_mbc = NONE;
	m_hasRam = false;
	m_hasBattery = false;
	m_hasClock = false;
	m_romBanks = 0;
	m_ramBanks = 0;
	m_romSize = 0;
	m_ramSize = 0;
	m_romBankIndex = 1;
	m_romBankIndexMask = 0;
	m_ramBankIndex = 0;
	m_modeFlag = false;
}

void CartridgeSlot::loadCartridge(const std::filesystem::path& filePath)
{
	m_cartridgePath = std::filesystem::absolute(filePath);
	if(!loadRom())
	{
		m_isValid = false;
		return;
	}

	constexpr uint16 MBC_HEADER_ADDRESS{0x147};
	uint8 mbcValue{m_rom[MBC_HEADER_ADDRESS]};
	switch(mbcValue)
	{
	case 0x00:
		m_mbc = NONE;
		break;
	case 0x08:
		m_mbc = NONE;
		m_hasRam = true;
		break;
	case 0x09:
		m_mbc = NONE;
		m_hasRam = true;
		m_hasBattery = true;
		break;
	case 0x01:
		m_mbc = MBC1;
		break;
	case 0x02:
		m_mbc = MBC1;
		m_hasRam = true;
		break;
	case 0x03:
		m_mbc = MBC1;
		m_hasRam = true;
		m_hasBattery = true;
		break;
	case 0x05:
		m_mbc = MBC2;
		break;
	case 0x06:
		m_mbc = MBC2;
		m_hasRam = true;
		m_hasBattery = true;
		break;
	case 0x0f:
		m_mbc = MBC3;
		m_hasClock = true;
	case 0x10:
		m_mbc = MBC3;
		m_hasClock = true;
		m_hasRam = true;
		m_hasBattery = true;
	case 0x11:
		m_mbc = MBC3;
	case 0x12:
		m_mbc = MBC3;
		m_hasRam = true;
	case 0x13:
		m_mbc = MBC3;
		m_hasRam = true;
		m_hasBattery = true;
	case 0x19:
		m_mbc = MBC5;
	case 0x1A:
		m_mbc = MBC5;
		m_hasRam = true;
	case 0x1B:
		m_mbc = MBC5;
		m_hasRam = true;
		m_hasBattery = true;
	case 0x1C:
		m_mbc = MBC5;
	case 0x1D:
		m_mbc = MBC5;
		m_hasRam = true;
	case 0x1E:
		m_mbc = MBC5;
		m_hasRam = true;
		m_hasBattery = true;
	}

	initializeRom();
	initializeRam();
	if(m_hasBattery) loadSave();
	m_isValid = true;

	std::cout << "ROM BANKS: " << m_romBanks << '\n'
		<< "ROM SIZE: " << m_romSize << '\n'
		<< "RAM BANKS: " << m_ramBanks << '\n'
		<< "RAM SIZE: " << m_ramSize << '\n';
}

bool CartridgeSlot::hasCartridge() const
{
	return m_isValid;
}

uint8 CartridgeSlot::readRom(const uint16 addr) const
{
	constexpr unsigned int ROM_BANK_0_END{0x3FFF};
	constexpr unsigned int ROM_BANK_1_END{0x7FFF};
	if(addr > 0x7FFF) __debugbreak();
	switch(m_mbc)
	{
	case NONE: return m_rom[addr];
	case MBC1:
	{
		if(addr <= ROM_BANK_0_END)
		{
			uint8 zeroBankIndex{};
			if(m_romBanks <= 32);
			else if(m_romBanks == 64) zeroBankIndex = (m_ramBankIndex & 1) << 5;
			else zeroBankIndex = m_ramBankIndex << 5;

			if(m_modeFlag) return m_rom[KB_16 * zeroBankIndex + addr];
			else return m_rom[addr];
		}
		else
		{
			uint8 highBankIndex{};
			if(m_romBanks <= 32) highBankIndex = m_romBankIndex & m_romBankIndexMask;
			else if(m_romBanks == 64) highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b100000) | ((m_ramBankIndex & 1) << 5);
			else highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b110'0000) | (m_ramBankIndex << 5);
			return m_rom[KB_16 * highBankIndex + (addr - 0x4000)]; //so rom bank 1
		}
	}
	break;
	case MBC2:
	{
	}
	break;
	}
	return 0xFF;
}

void CartridgeSlot::writeRom(const uint16 addr, const uint8 value)
{
	switch(m_mbc)
	{
	case NONE: return;
	case MBC1:
	{
		constexpr int ENABLE_RAM_END{0x1FFF};
		constexpr int ROM_BANK_END{0x3FFF};
		constexpr int RAM_BANK_END{0x5FFF};
		constexpr int MODE_SELECT_END{0x7FFF};
		if(addr <= ENABLE_RAM_END)
		{
			if((value & 0xF) == 0xA && m_hasRam) m_isExternalRamEnabled = true;
			else m_isExternalRamEnabled = false;
		}
		else if(addr <= ROM_BANK_END)
		{
			if((value & 0x1F) == 0) m_romBankIndex = 1; //mask first 5 bits because the rom bank index internally is a 5 bit unsigned number
			else m_romBankIndex = value & m_romBankIndexMask;
		}
		else if(addr <= RAM_BANK_END)
		{
			m_ramBankIndex = value & 0b11;
		}
		else m_modeFlag = value & 1; //so mode select
	}
	break;
	case MBC2:
	{
	}
	break;
	}
}

uint8 CartridgeSlot::readRam(const uint16 addr) const
{
	if(!m_isExternalRamEnabled) return 0xFF;

	switch(m_mbc)
	{
	case NONE: return 0xFF;
	case MBC1:
	{
		constexpr unsigned int EXTERNAL_RAM_START{0xA000};
		if(m_ramBanks == 1) return m_ram[(addr - EXTERNAL_RAM_START) % m_ramSize];
		else return m_ram[m_modeFlag ? KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM_START) : addr - EXTERNAL_RAM_START]; //always 4 on mbc1(assuming its not a faulty rom)

	}
	break;
	case MBC2:
	{
	}
	break;
	}
	return 0xFF;
}

void CartridgeSlot::writeRam(const uint16 addr, const uint8 value)
{
	if(!m_isExternalRamEnabled) return;

	switch(m_mbc)
	{
	case NONE: return;
	case MBC1:
	{
		constexpr unsigned int EXTERNAL_RAM_START{0xA000};
		if(m_ramBanks == 1) m_ram[(addr - EXTERNAL_RAM_START) % m_ramSize] = value;
		else m_ram[m_modeFlag ? KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM_START) : addr - EXTERNAL_RAM_START] = value; //always 4 on mbc1(assuming its not a faulty rom)
	}
	break;
	case MBC2:
	{
	}
	break;
	}
}

bool CartridgeSlot::loadRom()
{
	std::ifstream rom(m_cartridgePath, std::ios::binary | std::ios::ate);

	if(rom.fail())
	{
		std::cerr << "Failed to open file\n";
		return false;
	}

	std::streamsize size{rom.tellg()};
	std::unique_ptr<char[]> buffer{std::make_unique<char[]>(size)};
	rom.seekg(0);
	rom.read(buffer.get(), size);
	rom.close();

	m_rom.resize(size);
	for(int i{0x0}; i < size; ++i)
	{
		m_rom[i] = buffer[i];
	}
	return true;
}

void CartridgeSlot::initializeRom()
{
	int romBanks{2};
	if(m_mbc != NONE)
	{
		constexpr uint16 ROM_SIZE_ADDRESS{0x148};
		uint8 romSize{m_rom[ROM_SIZE_ADDRESS]};
		if(romSize > 0 && romSize < 0x9) romBanks <<= romSize;

		//weird edge cases
		if(romSize == 0x52) romBanks = 72;
		else if(romSize == 0x53) romBanks = 80;
		else if(romSize == 0x54) romBanks = 96;
	}
	m_romBanks = romBanks;

	m_romSize = romBanks * KB_16;
	m_romBankIndexMask = std::min(m_romSize / KB_32, 0b1'0000u);
	m_romBankIndexMask |= m_romBankIndexMask - 1; //fill every less significant bit
	m_rom.resize(m_romSize);
}

void CartridgeSlot::initializeRam()
{
	if(m_hasRam)
	{
		constexpr uint16 RAM_SIZE_ADDRESS{0x149};
		uint8 ramSize{m_rom[RAM_SIZE_ADDRESS]};
		switch(ramSize)
		{
		case 0x1: m_ramBanks = 1; m_ramSize = KB_2; break; //special case
		case 0x2: m_ramBanks = 1; break;
		case 0x3: m_ramBanks = 4; break;
		case 0x4: m_ramBanks = 16; break;
		case 0x5: m_ramBanks = 8; break;
		default: m_ramBanks = 0; break;
		}
	}
	else m_ramBanks = 0;
	if(m_ramSize != KB_2) m_ramSize = m_ramBanks * KB_8;

	m_ram.resize(m_ramSize);
}

void CartridgeSlot::saveRam()
{
	std::ofstream saveFile(m_cartridgePath.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail()) std::cerr << "Couldn't open save file\n";
	saveFile.write(reinterpret_cast<char*>(m_ram.data()), m_ram.size());
}

void CartridgeSlot::loadSave()
{
	std::ifstream saveFile(m_cartridgePath.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail())
	{
		std::cout << "No save file found or couldn't open it\n";
		return;
	}
	saveFile.read(reinterpret_cast<char*>(m_ram.data()), m_ram.size());
}