#include "gameboy.h"

Gameboy::Gameboy()
	: m_cpu{*this}
	, m_ppu{*this}
	, m_memory{}
	, m_timers{*this}
	, m_dmaTransferInProcess{false}
	, m_dmaTransferEnableNextCycle{false}
	, m_dmaTransferCurrentAddress{}
{
}

//in the main loop I will batch a number of these and render afterwards
void Gameboy::cycle() //1 machine cycle
{
	m_timers.cycle();

	if(m_dmaTransferInProcess)
	{
		uint16 destinationAddr{static_cast<uint16>((0xFE << 8) | m_dmaTransferCurrentAddress & 0xFF)};
		m_memory[destinationAddr] = m_memory[m_dmaTransferCurrentAddress++];
		if((m_dmaTransferCurrentAddress & 0xFF) == 0xA0) m_dmaTransferInProcess = false;
	}

	if(m_dmaTransferEnableNextCycle)
	{
		m_dmaTransferInProcess = true;
		m_dmaTransferEnableNextCycle = false;
	}

	m_ppu.cycle();
	m_cpu.cycle();
}

//memo to handle reads and writes to video related memory while the ppu is accessing it
uint8 Gameboy::readMemory(const uint16 addr) const
{
	using namespace hardwareReg;

	if(m_dmaTransferInProcess) //while dma is in process only hram can be accessed
	{
		constexpr uint16 HRAM_START{0xFF80};
		constexpr uint16 HRAM_END{0xFFFE};
		if(addr >= HRAM_START && addr <= HRAM_END) return m_memory[addr];
		else return 0xFF;
	}

	switch(addr)
	{ //handle virtual registers
	case DIV: return m_timers.read(Timers::DIV); 
	case TIMA: return m_timers.read(Timers::TIMA);
	case TMA: return m_timers.read(Timers::TMA);
	case TAC: return m_timers.read(Timers::TAC);
	case LCDC: return m_ppu.read(PPU::LCDC);
	case STAT: return m_ppu.read(PPU::STAT);
	case SCY: return m_ppu.read(PPU::SCY);
	case SCX: return m_ppu.read(PPU::SCX);
	case LY: return m_ppu.read(PPU::LY);
	case LYC: return m_ppu.read(PPU::LYC);
	case BGP: return m_ppu.read(PPU::BGP);
	case OBP0: return m_ppu.read(PPU::OBP0);
	case OBP1: return m_ppu.read(PPU::OBP1);
	case WY: return m_ppu.read(PPU::WY);
	case WX: return m_ppu.read(PPU::WX);
	default: return m_memory[addr];
	}
}

void Gameboy::writeMemory(const uint16 addr, const uint8 value)
{
	using namespace hardwareReg;

	if(m_dmaTransferInProcess) //while dma is in process only hram can be accessed
	{
		constexpr uint16 HRAM_START{0xFF80};
		constexpr uint16 HRAM_END{0xFFFE};
		if(addr >= HRAM_START && addr <= HRAM_END) m_memory[addr] = value;
		else return;
	}

	switch(addr)
	{
	case DIV: m_timers.write(Timers::DIV, value); break;
	case TIMA: m_timers.write(Timers::TIMA, value); break;
	case TMA: m_timers.write(Timers::TMA, value); break;
	case TAC: m_timers.write(Timers::TAC, value); break;
	case LCDC: m_ppu.write(PPU::LCDC, value); break;
	case STAT: m_ppu.write(PPU::STAT, value); break;
	case SCY: m_ppu.write(PPU::SCY, value); break;
	case SCX: m_ppu.write(PPU::SCX, value); break;
	case LY: m_ppu.write(PPU::LY, value); break;
	case LYC: m_ppu.write(PPU::LYC, value); break;
	case DMA: 
		m_dmaTransferCurrentAddress = (value << 8); 
		m_dmaTransferEnableNextCycle = true; 
		break;
	case BGP: m_ppu.write(PPU::BGP, value); break;
	case OBP0: m_ppu.write(PPU::OBP0, value); break;
	case OBP1: m_ppu.write(PPU::OBP1, value); break;
	case WY: m_ppu.write(PPU::WY, value); break;
	case WX: m_ppu.write(PPU::WX, value); break;
	default: m_memory[addr] = value; break;
	}
}