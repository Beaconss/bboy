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
	loadBootRom();
	loadRom("test/11-op a,(hl).gb");
}

void Gameboy::loadRom(char const* filePath)
{
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);

	if(file.is_open())
	{
		std::streamsize size{file.tellg()};
		if(size > 0xFFFF)
		{
			std::cerr << "File too large\n";
			return;
		}

		uint8* buffer{new uint8[size]};

		file.seekg(0, std::ios::beg);
		file.read((char*)buffer, size);
		file.close();

		for(int i{0x100}; i < size; ++i)
		{
			m_memory[i] = buffer[i];
		}

		delete[] buffer;
	}
	else std::cerr << "Failed to open file";
}

void Gameboy::loadBootRom()
{
	std::ifstream file("custom_boot.bin", std::ios::binary | std::ios::ate);

	if(file.is_open())
	{
		std::streamsize size{file.tellg()};

		uint8* buffer{new uint8[size]};

		file.seekg(0, std::ios::beg);
		file.read((char*)buffer, size);
		file.close();

		for(int i{0x0}; i < 0x100; ++i)
		{
			m_memory[i] = buffer[i];
		}

		delete[] buffer;
	}
	else std::cerr << "Failed to open file";
}

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

	m_cpu.cycle();
	for(int i{0}; i < 4; ++i) m_ppu.cycle(); //each cycle is treated as a t-cycle, so 4 per m-cycle

}

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

	constexpr uint16 VRAM_START{0x8000};
	constexpr uint16 VRAM_END{0x9FFF};

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
		return;
	}

	constexpr uint16 VRAM_START{0x8000};
	constexpr uint16 VRAM_END{0x9FFF};

	if(addr >= VRAM_START && addr <= VRAM_END)
	{
	//	std::cout << std::hex << "addr: " << (int)addr << ' ' << "value: " << (int)value << '\n';
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
	case IF:
	case IE: 
		m_cpu.interruptRequestedOrEnabled();
		[[fallthrough]];
	default: m_memory[addr] = value; break;
	}
}