#include "core/cartridge/cartridges.h"
#include "memory_regions.h"
#include <iostream>
#include <cstring>
#include <fstream>

Cartridge::Cartridge(const std::filesystem::path& path, bool hasRam, bool hasBattery)
	: m_path{std::filesystem::absolute(path)} 
	, m_rom{}
    , m_ram{}
    , m_hasRam{hasRam}
    , m_hasBattery{hasBattery}
    , m_romBanks{}
    , m_ramBanks{}
    , m_externalRamEnabled{}
    , m_romBankIndex{}
    , m_ramBankIndex{}
{
    if(!loadRom()) return;

    int romBanks{2};
	constexpr uint16 romSizeAddress{0x148};
	uint8 romSize{m_rom[romSizeAddress]};
	if(romSize > 0 && romSize < 0x9) romBanks <<= romSize;
	else if(romSize == 0x52) romBanks = 72;
	else if(romSize == 0x53) romBanks = 80;
	else if(romSize == 0x54) romBanks = 96;
	m_romBanks = romBanks;

	uint32 ramSize{};
   	if(m_hasRam)
	{
		constexpr uint16 ramSizeAddress{0x149};
		uint8 ramSizeValue{m_rom[ramSizeAddress]};
		switch(ramSizeValue)
		{
		case 0x1: m_ramBanks = 1; ramSize = kb2; break; //special case
		case 0x2: m_ramBanks = 1; break;
		case 0x3: m_ramBanks = 4; break;
		case 0x4: m_ramBanks = 16; break;
		case 0x5: m_ramBanks = 8; break;
		default:
			m_ramBanks = 0; 
			m_hasRam = false;
			break;
		}
	}
	else m_ramBanks = 0;

	if(ramSize != kb2) ramSize = kb8 * m_ramBanks;
	m_ram.resize(ramSize);
	loadSave();
}

void Cartridge::save()
{
	if(!m_hasRam || !m_hasBattery) return;

	std::filesystem::path save{m_path};
	std::ofstream saveFile(save.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail()) 
	{
		std::cerr << "Couldn't open save file\n";
		return;
	}
	saveFile.write(reinterpret_cast<const char*>(m_ram.data()), m_ram.size());
}

void Cartridge::loadSave()
{
	if(!m_hasRam || !m_hasBattery) return;

	std::filesystem::path save{m_path};
	std::ifstream saveFile(save.replace_extension(".sav"), std::ios::binary);
	if(saveFile.fail())	
	{
		std::cout << "No save file found or couldn't open it\n";
		return;
	}
	saveFile.read(reinterpret_cast<char*>(m_ram.data()), m_ram.size());
}

const std::filesystem::path& Cartridge::getPath() const
{
	return m_path;
}

uint8 Cartridge::readRom(const uint16 addr)
{
    return m_rom[addr];
}

void Cartridge::writeRom(const uint16 addr, const uint8 value)
{
    return;
}

uint8 Cartridge::readRam(const uint16 addr)
{
    return 0xFF;
}

void Cartridge::writeRam(const uint16 addr, const uint8 value)
{
    return;
}

bool Cartridge::loadRom()
{
	std::ifstream rom(m_path, std::ios::binary | std::ios::ate);

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
	memcpy(m_rom.data(), buffer.get(), size);
	return true;
}


CartridgeMbc1::CartridgeMbc1(const std::filesystem::path& path, bool hasRam, bool hasBattery)
    : Cartridge(path, hasRam, hasBattery)
    , m_ramSize{}
    , m_romBankIndexMask{}
    , m_modeFlag{}
{
	uint32 romSize = m_romBanks * kb16;
    constexpr uint16 kb32{0x8000};
	m_romBankIndexMask = std::min(romSize / kb32, 0b1'0000u);
	m_romBankIndexMask |= m_romBankIndexMask - 1; //fill every less significant bit
	std::cout << (int)m_romBankIndexMask << '\n';

    if(m_ramSize != kb2) m_ramSize = m_ramBanks * kb8;

	std::cout << "File name: " << path.filename() << '\n'
		<< "Rom banks: " << m_romBanks << '\n'
		<< "Rom size: " << romSize << '\n'
		<< "Ram banks: " << m_ramBanks << '\n'
		<< "Ram size: " << m_ramSize << '\n'
		<< "Has ram: " << (m_hasRam ? "Yes\n" : "No\n")
		<< "Has battery: " << (m_hasBattery ? "Yes\n\n" : "No\n\n");
}

uint8 CartridgeMbc1::readRom(const uint16 addr)
{
    if(addr <= MemoryRegions::romBank0.second)
	{
        uint8 zeroBankIndex{};
		if(m_romBanks <= 32);
		else if(m_romBanks == 64) zeroBankIndex = (m_ramBankIndex & 1) << 5;
		else zeroBankIndex = m_ramBankIndex << 5;

		if(m_modeFlag) return m_rom[kb16 * zeroBankIndex + addr];
		else return m_rom[addr];
	}
	else
	{
		uint8 highBankIndex{};
		if(m_romBanks <= 32) highBankIndex = m_romBankIndex & m_romBankIndexMask;
		else if(m_romBanks == 64) highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b10'0000) | ((m_ramBankIndex & 1) << 5);
		else highBankIndex = ((m_romBankIndex & m_romBankIndexMask) & ~0b110'0000) | (m_ramBankIndex << 5);
		return m_rom[kb16 * highBankIndex + (addr - 0x4000)];
	}
}

void CartridgeMbc1::writeRom(const uint16 addr, const uint8 value)
{
    constexpr uint16 enableRamEnd{0x1FFF};
    constexpr uint16 ramBankEnd{0x5FFF};
    if(addr <= enableRamEnd)
	{
		if((value & 0xF) == 0xA && m_hasRam) m_externalRamEnabled = true;
		else m_externalRamEnabled = false;
	}
	else if(addr <= MemoryRegions::romBank0.second)
	{
		if((value & 0x1F) == 0) m_romBankIndex = 1; //rom bank index is 5 bits
		else m_romBankIndex = value & m_romBankIndexMask;
	}
	else if(addr <= ramBankEnd) m_ramBankIndex = value & 0b11;
	else m_modeFlag = value & 1; //so mode select
}

uint8 CartridgeMbc1::readRam(const uint16 addr)
{
    if(!m_externalRamEnabled) return 0xFF;

	throw std::exception();
    using namespace MemoryRegions;
    if(m_ramBanks == 1) return m_ram[(addr - externalRam.first) % m_ramSize];
	else return m_ram[m_modeFlag ? kb8 * m_ramBankIndex + (addr - externalRam.first) : addr - externalRam.first];
}

void CartridgeMbc1::writeRam(const uint16 addr, const uint8 value)
{
    if(!m_externalRamEnabled) return;

	throw std::exception();
   	constexpr unsigned int externalRamStart{0xA000};
    if(m_ramBanks == 1) m_ram[(addr - externalRamStart) % m_ramSize] = value;
	else m_ram[m_modeFlag ? kb8 * m_ramBankIndex + (addr - externalRamStart) : addr - externalRamStart] = value;
}


CartridgeMbc5::CartridgeMbc5(const std::filesystem::path& path, bool hasRam, bool hasBattery, bool hasRumble)
    : Cartridge(path)
    , m_hasRumble{hasRumble}
{
    m_hasRam = hasRam;
    m_hasBattery = hasBattery;
}

uint8 CartridgeMbc5::readRom(const uint16 addr)
{
    if(addr <= MemoryRegions::romBank0.second) return m_rom[addr];
	else return m_rom[kb16 * m_romBankIndex + (addr - kb16)];
}

void CartridgeMbc5::writeRom(const uint16 addr, const uint8 value)
{
    constexpr uint16 enableRamEnd{0x1FFF};
    constexpr uint16 romBankLowEnd{0x2FFF};
    constexpr uint16 ramBankEnd{0x5FFF};
	if(addr <= enableRamEnd)
	{
		if((value & 0xF) == 0xA && m_hasRam && m_ramBanks > 0) m_externalRamEnabled = true;
		else m_externalRamEnabled = false;
	}
	else if(addr <= romBankLowEnd)
	{
		m_romBankIndex = (m_romBankIndex & 0b1'0000'0000) | value;
		m_romBankIndex &= m_romBanks - 1;
	}
	else if(addr <= MemoryRegions::romBank0.second) 
	{
		m_romBankIndex = (m_romBankIndex & 0b0'1111'1111) | ((value & 1) << 8);
		m_romBankIndex &= m_romBanks - 1;
	}
	else if(addr <= ramBankEnd) m_ramBankIndex = value & (m_hasRumble ? 0b111 : 0xF);
}

uint8 CartridgeMbc5::readRam(const uint16 addr)
{
    if(!m_externalRamEnabled) return 0xFF;

    return m_ram[kb8 * m_ramBankIndex + (addr - MemoryRegions::externalRam.first)];
}

void CartridgeMbc5::writeRam(const uint16 addr, const uint8 value)
{
    if(!m_externalRamEnabled) return;

    m_ram[kb8 * m_ramBankIndex + (addr - MemoryRegions::externalRam.first)] = value;
}