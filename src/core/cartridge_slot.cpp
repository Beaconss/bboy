#include <core/cartridge_slot.h>

CartridgeSlot::CartridgeSlot()
	: m_hasCartridge{}
	, m_cartridgeInfo{}
	, m_rom{}
	, m_ram{}
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
	if(m_cartridgeInfo.hasRam && m_cartridgeInfo.hasBattery) saveRam();
	m_hasCartridge = false;
	std::ranges::fill(m_rom, 0);
	std::ranges::fill(m_ram, 0);
	m_cartridgeInfo.mbc = NONE;
	m_cartridgeInfo.hasRam = false;
	m_cartridgeInfo.hasBattery = false;
	m_cartridgeInfo.hasClock = false;
	m_cartridgeInfo.romBanks = 0;
	m_cartridgeInfo.ramBanks = 0;
	m_romSize = 0;
	m_ramSize = 0;
	m_romBankIndex = 1;
	m_romBankIndexMask = 0;
	m_ramBankIndex = 0;
	m_modeFlag = false;
}

void CartridgeSlot::loadCartridge(const std::filesystem::path& filePath)
{
	m_cartridgeInfo.path = std::filesystem::absolute(filePath);
	if(!loadRom())
	{
		m_hasCartridge = false;
		return;
	}

	constexpr uint16 MBC_HEADER_ADDRESS{0x147};
	uint8 mbcValue{m_rom[MBC_HEADER_ADDRESS]};
	switch(mbcValue)
	{
	case 0x00:
		m_cartridgeInfo.mbc = NONE;
		break;
	case 0x08:
		m_cartridgeInfo.mbc = NONE;
		m_cartridgeInfo.hasRam = true;
		break;
	case 0x09:
		m_cartridgeInfo.mbc = NONE;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x01:
		m_cartridgeInfo.mbc = MBC1;
		break;
	case 0x02:
		m_cartridgeInfo.mbc = MBC1;
		m_cartridgeInfo.hasRam = true;
		break;
	case 0x03:
		m_cartridgeInfo.mbc = MBC1;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x05:
		m_cartridgeInfo.mbc = MBC2;
		break;
	case 0x06:
		m_cartridgeInfo.mbc = MBC2;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x0f:
		m_cartridgeInfo.mbc = MBC3;
		m_cartridgeInfo.hasClock = true;
		break;
	case 0x10:
		m_cartridgeInfo.mbc = MBC3;
		m_cartridgeInfo.hasClock = true;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x11:
		m_cartridgeInfo.mbc = MBC3;
		break;
	case 0x12:
		m_cartridgeInfo.mbc = MBC3;
		m_cartridgeInfo.hasRam = true;
		break;
	case 0x13:
		m_cartridgeInfo.mbc = MBC3;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x19:
		m_cartridgeInfo.mbc = MBC5;
		break;
	case 0x1A:
		m_cartridgeInfo.mbc = MBC5;
		m_cartridgeInfo.hasRam = true;
		break;
	case 0x1B:
		m_cartridgeInfo.mbc = MBC5;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		break;
	case 0x1C:
		m_cartridgeInfo.mbc = MBC5;
		m_cartridgeInfo.hasRumble = true;
		break;
	case 0x1D:
		m_cartridgeInfo.mbc = MBC5;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasRumble = true;
		break;
	case 0x1E:
		m_cartridgeInfo.mbc = MBC5;
		m_cartridgeInfo.hasRam = true;
		m_cartridgeInfo.hasBattery = true;
		m_cartridgeInfo.hasRumble = true;
		break;
	}

	initializeRom();
	initializeRam();
	if(m_cartridgeInfo.hasRam && m_cartridgeInfo.hasBattery) loadSave();
	m_hasCartridge = true;

	std::cout << "File name: " << filePath.filename() << '\n'
		<< "Rom banks: " << m_cartridgeInfo.romBanks << '\n'
		<< "Rom size: " << m_romSize << '\n'
		<< "Ram banks: " << m_cartridgeInfo.ramBanks << '\n'
		<< "Ram size: " << m_ramSize << '\n'
		<< "Has ram: " << (m_cartridgeInfo.hasRam ? "Yes\n" : "No\n")
		<< "Has battery: " << (m_cartridgeInfo.hasBattery ? "Yes\n" : "No\n")
		<< "Has rumble: " << (m_cartridgeInfo.hasRumble ? "Yes\n" : "No\n")
		<< "Mbc type: " << MBC_TYPES[m_cartridgeInfo.mbc] << "\n\n";
}

bool CartridgeSlot::hasCartridge() const
{
	return m_hasCartridge;
}

uint8 CartridgeSlot::readRom(const uint16 addr) const
{
	using namespace MemoryRegions;
	switch(m_cartridgeInfo.mbc)
	{
	case NONE: return m_rom[addr];
	case MBC1:
	{
		if(addr <= ROM_BANK_0.second)
		{
			uint8 zeroBankIndex{};
			if(m_cartridgeInfo.romBanks <= 32);
			else if(m_cartridgeInfo.romBanks == 64) zeroBankIndex = (m_ramBankIndex & 1) << 5;
			else zeroBankIndex = m_ramBankIndex << 5;

			if(m_modeFlag) return m_rom[KB_16 * zeroBankIndex + addr];
			else return m_rom[addr];
		}
		else
		{
			uint8 highBankIndex{};
			if(m_cartridgeInfo.romBanks <= 32) highBankIndex = m_romBankIndex & m_romBankIndexMask;
			else if(m_cartridgeInfo.romBanks == 64) highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b100000) | ((m_ramBankIndex & 1) << 5);
			else highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b110'0000) | (m_ramBankIndex << 5);
			return m_rom[KB_16 * highBankIndex + (addr - 0x4000)]; //so rom bank 1
		}
	}
	break;
	case MBC5:
	{
		if(addr <= ROM_BANK_0.second) return m_rom[addr];
		else return m_rom[0x4000 * m_romBankIndex +(addr - 0x4000)];
	}
	break;
	}
	return 0xFF;
}

void CartridgeSlot::writeRom(const uint16 addr, const uint8 value)
{
	using namespace MemoryRegions;
	switch(m_cartridgeInfo.mbc)
	{
	case NONE: return;
	case MBC1:
	{
		constexpr int ENABLE_RAM_END{0x1FFF};
		constexpr int RAM_BANK_END{0x5FFF};
		if(addr <= ENABLE_RAM_END)
		{
			if((value & 0xF) == 0xA && m_cartridgeInfo.hasRam) m_isExternalRamEnabled = true;
			else m_isExternalRamEnabled = false;
		}
		else if(addr <= ROM_BANK_0.second)
		{
			if((value & 0x1F) == 0) m_romBankIndex = 1; //the rom bank index is a 5 bit unsigned number
			else m_romBankIndex = value & m_romBankIndexMask;
		}
		else if(addr <= RAM_BANK_END) m_ramBankIndex = value & 0b11;
		else m_modeFlag = value & 1; //so mode select
	}
	break;
	case MBC5:
	{
		constexpr int ENABLE_RAM_END{0x1FFF};
		constexpr int ROM_BANK_LOW_END{0x2FFF};
		constexpr int RAM_BANK_END{0x5FFF};
		if(addr <= ENABLE_RAM_END)
		{
			if((value & 0xF) == 0xA && m_cartridgeInfo.hasRam && m_cartridgeInfo.ramBanks > 0) m_isExternalRamEnabled = true;
			else m_isExternalRamEnabled = false;
		}
		else if(addr <= ROM_BANK_LOW_END) 
		{
			m_romBankIndex = (m_romBankIndex & 0b1'0000'0000) | value;
			m_romBankIndex &= m_cartridgeInfo.romBanks - 1;
		}
		else if(addr <= ROM_BANK_0.second) 
		{
			m_romBankIndex = (m_romBankIndex & 0b0'1111'1111) | ((value & 1) << 8);
			m_romBankIndex &= m_cartridgeInfo.romBanks - 1;
		}
		else if(addr <= RAM_BANK_END) 
		{
			m_ramBankIndex = value & (m_cartridgeInfo.hasRumble ? 0b111 : 0xF);
		}
	}
	break;
	}
}

uint8 CartridgeSlot::readRam(const uint16 addr) const
{
	if(!m_isExternalRamEnabled) return 0xFF;

	using namespace MemoryRegions;
	switch(m_cartridgeInfo.mbc)
	{
	case NONE: return 0xFF;
	case MBC1:
	{
		if(m_cartridgeInfo.ramBanks == 1) return m_ram[(addr - EXTERNAL_RAM.first) % m_ramSize];
		else return m_ram[m_modeFlag ? KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM.first) : addr - EXTERNAL_RAM.first]; //always 4 on mbc1(assuming its not a faulty rom)
	}
	break;
	case MBC5: return m_ram[KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM.first)];
	}
	return 0xFF;
}

void CartridgeSlot::writeRam(const uint16 addr, const uint8 value)
{
	if(!m_isExternalRamEnabled) return;

	constexpr unsigned int EXTERNAL_RAM_START{0xA000};
	switch(m_cartridgeInfo.mbc)
	{
	case NONE: return;
	case MBC1:
	{
		if(m_cartridgeInfo.ramBanks == 1) m_ram[(addr - EXTERNAL_RAM_START) % m_ramSize] = value;
		else m_ram[m_modeFlag ? KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM_START) : addr - EXTERNAL_RAM_START] = value; //always 4 on mbc1(assuming its not a faulty rom)
	}
	break;
	case MBC5:
		m_ram[KB_8 * m_ramBankIndex + (addr - EXTERNAL_RAM_START)] = value;
		break;
	}
}

bool CartridgeSlot::loadRom()
{
	std::ifstream rom(m_cartridgeInfo.path, std::ios::binary | std::ios::ate);

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
	std::memcpy(m_rom.data(), buffer.get(), size);
	return true;
}

void CartridgeSlot::initializeRom()
{
	int romBanks{2};
	if(m_cartridgeInfo.mbc != NONE)
	{
		constexpr uint16 ROM_SIZE_ADDRESS{0x148};
		uint8 romSize{m_rom[ROM_SIZE_ADDRESS]};
		if(romSize > 0 && romSize < 0x9) romBanks <<= romSize;

		//weird edge cases
		if(romSize == 0x52) romBanks = 72;
		else if(romSize == 0x53) romBanks = 80;
		else if(romSize == 0x54) romBanks = 96;
	}
	m_cartridgeInfo.romBanks = romBanks;

	m_romSize = romBanks * KB_16;
	if(m_cartridgeInfo.mbc == MBC1)
	{
		m_romBankIndexMask = std::min(m_romSize / KB_32, 0b1'0000u);
		m_romBankIndexMask |= m_romBankIndexMask - 1; //fill every less significant bit
	}
	m_rom.resize(m_romSize);
}

void CartridgeSlot::initializeRam()
{
	if(m_cartridgeInfo.hasRam)
	{
		constexpr uint16 RAM_SIZE_ADDRESS{0x149};
		uint8 ramSize{m_rom[RAM_SIZE_ADDRESS]};
		switch(ramSize)
		{
		case 0x1: m_cartridgeInfo.ramBanks = 1; m_ramSize = KB_2; break; //special case
		case 0x2: m_cartridgeInfo.ramBanks = 1; break;
		case 0x3: m_cartridgeInfo.ramBanks = 4; break;
		case 0x4: m_cartridgeInfo.ramBanks = 16; break;
		case 0x5: m_cartridgeInfo.ramBanks = 8; break;
		default:
			m_cartridgeInfo.ramBanks = 0; 
			m_cartridgeInfo.hasRam = false;
			break;
		}
	}
	else m_cartridgeInfo.ramBanks = 0;

	if(m_ramSize != KB_2) m_ramSize = m_cartridgeInfo.ramBanks * KB_8;

	m_ram.resize(m_ramSize);
}

void CartridgeSlot::saveRam()
{
	std::ofstream saveFile(m_cartridgeInfo.path.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail()) std::cerr << "Couldn't open save file\n";
	saveFile.write(reinterpret_cast<char*>(m_ram.data()), m_ram.size());
}

void CartridgeSlot::loadSave()
{
	std::ifstream saveFile(m_cartridgeInfo.path.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail())
	{
		std::cout << "No save file found or couldn't open it\n";
		return;
	}
	saveFile.read(reinterpret_cast<char*>(m_ram.data()), m_ram.size());
}