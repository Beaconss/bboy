#include "bus.h"
#include "gameboy.h"

Bus::Bus(Gameboy& gb)
	: m_gameboy{gb}
	, m_memory{}
	, m_rom{}
	, m_isExternalRamEnabled{}
	, m_isExternalBusBlocked{}
	, m_isVramBusBlocked{}
	, m_dmaTransferCurrentAddress{}
	, m_dmaTransferInProcess{}
	, m_dmaTransferEnableNextCycle{}
{
	constexpr int KB_64{0x10000};
	m_memory.resize(0x10000);
	reset();
	loadRom("");
}

void Bus::reset()
{
	std::ranges::fill(m_memory, 0);
	m_rom = Rom{};
	m_isExternalBusBlocked = 0;
	m_isVramBusBlocked = 0;
	m_dmaTransferCurrentAddress = 0;
	m_dmaTransferInProcess = false;
	m_dmaTransferEnableNextCycle = false;
	m_memory[hardwareReg::IF] = 0xE1;
	m_memory[hardwareReg::IE] = 0xE0;
	m_memory[hardwareReg::DMA] = 0xFF;
}

void Bus::cycle()
{
	if((m_dmaTransferCurrentAddress & 0xFF) == 0xA0) 
	{
		m_isExternalBusBlocked = false;
		m_isVramBusBlocked = false;
		m_dmaTransferInProcess = false;
		m_dmaTransferCurrentAddress = 0;
	}

	using namespace MemoryRegions;
	if(m_dmaTransferInProcess)
	{
		if(m_dmaTransferCurrentAddress >= VRAM.first && m_dmaTransferCurrentAddress <= VRAM.second) 
		{ 
			m_isVramBusBlocked = true;
		}
		else if(isInExternalBus(m_dmaTransferCurrentAddress))
		{
			m_isExternalBusBlocked = true;
		}

		const uint16 destinationAddr{static_cast<uint16>(0xFE00 | m_dmaTransferCurrentAddress & 0xFF)};
		m_memory[destinationAddr] = m_memory[m_dmaTransferCurrentAddress++]; //access array directly because this has the priority over everything else
	}
	if(m_dmaTransferEnableNextCycle)
	{
		m_dmaTransferCurrentAddress = ((m_memory[hardwareReg::DMA] & 0xDF) << 8);
		m_dmaTransferInProcess = true;
		m_dmaTransferEnableNextCycle = false;
	}
}

void Bus::loadRom(std::string_view file)
{
	if(!loadMemory(file)) return;

	constexpr uint16 MBC_HEADER_ADDRESS{0x147};
	uint8 mbcValue{m_memory[MBC_HEADER_ADDRESS]};
	switch(mbcValue)
	{
	case 0x00:
		m_rom.mbc = NONE;
		break;
	case 0x08:
		m_rom.mbc = NONE;
		m_rom.hasRam = true;
		break;
	case 0x09:
		m_rom.mbc = NONE;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
		break;
	case 0x01:
		m_rom.mbc = MBC1;
		break;
	case 0x02:
		m_rom.mbc = MBC1;
		m_rom.hasRam = true;
		break;
	case 0x03:
		m_rom.mbc = MBC1;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
		break;
	case 0x05:
		m_rom.mbc = MBC2;
		break;
	case 0x06:
		m_rom.mbc = MBC2;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
		break;
	case 0x0f:
		m_rom.mbc = MBC3;
		m_rom.hasClock = true;
	case 0x10:
		m_rom.mbc = MBC3;
		m_rom.hasClock = true;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
	case 0x11:
		m_rom.mbc = MBC3;
	case 0x12:
		m_rom.mbc = MBC3;
		m_rom.hasRam = true;
	case 0x13:
		m_rom.mbc = MBC3;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
	case 0x19:
		m_rom.mbc = MBC5;
	case 0x1A:
		m_rom.mbc = MBC5;
		m_rom.hasRam = true;
	case 0x1B:
		m_rom.mbc = MBC5;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
	case 0x1C:
		m_rom.mbc = MBC5;
	case 0x1D:
		m_rom.mbc = MBC5;
		m_rom.hasRam = true;
	case 0x1E:
		m_rom.mbc = MBC5;
		m_rom.hasRam = true;
		m_rom.hasBattery = true;
	}

	setRomSize();
	setRamSize();
	std::cout << m_rom.mbc << ' ' << m_rom.romBanks << ' ' << m_rom.ramBanks << '\n';
	m_rom.isValid = true;
}

bool Bus::hasRom() const
{
	return m_rom.isValid;
}

uint8 Bus::read(const uint16 addr, const Component component) const
{
	using namespace MemoryRegions;
	using namespace hardwareReg;
	
	switch(addr)
	{
	case P1: return m_gameboy.m_input.read();
	case DIV: return m_gameboy.m_timers.read(Timers::DIV);
	case TIMA: return m_gameboy.m_timers.read(Timers::TIMA);
	case TMA: return m_gameboy.m_timers.read(Timers::TMA);
	case TAC: return m_gameboy.m_timers.read(Timers::TAC);
	case IF: return m_memory[IF];
	case LCDC: return m_gameboy.m_ppu.read(PPU::LCDC);
	case STAT: return m_gameboy.m_ppu.read(PPU::STAT);
	case SCY: return m_gameboy.m_ppu.read(PPU::SCY);
	case SCX: return m_gameboy.m_ppu.read(PPU::SCX);
	case LY: return m_gameboy.m_ppu.read(PPU::LY);
	case LYC: return m_gameboy.m_ppu.read(PPU::LYC);
	case DMA: return m_memory[DMA];
	case BGP: return m_gameboy.m_ppu.read(PPU::BGP);
	case OBP0: return m_gameboy.m_ppu.read(PPU::OBP0);
	case OBP1: return m_gameboy.m_ppu.read(PPU::OBP1);
	case WY: return m_gameboy.m_ppu.read(PPU::WY);
	case WX: return m_gameboy.m_ppu.read(PPU::WX);
	case IE: return m_memory[IE];
	default: 
	{
		if(m_dmaTransferInProcess)
		{
			const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
			const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
			if(addrInOam || (m_isVramBusBlocked && addrInVram) || (m_isExternalBusBlocked && isInExternalBus(addr))) return 0xFF;
		}

		if(component == Component::CPU)
		{
			const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
			const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
			const PPU::Mode ppuMode{m_gameboy.m_ppu.getCurrentMode()};
			if((ppuMode == PPU::OAM_SCAN && addrInOam) || (ppuMode == PPU::DRAWING && (addrInOam || addrInVram))) return 0xFF;
		}

		if(!m_isExternalRamEnabled && addr >= EXTERNAL_RAM.first && addr <= EXTERNAL_RAM.second) return 0xFF;
		
		return m_memory[addr];
	}
	}
}

void Bus::write(const uint16 addr, const uint8 value, const Component component)
{
	using namespace MemoryRegions;
	using namespace hardwareReg;

	switch(addr)
	{
	case P1: m_gameboy.m_input.write(value); break;
	case DIV: m_gameboy.m_timers.write(Timers::DIV, value); break;
	case TIMA: m_gameboy.m_timers.write(Timers::TIMA, value); break;
	case TMA: m_gameboy.m_timers.write(Timers::TMA, value); break;
	case TAC: m_gameboy.m_timers.write(Timers::TAC, value); break;
	case IF: m_memory[IF] = value | 0b1110'0000; break;
	case LCDC: m_gameboy.m_ppu.write(PPU::LCDC, value); break;
	case STAT: m_gameboy.m_ppu.write(PPU::STAT, value); break;
	case SCY: m_gameboy.m_ppu.write(PPU::SCY, value); break;
	case SCX: m_gameboy.m_ppu.write(PPU::SCX, value); break;
	case LY: m_gameboy.m_ppu.write(PPU::LY, value); break;
	case LYC: m_gameboy.m_ppu.write(PPU::LYC, value); break;
	case DMA:
		m_memory[DMA] = value;
		m_dmaTransferEnableNextCycle = true;
		break;
	case BGP: m_gameboy.m_ppu.write(PPU::BGP, value); break;
	case OBP0: m_gameboy.m_ppu.write(PPU::OBP0, value); break;
	case OBP1: m_gameboy.m_ppu.write(PPU::OBP1, value); break;
	case WY: m_gameboy.m_ppu.write(PPU::WY, value); break;
	case WX: m_gameboy.m_ppu.write(PPU::WX, value); break;
	case IE: m_memory[IE] = value | 0b1110'0000; break;
	default: 
	{
		if(m_dmaTransferInProcess)
		{
			const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
			const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
			if(addrInOam || (m_isVramBusBlocked && addrInVram) || (m_isExternalBusBlocked && isInExternalBus(addr))) return;
		}

		if(component == Component::CPU && m_gameboy.m_ppu.isEnabled())
		{
			const PPU::Mode ppuMode{m_gameboy.m_ppu.getCurrentMode()};
			const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
			const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
			if((ppuMode == PPU::OAM_SCAN && addrInOam) || (ppuMode == PPU::DRAWING && (addrInOam || addrInVram))) return;
		}

		if(addr >= ROM_BANK_0.first && addr <= ROM_BANK_1.second)
		{
			switch(m_rom.mbc)
			{
			case NONE: break;
			case MBC1:
			{
				constexpr int ENABLE_RAM_END{0x1FFF};
				constexpr int ROM_BANK_END{0x3FFF};
				constexpr int RAM_BANK_END{0x5FFF};
				constexpr int MODE_SELECT_END{0x7FFF};
				if(addr <= ENABLE_RAM_END)
				{
					if((value & 0xF) == 0xA) m_isExternalRamEnabled = true;
					else m_isExternalRamEnabled = false;
				}
				else if(addr <= ROM_BANK_END)
				{
					if(value == 0) m_rom.romBankIndex = 1;
					else
					{
						const uint8 mask{static_cast<uint8>(std::min(((m_rom.romBanks * 0x4000) / 0x8000), 0b1'0000))};
						m_rom.romBankIndex = value & (mask | (mask - 1)); //mask | mask - 1 is to set every less significant bit
					}
				}
				else if(addr <= RAM_BANK_END)
				{
					m_rom.ramBankIndex = value & 0b11;
				}
				else m_rom.modeFlag = value & 1;//so mode select
			}
			break;
			case MBC2:
			{

			}
			break;
			}
			return;
		}

		if(addr >= EXTERNAL_RAM.first && addr <= EXTERNAL_RAM.second)
		{
			if(!m_isExternalRamEnabled) return;

			switch(m_rom.mbc)
			{
			case NONE: break;
			case MBC1:
			{
				if(m_rom.ramBanks == 1)
				{
					m_memory[(addr - 0xA000) % m_rom.ramSize] = value;
				}
				else //always 4 on mbc1(assuming its a good rom)
				{

				}
			}
			break;
			}
			return;
		}

		m_memory[addr] = value;
		break;
	}
	}
}

template<>
void Bus::fillSprite(uint16 addr, PPU::Sprite& sprite) const
{
	if(m_dmaTransferInProcess) return;
	sprite.yPosition = m_memory[addr];
	sprite.xPosition = m_memory[addr + 1];
	sprite.tileNumber = m_memory[addr + 2];
	sprite.flags = m_memory[addr + 3];
}

bool Bus::loadMemory(std::string_view file)
{
	std::ifstream rom(file.data(), std::ios::binary | std::ios::ate);

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

	constexpr int KB_64{0x10000};
	if(size > KB_64) m_memory.resize(size );
	for(int i{0x0}; i < size; ++i)
	{
		m_memory[i] = buffer[i];
		//after dumping the second rom bank, skip the rest of the addressable memory
		if(i == (MemoryRegions::ROM_BANK_1.second + 1)) i = 0x10000;
	}
	return true;
}

void Bus::setRomSize()
{
	int romBanks{2};
	if(m_rom.mbc != NONE)
	{
		constexpr uint16 ROM_SIZE_ADDRESS{0x148};
		uint8 romSize{m_memory[ROM_SIZE_ADDRESS]};
		if(romSize > 0 && romSize < 0x9) romBanks <<= romSize;

		//weird edge cases
		if(romSize == 0x52) romBanks = 72;
		else if(romSize == 0x53) romBanks = 80;
		else if(romSize == 0x54) romBanks = 96;
	}
	m_rom.romBanks = romBanks;
	constexpr uint16 KB_16{0x4000};
	m_rom.romSize = romBanks * KB_16;
}

void Bus::setRamSize()
{
	constexpr uint16 KB_2{0x800};
	if(m_rom.hasRam)
	{
		constexpr uint16 RAM_SIZE_ADDRESS{0x149};
		uint8 ramSize{m_memory[RAM_SIZE_ADDRESS]};
		std::cout << int(ramSize) << '\n';
		switch(ramSize)
		{
		case 0x1: m_rom.ramBanks = 1; m_rom.ramSize = KB_2; break; //special case
		case 0x2: m_rom.ramBanks = 1; break;
		case 0x3: m_rom.ramBanks = 4; break;
		case 0x4: m_rom.ramBanks = 16; break;
		case 0x5: m_rom.ramBanks = 8; break;
		default: m_rom.ramBanks = 0; break;
		}
	}
	constexpr uint16 KB_8{0x2000};
	if(m_rom.ramSize != KB_2) m_rom.ramSize = m_rom.ramBanks * KB_8;
}

bool Bus::isInExternalBus(const uint16 addr) const
{
	constexpr uint16 EXTERNAL_BUS_FIRST_START{0};
	constexpr uint16 EXTERNAL_BUS_FIRST_END{0x7FFF};
	constexpr uint16 EXTERNAL_BUS_SECOND_START{0xA000};
	constexpr uint16 EXTERNAL_BUS_SECOND_END{0xFDFF};
	return (addr >= EXTERNAL_BUS_FIRST_START && addr <= EXTERNAL_BUS_FIRST_END)
		|| (addr >= EXTERNAL_BUS_SECOND_START && addr <= EXTERNAL_BUS_SECOND_END);
}