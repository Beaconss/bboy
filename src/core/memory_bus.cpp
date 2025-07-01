#include "memory_bus.h"
#include "gameboy.h"

MemoryBus::MemoryBus(Gameboy& gb)
	: m_gameboy{gb}
	, m_memory{}
	, m_dmaTransferCurrentAddress{}
	, m_dmaTransferInProcess{}
	, m_dmaTransferEnableNextCycle{}
{
	//loadBootRom();
	loadRom("test/01-special.gb");
}

void MemoryBus::cycle()
{
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
}

void MemoryBus::loadBootRom()
{
	std::ifstream file("dmg_boot.bin", std::ios::binary | std::ios::ate);

	if(file.is_open())
	{
		std::streamsize size{file.tellg()};

		uint8* buffer{new uint8[size]};

		file.seekg(0, std::ios::beg);
		file.read((char*)buffer, size);
		file.close();

		for(int i{0x0}; i < 0x100; ++i)
		{
			write(i, buffer[i]);
		}

		delete[] buffer;
	}
	else std::cerr << "Failed to open file";
}

void MemoryBus::loadRom(char const* filePath)
{
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);

	if(file.is_open())
	{
		std::streamsize size{file.tellg()};
		if(size > 0xFFFF + 1)
		{
			std::cerr << "File too large\n";
			return;
		}

		uint8* buffer{new uint8[size]};

		file.seekg(0, std::ios::beg);
		file.read((char*)buffer, size);
		file.close();

		for(int i{0x0}; i < size; ++i)
		{
			write(i, buffer[i]);
		}

		delete[] buffer;
	}
	else std::cerr << "Failed to open file";
}

uint8 MemoryBus::read(const uint16 addr) const
{
	using namespace hardwareReg;

	//TODO: block cpu's access to vram during drawing

	if(m_dmaTransferInProcess)
	{
		return m_memory[m_dmaTransferCurrentAddress];
	}

	switch(addr)
	{
	//handle virtual registers
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

void MemoryBus::write(const uint16 addr, const uint8 value)
{
	using namespace hardwareReg;

	if(m_dmaTransferInProcess) return;

	switch(addr)
	{
	//handle virtual registers
	case DIV: m_gameboy.m_timers.write(Timers::DIV, value); break;
	case TIMA: m_gameboy.m_timers.write(Timers::TIMA, value); break;
	case TMA: m_gameboy.m_timers.write(Timers::TMA, value); break;
	case TAC: m_gameboy.m_timers.write(Timers::TAC, value); break;
	case LCDC: m_gameboy.m_ppu.write(PPU::LCDC, value); break;
	case STAT: m_gameboy.m_ppu.write(PPU::STAT, value); break;
	case SCY: m_gameboy.m_ppu.write(PPU::SCY, value); break;
	case SCX: m_gameboy.m_ppu.write(PPU::SCX, value); break;
	case LY: m_gameboy.m_ppu.write(PPU::LY, value); break;
	case LYC: m_gameboy.m_ppu.write(PPU::LYC, value); break;
	case DMA:
		m_dmaTransferCurrentAddress = (value << 8);
		m_dmaTransferEnableNextCycle = true;
		break;
	case BGP: m_gameboy.m_ppu.write(PPU::BGP, value); break;
	case OBP0: m_gameboy.m_ppu.write(PPU::OBP0, value); break;
	case OBP1: m_gameboy.m_ppu.write(PPU::OBP1, value); break;
	case WY: m_gameboy.m_ppu.write(PPU::WY, value); break;
	case WX: m_gameboy.m_ppu.write(PPU::WX, value); break;
	default: m_memory[addr] = value; break;
	}
}