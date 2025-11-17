#include "core/cartridge/cartridge_slot.h"
#include "core/cartridge/cartridges.h"
#include <iostream>
#include <fstream>

CartridgeSlot::CartridgeSlot()
	: m_cartridge{nullptr}
{
	reset();
}

CartridgeSlot::~CartridgeSlot()
{
	reset();
}

void CartridgeSlot::reset()
{
	if(m_cartridge) m_cartridge->save();
	delete m_cartridge;
	m_cartridge = nullptr;
}

void CartridgeSlot::loadCartridge(const std::filesystem::path& path)
{
	if(m_cartridge) reset();
	if(path.extension() != ".gb")
	{
		std::cerr << "Invalid file extension " << path.extension() << '\n';
		return;
	}

	std::ifstream rom(path, std::ios::binary);
	if(rom.fail())
	{
		std::cerr << "failed to open file";
		return;
	}

	constexpr uint16 mbcHeaderAddress{0x147};
	rom.seekg(mbcHeaderAddress, std::ios::beg);
	uint8 mbcValue{};
	rom.read(reinterpret_cast<char*>(&mbcValue), 1);
	rom.close();
		
	std::cout << (int)mbcValue << '\n';
	switch(mbcValue)
	{
	case 0x00:case 0x08:case 0x09: m_cartridge = new Cartridge(path); break;
	case 0x01: m_cartridge = new CartridgeMbc1(path); break;
	case 0x02: m_cartridge = new CartridgeMbc1(path, true); break;
	case 0x03: m_cartridge = new CartridgeMbc1(path, true, true); break;
	/*case 0x05:
		//m_cartridgeInfo.mbc = MbcType::mbc2;
		break;
	case 0x06:
		//m_cartridgeInfo.mbc = MbcType::mbc2;
		//m_cartridgeInfo.hasRam = true;
		//m_cartridgeInfo.hasBattery = true;
		break;
	case 0x0f:
		//m_cartridgeInfo.mbc = MbcType::mbc3;
		//m_cartridgeInfo.hasClock = true;
		break;
	case 0x10:
		//m_cartridgeInfo.mbc = MbcType::mbc3;
		//m_cartridgeInfo.hasClock = true;
		//m_cartridgeInfo.hasRam = true;
		//m_cartridgeInfo.hasBattery = true;
		break;
	case 0x11:
		//m_cartridgeInfo.mbc = MbcType::mbc3;
		break;
	case 0x12:
		//m_cartridgeInfo.mbc = MbcType::mbc3;
		//m_cartridgeInfo.hasRam = true;
		break;
	case 0x13:
		//m_cartridgeInfo.mbc = MbcType::mbc3;
		//m_cartridgeInfo.hasRam = true;
		//m_cartridgeInfo.hasBattery = true;
		break;*/
	case 0x19: m_cartridge = new CartridgeMbc5(path); break;
	case 0x1A: m_cartridge = new CartridgeMbc5(path, true); break;
	case 0x1B: m_cartridge = new CartridgeMbc5(path, true, true); break;
	case 0x1C: m_cartridge = new CartridgeMbc5(path, false, false, true); break;
	case 0x1D: m_cartridge = new CartridgeMbc5(path, true, false, true); break;
	case 0x1E: m_cartridge = new CartridgeMbc5(path, true, true, true); break;
	}
}

void CartridgeSlot::reloadCartridge()
{
	loadCartridge(m_cartridge->getPath());
}

std::string CartridgeSlot::getCartridgeName() const
{
	return "";
}

bool CartridgeSlot::hasCartridge() const
{
	return static_cast<bool>(m_cartridge);
}

uint8 CartridgeSlot::readRom(const uint16 addr) const
{
	return m_cartridge->readRom(addr);
}

void CartridgeSlot::writeRom(const uint16 addr, const uint8 value)
{
	m_cartridge->writeRom(addr, value);
}

uint8 CartridgeSlot::readRam(const uint16 addr) const
{
	return m_cartridge->readRam(addr);
}

void CartridgeSlot::writeRam(const uint16 addr, const uint8 value)
{
	m_cartridge->writeRam(addr, value);
}