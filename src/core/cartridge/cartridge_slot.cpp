#include "core/cartridge/cartridge_slot.h"
#include "core/cartridge/cartridges.h"
#include <iostream>
#include <fstream>

CartridgeSlot::CartridgeSlot()
	: m_cartridge{}
	, m_cartridgePath{}
	, m_cartridgeHasClock{}
	, m_frameCounter{}
{
	reset();
}

CartridgeSlot::~CartridgeSlot()
{
	reset();
}

void CartridgeSlot::reset()
{
	if(m_cartridge) m_cartridge->save(m_cartridgePath);
	delete m_cartridge;
	m_cartridge = nullptr;
	m_cartridgeHasClock = false;
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
		break;*/
	case 0x0f: m_cartridge = new CartridgeMbc3(path, false, false, true); m_cartridgeHasClock = true; break;
	case 0x10: m_cartridge = new CartridgeMbc3(path, true, true, true); m_cartridgeHasClock = true; break;
	case 0x11: m_cartridge = new CartridgeMbc3(path); break;
	case 0x12: m_cartridge = new CartridgeMbc3(path, true); break;
	case 0x13: m_cartridge = new CartridgeMbc3(path, true, true); break;
	case 0x19: m_cartridge = new CartridgeMbc5(path); break;
	case 0x1A: m_cartridge = new CartridgeMbc5(path, true); break;
	case 0x1B: m_cartridge = new CartridgeMbc5(path, true, true); break;
	case 0x1C: m_cartridge = new CartridgeMbc5(path, false, false, true); break;
	case 0x1D: m_cartridge = new CartridgeMbc5(path, true, false, true); break;
	case 0x1E: m_cartridge = new CartridgeMbc5(path, true, true, true); break;
	}

	m_cartridgePath = path;
	m_cartridgeName = path.filename().stem();
}

void CartridgeSlot::reloadCartridge()
{
	loadCartridge(m_cartridgePath);
}

const std::string& CartridgeSlot::getCartridgeName() const
{
	return m_cartridgeName;
}

bool CartridgeSlot::hasCartridge() const
{
	return static_cast<bool>(m_cartridge);
}

void CartridgeSlot::clockFrame()
{
	if(m_cartridgeHasClock)
	{
		constexpr int oneSecondFrames{60}; //circa
		if(++m_frameCounter == oneSecondFrames) 
		{
			m_frameCounter = 0;
			dynamic_cast<CartridgeMbc3*>(m_cartridge)->rtcCycle();  
		}
	}
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