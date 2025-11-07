#include "core/bus.h"
#include "core/gameboy.h"
#include "hardware_registers.h"

static std::vector<std::filesystem::path> fillCartridges()
{
	namespace fs = std::filesystem;
	std::vector<fs::path> cartridges{};
	try
	{
		for(const auto& entry : fs::recursive_directory_iterator(fs::current_path() / "..\\..\\roms")) 
		{
			if(entry.path().extension() == ".gb") cartridges.emplace_back(entry);
		}
	}
	catch(const fs::filesystem_error& ex)
	{
		std::cout << ex.what() << '\n'
			<< ex.path1() << '\n';
	}
	return cartridges;
}

std::vector<std::filesystem::path> cartridges{fillCartridges()};

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
	reset();
	m_cartridgeSlot.loadCartridge("..\\..\\roms\\Super Mario Land 2 - 6 Golden Coins (USA, Europe) (Rev 2).gb");
	//m_cartridgeSlot.loadCartridge("../../test/acceptance/ppu/stat_lyc_onoff.gb");
}

void Bus::reset()
{
	constexpr unsigned int kb64{0x10000};
	m_memory.resize(kb64); //32 kbs are not used but its ok to have less overhead
	std::fill(m_memory.begin(), m_memory.end(), 0);
	m_cartridgeSlot.reset();
	m_externalBusBlocked = false;
	m_vramBusBlocked = false;
	m_dmaTransferCurrentAddress = 0;
	m_dmaTransferInProcess = false;
	m_dmaTransferEnableDelay = 0;
	m_memory[hardwareReg::IF] = 0xE1;
	m_memory[hardwareReg::IE] = 0xE0;
	m_memory[hardwareReg::DMA] = 0xFF;
}

void Bus::handleDmaTransfer()
{
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
		write(destinationAddr, read(m_dmaTransferCurrentAddress++, Component::bus), Component::bus);
		
		using namespace MemoryRegions;
		m_vramBusBlocked = m_dmaTransferCurrentAddress >= vram.first && m_dmaTransferCurrentAddress <= vram.second;
		m_externalBusBlocked = isInExternalBus(m_dmaTransferCurrentAddress);
	}
}

CartridgeSlot& Bus::getCartridgeSlot()
{
	return m_cartridgeSlot;
}

void Bus::nextCartridge()
{
	static int next{};
	m_cartridgeSlot.loadCartridge(cartridges[next++]);
}

uint8 Bus::read(const uint16 addr, const Component component) const
{
	using namespace MemoryRegions;
	using namespace hardwareReg;
	
	switch(addr)
	{
	case P1: return m_gameboy.m_input.read();
	case DIV: return m_gameboy.m_timers.getDiv();
 	case TIMA: return m_gameboy.m_timers.getTima(); 
	case TMA: return m_gameboy.m_timers.getTma();
	case TAC: return m_gameboy.m_timers.getTac();
	case IF: return m_memory[IF];
	case CH1_SW: return m_gameboy.m_apu.read(APU::ch1Sw);
	case CH1_TIM_DUTY: return m_gameboy.m_apu.read(APU::ch1TimDuty);
	case CH1_VOL_ENV: return m_gameboy.m_apu.read(APU::ch1VolEnv);
	case CH1_PE_LOW: return m_gameboy.m_apu.read(APU::ch1PeLow);
	case CH1_PE_HI_CTRL: return m_gameboy.m_apu.read(APU::ch1PeHighCtrl);
	case CH2_TIM_DUTY: return m_gameboy.m_apu.read(APU::ch2TimDuty);
	case CH2_VOL_ENV: return m_gameboy.m_apu.read(APU::ch2VolEnv);
	case CH2_PE_LOW: return m_gameboy.m_apu.read(APU::ch2PeLow);
	case CH2_PE_HI_CTRL: return m_gameboy.m_apu.read(APU::ch2PeHighCtrl);
	case CH3_DAC_EN: return m_gameboy.m_apu.read(APU::ch3DacEn);
	case CH3_TIM: return m_gameboy.m_apu.read(APU::ch3Tim);
	case CH3_VOL: return m_gameboy.m_apu.read(APU::ch3Vol);
	case CH3_PE_LOW: return m_gameboy.m_apu.read(APU::ch3PeLow);
	case CH3_PE_HI_CTRL: return m_gameboy.m_apu.read(APU::ch3PeHighCtrl);
	case CH4_TIM: return m_gameboy.m_apu.read(APU::ch4Tim);
	case CH4_VOL_ENV: return m_gameboy.m_apu.read(APU::ch4VolEnv);
	case CH4_FRE_RAND: return m_gameboy.m_apu.read(APU::ch4FreRand);
	case CH4_CTRL: return m_gameboy.m_apu.read(APU::ch4Ctrl);
	case AU_VOL: return m_gameboy.m_apu.read(APU::audioVolume);
	case AU_PAN: return m_gameboy.m_apu.read(APU::audioPanning);
	case AU_CTRL: return m_gameboy.m_apu.read(APU::audioCtrl);
	case WAVE_RAM[0]:case WAVE_RAM[1]:case WAVE_RAM[2]:case WAVE_RAM[3]:
	case WAVE_RAM[4]:case WAVE_RAM[5]:case WAVE_RAM[6]:case WAVE_RAM[7]:
	case WAVE_RAM[8]:case WAVE_RAM[9]:case WAVE_RAM[10]:case WAVE_RAM[11]:
	case WAVE_RAM[12]:case WAVE_RAM[13]:case WAVE_RAM[14]:case WAVE_RAM[15]:
		return m_gameboy.m_apu.read(APU::waveRam, addr & 0xF);
	case LCDC: return m_gameboy.m_ppu->read(PPU::lcdc);
	case STAT: return m_gameboy.m_ppu->read(PPU::stat);
	case SCY: return m_gameboy.m_ppu->read(PPU::scy);
	case SCX: return m_gameboy.m_ppu->read(PPU::scx);
	case LY: return m_gameboy.m_ppu->read(PPU::ly);
	case LYC: return m_gameboy.m_ppu->read(PPU::lyc);
	case DMA: return m_memory[DMA];
	case BGP: return m_gameboy.m_ppu->read(PPU::bgp);
	case OBP0: return m_gameboy.m_ppu->read(PPU::obp0);
	case OBP1: return m_gameboy.m_ppu->read(PPU::obp1);
	case WY: return m_gameboy.m_ppu->read(PPU::wy);
	case WX: return m_gameboy.m_ppu->read(PPU::wx);
	case IE: return m_memory[IE];
	default:
	{
		const bool addrInOam{addr >= oam.first && addr <= oam.second};
		const bool addrInVram{addr >= vram.first && addr <= vram.second};
		if(m_dmaTransferInProcess && component != Component::bus && (addrInOam || (m_vramBusBlocked && addrInVram) || (m_externalBusBlocked && isInExternalBus(addr))))	return 0xFF;

		if(addr <= romBank1.second) return m_cartridgeSlot.readRom(addr);
		else if(addr >= externalRam.first && addr <= externalRam.second) return m_cartridgeSlot.readRam(addr);
		else if(addr >= echoRam.first && addr <= echoRam.second) return m_memory[addr - echoRamOffset];
		
		const PPU::Mode ppuMode{m_gameboy.m_ppu->getMode()};
		if(component == Component::cpu && ((addrInOam && (ppuMode == PPU::oamScan || ppuMode == PPU::drawing)) || (addrInVram && ppuMode == PPU::drawing))) return 0xFF;

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
	case DIV: m_gameboy.m_timers.setDiv(); break;
	case TIMA: m_gameboy.m_timers.setTima(value); break;
	case TMA: m_gameboy.m_timers.setTma(value); break;
	case TAC: m_gameboy.m_timers.setTac(value); break;
	case IF: m_memory[IF] = value | 0b1110'0000; break;
	case CH1_SW: m_gameboy.m_apu.write(APU::ch1Sw, value); break;
	case CH1_TIM_DUTY: m_gameboy.m_apu.write(APU::ch1TimDuty, value); break;
	case CH1_VOL_ENV: m_gameboy.m_apu.write(APU::ch1VolEnv, value);	break;
	case CH1_PE_LOW: m_gameboy.m_apu.write(APU::ch1PeLow, value); break;
	case CH1_PE_HI_CTRL: m_gameboy.m_apu.write(APU::ch1PeHighCtrl, value); break;
	case CH2_TIM_DUTY: m_gameboy.m_apu.write(APU::ch2TimDuty, value); break;
	case CH2_VOL_ENV: m_gameboy.m_apu.write(APU::ch2VolEnv, value); break;
	case CH2_PE_LOW: m_gameboy.m_apu.write(APU::ch2PeLow, value); break;
	case CH2_PE_HI_CTRL: m_gameboy.m_apu.write(APU::ch2PeHighCtrl, value); break;
	case CH3_DAC_EN: m_gameboy.m_apu.write(APU::ch3DacEn, value); break;
	case CH3_TIM: m_gameboy.m_apu.write(APU::ch3Tim, value); break;
	case CH3_VOL: m_gameboy.m_apu.write(APU::ch3Vol, value); break;
	case CH3_PE_LOW: m_gameboy.m_apu.write(APU::ch3PeLow, value); break;
	case CH3_PE_HI_CTRL: m_gameboy.m_apu.write(APU::ch3PeHighCtrl, value); break;
	case CH4_TIM: m_gameboy.m_apu.write(APU::ch4Tim, value); break;
	case CH4_VOL_ENV: m_gameboy.m_apu.write(APU::ch4VolEnv, value); break;
	case CH4_FRE_RAND: m_gameboy.m_apu.write(APU::ch4FreRand, value); break;
	case CH4_CTRL: m_gameboy.m_apu.write(APU::ch4Ctrl, value); break;
	case AU_VOL: m_gameboy.m_apu.write(APU::audioVolume, value); break;
	case AU_PAN: m_gameboy.m_apu.write(APU::audioPanning, value); break;
	case AU_CTRL: m_gameboy.m_apu.write(APU::audioCtrl, value); break;
	case WAVE_RAM[0]:case WAVE_RAM[1]:case WAVE_RAM[2]:case WAVE_RAM[3]: break;
	case WAVE_RAM[4]:case WAVE_RAM[5]:case WAVE_RAM[6]:case WAVE_RAM[7]: break;
	case WAVE_RAM[8]:case WAVE_RAM[9]:case WAVE_RAM[10]:case WAVE_RAM[11]: break;
	case WAVE_RAM[12]:case WAVE_RAM[13]:case WAVE_RAM[14]:case WAVE_RAM[15]: break;
		 m_gameboy.m_apu.write(APU::waveRam, value, addr & 0xF);
	case LCDC: m_gameboy.m_ppu->write(PPU::lcdc, value); break;
	case STAT: m_gameboy.m_ppu->write(PPU::stat, value); break;
	case SCY: m_gameboy.m_ppu->write(PPU::scy, value); break;
	case SCX: m_gameboy.m_ppu->write(PPU::scx, value); break;
	case LY: m_gameboy.m_ppu->write(PPU::ly, value); break;
	case LYC: m_gameboy.m_ppu->write(PPU::lyc, value); break;
	case DMA:
	{
		m_memory[DMA] = value;
		constexpr int dmaTransferEnableDelay = 2;
		m_dmaTransferEnableDelay = dmaTransferEnableDelay;
	}
	break;
	case BGP: m_gameboy.m_ppu->write(PPU::bgp, value); break;
	case OBP0: m_gameboy.m_ppu->write(PPU::obp0, value); break;
	case OBP1: m_gameboy.m_ppu->write(PPU::obp1, value); break;
	case WY: m_gameboy.m_ppu->write(PPU::wy, value); break;
	case WX: m_gameboy.m_ppu->write(PPU::wx, value); break;
	case IE: m_memory[IE] = value | 0b1110'0000; break;
	default: 
	{
		const bool addrInOam{addr >= oam.first && addr <= oam.second};
		const bool addrInVram{addr >= vram.first && addr <= vram.second};
		if(m_dmaTransferInProcess && component != Component::bus && (addrInOam || (m_vramBusBlocked && addrInVram) || (m_externalBusBlocked && isInExternalBus(addr)))) return;

		if(addr <= romBank1.second)
		{
			m_cartridgeSlot.writeRom(addr, value);
			return;
		}
		else if(addr >= externalRam.first && addr <= externalRam.second)
		{
			m_cartridgeSlot.writeRam(addr, value);
			return;
		}
		else if(addr >= echoRam.first && addr <= echoRam.second)
		{
			m_memory[addr - echoRamOffset] = value;
			return;
		}

		const PPU::Mode ppuMode{m_gameboy.m_ppu->getMode()};
		if(component == Component::cpu && ((addrInOam && (ppuMode == PPU::oamScan || ppuMode == PPU::drawing)) || (addrInVram && ppuMode == PPU::drawing))) return;

		m_memory[addr] = value;
		break;
	}
	}
}

uint16 Bus::currentCycle() const
{
	return m_gameboy.m_currentCycle;
}

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
	constexpr uint16 externalBusFirstStart{0};
	constexpr uint16 externalBusFirstEnd{0x7FFF};
	constexpr uint16 externalBusSecondStart{0xA000};
	constexpr uint16 externalBusSecondEnd{0xFDFF};
	return (addr >= externalBusFirstStart && addr <= externalBusFirstEnd)
		|| (addr >= externalBusSecondStart && addr <= externalBusSecondEnd);
}