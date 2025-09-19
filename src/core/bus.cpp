#include <core/bus.h>
#include <core/gameboy.h>

static std::vector<std::filesystem::path> fillTests()
{
	namespace fs = std::filesystem;
	std::vector<fs::path> tests{};
	try
	{
		for(const auto& entry : fs::recursive_directory_iterator(fs::current_path() / "test/acceptance/ppu"))
		{
			if(entry.path().extension() == ".gb") tests.emplace_back(entry);
		}
	}
	catch(const fs::filesystem_error& ex)
	{
		std::cout << ex.what() << '\n'
			<< ex.path1() << '\n';
	}
	return tests;
}

std::vector<std::filesystem::path> TESTS{fillTests()};

Bus::Bus(Gameboy& gb)
	: m_gameboy{gb}
	, m_memory{}
	, m_cartridgeSlot{}
	, m_externalBusBlocked{}
	, m_vramBusBlocked{}
	, m_dmaTransferCurrentAddress{}
	, m_dmaTransferInProcess{}
	, m_dmaTransferEnableDelay{}
{
	constexpr unsigned int KB_64{0x10000};
	m_memory.resize(KB_64); //32 kbs are not used but its ok to have less overhead
	reset();
	m_cartridgeSlot.loadCartridge("test/acceptance/ppu/lcdon_write_timing-GS.gb");
	//nextTest();
}

void Bus::reset()
{
	std::ranges::fill(m_memory, 0);
	m_cartridgeSlot.reset();
	m_externalBusBlocked = 0;
	m_vramBusBlocked = 0;
	m_dmaTransferCurrentAddress = 0;
	m_dmaTransferInProcess = false;
	m_dmaTransferEnableDelay = 0;
	m_memory[hardwareReg::IF] = 0xE1;
	m_memory[hardwareReg::IE] = 0xE0;
	m_memory[hardwareReg::DMA] = 0xFF;
}

void Bus::handleDmaTransfer()
{
	using namespace MemoryRegions;

	if(m_dmaTransferEnableDelay > 0)
	{
		if(--m_dmaTransferEnableDelay == 0)
		{
			m_dmaTransferInProcess = true;
			m_dmaTransferCurrentAddress = m_memory[hardwareReg::DMA] >= 0xFE ? (0xDE00 + ((m_memory[hardwareReg::DMA] - 0xFE) << 8)) : (m_memory[hardwareReg::DMA] << 8);
		}
	}

	if((m_dmaTransferCurrentAddress & 0xFF) == 0xA0)
	{
		m_externalBusBlocked = false;
		m_vramBusBlocked = false;
		m_dmaTransferInProcess = false;
		m_dmaTransferCurrentAddress = 0;
	}
	if(m_dmaTransferInProcess)
	{
		const uint16 destinationAddr{static_cast<uint16>(0xFE00 | m_dmaTransferCurrentAddress & 0xFF)};
		write(destinationAddr, read(m_dmaTransferCurrentAddress++, Component::BUS), Component::BUS);
	
		if(m_dmaTransferCurrentAddress >= VRAM.first && m_dmaTransferCurrentAddress <= VRAM.second)
		{
			m_vramBusBlocked = true;
		}
		else if(isInExternalBus(m_dmaTransferCurrentAddress))
		{
			m_externalBusBlocked = true;
		}
	}
}

void Bus::loadCartridge(const std::filesystem::path& filePath)
{
	m_cartridgeSlot.loadCartridge(filePath);
}

bool Bus::hasRom() const
{
	return m_cartridgeSlot.hasCartridge();
}

void Bus::nextTest()
{
	static int next{};
	m_cartridgeSlot.loadCartridge(TESTS[next++]);
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
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		if(m_dmaTransferInProcess && component != Component::BUS && (addrInOam || (m_vramBusBlocked && addrInVram) || (m_externalBusBlocked && isInExternalBus(addr))))	return 0xFF;

		if(addr <= ROM_BANK_1.second) return m_cartridgeSlot.readRom(addr);
		else if(addr >= EXTERNAL_RAM.first && addr <= EXTERNAL_RAM.second) return m_cartridgeSlot.readRam(addr);
		else if(constexpr int ECHO_RAM_OFFSET{ECHO_RAM.first - WORK_RAM_0.first}; addr >= ECHO_RAM.first && addr <= ECHO_RAM.second) return m_memory[addr - ECHO_RAM_OFFSET];
		
		if(m_gameboy.m_ppu.isEnabled())
		{
			const PPU::Mode ppuMode{m_gameboy.m_ppu.getMode()};
			if(component == Component::CPU && ((addrInOam && (ppuMode == PPU::OAM_SCAN || ppuMode == PPU::DRAWING)) || (addrInVram && ppuMode == PPU::DRAWING))) return 0xFF;
		}

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
	{
		m_memory[DMA] = value;
		constexpr int DMA_TRANSFER_ENABLE_DELAY = 2;
		m_dmaTransferEnableDelay = DMA_TRANSFER_ENABLE_DELAY;
	}
	break;
	case BGP: m_gameboy.m_ppu.write(PPU::BGP, value); break;
	case OBP0: m_gameboy.m_ppu.write(PPU::OBP0, value); break;
	case OBP1: m_gameboy.m_ppu.write(PPU::OBP1, value); break;
	case WY: m_gameboy.m_ppu.write(PPU::WY, value); break;
	case WX: m_gameboy.m_ppu.write(PPU::WX, value); break;
	case IE: m_memory[IE] = value | 0b1110'0000; break;
	default: 
	{
		const bool addrInOam{addr >= OAM.first && addr <= OAM.second};
		const bool addrInVram{addr >= VRAM.first && addr <= VRAM.second};
		if(m_dmaTransferInProcess && component != Component::BUS && (addrInOam || (m_vramBusBlocked && addrInVram) || (m_externalBusBlocked && isInExternalBus(addr)))) return;

		if(addr <= ROM_BANK_1.second)
		{
			m_cartridgeSlot.writeRom(addr, value);
			return;
		}
		else if(addr >= EXTERNAL_RAM.first && addr <= EXTERNAL_RAM.second)
		{
			m_cartridgeSlot.writeRam(addr, value);
			return;
		}
		else if(addr >= ECHO_RAM.first && addr <= ECHO_RAM.second)
		{
			constexpr int ECHO_RAM_OFFSET{ECHO_RAM.first - WORK_RAM_0.first};
			m_memory[addr - ECHO_RAM_OFFSET] = value;
			return;
		}

		if(m_gameboy.m_ppu.isEnabled())
		{
			const PPU::Mode ppuMode{m_gameboy.m_ppu.getMode()};
			if(component == Component::CPU && ((addrInOam && (ppuMode == PPU::OAM_SCAN || ppuMode == PPU::DRAWING)) || (addrInVram && ppuMode == PPU::DRAWING))) return;
		}

		m_memory[addr] = value;
		break;
	}
	}
}

template<>
void Bus::fillSprite(uint16 oamAddr, PPU::Sprite& sprite) const
{
	if(m_dmaTransferInProcess) return;
	sprite.yPosition = m_memory[oamAddr];
	sprite.xPosition = m_memory[oamAddr + 1];
	sprite.tileNumber = m_memory[oamAddr + 2];
	sprite.flags = m_memory[oamAddr + 3];
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