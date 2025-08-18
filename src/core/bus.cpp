#include "bus.h"
#include "gameboy.h"

Bus::Bus(Gameboy& gb)
	: m_gameboy{gb}
	, m_memory{}
	, m_externalBusBlocked{}
	, m_vramBusBlocked{}
	, m_dmaTransferCurrentAddress{}
	, m_dmaTransferInProcess{}
	, m_dmaTransferEnableNextCycle{}
	, m_hasRom{}
{
	reset();
	loadRom("Legend of Zelda, The - Link's Awakening (USA, Europe) (Rev 2).gb");
}

void Bus::reset()
{
	std::ranges::fill(m_memory, 0);
	m_externalBusBlocked = 0;
	m_vramBusBlocked = 0;
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
		m_externalBusBlocked = false;
		m_vramBusBlocked = false;
		m_dmaTransferInProcess = false;
		m_dmaTransferCurrentAddress = 0;
	}

	using namespace MemoryRegions;
	if(m_dmaTransferInProcess)
	{
		if(m_dmaTransferCurrentAddress >= VRAM.first && m_dmaTransferCurrentAddress <= VRAM.second) 
		{
			m_vramBusBlocked = true;
		}
		else if(isInExternalBus(m_dmaTransferCurrentAddress))
		{
			m_externalBusBlocked = true;
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

void Bus::loadRom(std::string_view filePath)
{
	std::ifstream rom(filePath.data(), std::ios::binary | std::ios::ate);

	if(rom.fail())
	{
		std::cerr << "Failed to open file\n";
		return;
	}

	std::streamsize size{rom.tellg()};
	if(size > 0x10000)
	{
		std::cerr << "File too large\n";
		return;
	}

	std::unique_ptr<char[]> buffer{new char[size]};

	rom.seekg(0);
	rom.read(buffer.get(), size);
	rom.close();

	for(int i{0x0}; i < size; ++i)
	{
		m_memory[i] = buffer[i];
	}
	m_hasRom = true;
}

bool Bus::hasRom() const
{
	return m_hasRom;
}

uint8 Bus::read(const uint16 addr, const Component component) const
{
	using namespace MemoryRegions;
	using namespace hardwareReg;

	if(component == Component::CPU)
	{
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		const PPU::Mode ppuMode{m_gameboy.m_ppu.getCurrentMode()};
		if(ppuMode == PPU::OAM_SCAN && addrInOam) return 0xFF;
		else if(ppuMode == PPU::DRAWING && (addrInOam || addrInVram)) return 0xFF;
	}

	if(m_dmaTransferInProcess)
	{
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		if(addrInOam) return 0xFF;
		else if(m_vramBusBlocked && addrInVram) return 0xFF;
		else if(m_externalBusBlocked && isInExternalBus(addr)) return 0xFF;
	}

	switch(addr)
	{
	case P1: return m_gameboy.m_input.read();
	case DIV: return m_gameboy.m_timers.read(Timers::DIV);
	case TIMA: return m_gameboy.m_timers.read(Timers::TIMA);
	case TMA: return m_gameboy.m_timers.read(Timers::TMA);
	case TAC: return m_gameboy.m_timers.read(Timers::TAC);
	case LCDC: return m_gameboy.m_ppu.read(PPU::LCDC);
	case STAT: return m_gameboy.m_ppu.read(PPU::STAT);
	case SCY: return m_gameboy.m_ppu.read(PPU::SCY);
	case SCX: return m_gameboy.m_ppu.read(PPU::SCX);
	case LY: return m_gameboy.m_ppu.read(PPU::LY);
	case LYC: return m_gameboy.m_ppu.read(PPU::LYC);
	case BGP: return m_gameboy.m_ppu.read(PPU::BGP);
	case OBP0: return m_gameboy.m_ppu.read(PPU::OBP0);
	case OBP1: return m_gameboy.m_ppu.read(PPU::OBP1);
	case WY: return m_gameboy.m_ppu.read(PPU::WY);
	case WX: return m_gameboy.m_ppu.read(PPU::WX);
	default: return m_memory[addr];
	}
}

void Bus::write(const uint16 addr, const uint8 value, const Component component)
{
	using namespace MemoryRegions;
	using namespace hardwareReg;

	if((addr >= ROM_BANK_0.first && addr <= ROM_BANK_1.second))
	{
		//mbc...
		return;
	}


	if(component == Component::CPU && m_gameboy.m_ppu.isEnabled())
	{
		const PPU::Mode ppuMode{m_gameboy.m_ppu.getCurrentMode()};
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		if(ppuMode == PPU::OAM_SCAN && addrInOam) return;
		else if(ppuMode == PPU::DRAWING && (addrInOam || addrInVram)) return;
	}

	if(m_dmaTransferInProcess)
	{
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		if(addrInOam) return;
		else if(m_vramBusBlocked && addrInVram) return;
		else if(m_externalBusBlocked && isInExternalBus(addr)) return;
	}

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
	case IE: m_memory[addr] = value | 0b1110'0000; break;
	default: m_memory[addr] = value; break;
	}
}


bool Bus::isInExternalBus(const uint16 addr) const
{
	return (addr >= EXTERNAL_BUS[0].first && addr <= EXTERNAL_BUS[0].second)
		   || (addr >= EXTERNAL_BUS[1].first && addr <= EXTERNAL_BUS[1].second);
}