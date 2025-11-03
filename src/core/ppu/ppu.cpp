#include "core/ppu/ppu.h"
#include "hardware_registers.h"
#include "core/bus.h"
#include <ranges>
#include <algorithm>

PPU::PPU(Bus& bus)
	: m_bus{bus}
	, m_fetcher{*this}
	, m_statInterrupt{}
	, m_mode{}
	, m_cycleCounter{}
	, m_vblankInterruptNextCycle{}
	, m_reEnabling{}
	, m_reEnableDelay{}
	, m_lcdBuffer{}
	, m_xPosition{}
	, m_pixelsToDiscard{}
	, m_spriteBuffer{}
	, m_spriteAddress{}
	, m_pixelFifoBackground{}
	, m_pixelFifoSprite{}
	, m_lcdc{}
	, m_stat{}
	, m_scy{}
	, m_scx{}
	, m_ly{}
	, m_lyc{}
	, m_bgp{}
	, m_oldBgp{}
	, m_obp0{}
	, m_obp1{}
	, m_wy{}
	, m_wx{}
{
	m_spriteBuffer.reserve(10);
	reset();
}

void PPU::reset()
{
	m_fetcher.reset();
	m_statInterrupt = StatInterrupt{};
	m_mode = vBlank;
	m_cycleCounter = 0;
	m_vblankInterruptNextCycle = false;
	m_reEnabling = false;
	m_reEnableDelay = 0;
	std::ranges::fill(m_lcdBuffer, 0);
	m_xPosition = 0;
	m_pixelsToDiscard = 0;
	m_spriteBuffer.clear();
	m_spriteAddress = oamMemoryStart;
	clearFifos();
	m_lcdc = 0x91;
	m_stat = 0x85;
	m_scy = 0;
	m_scx = 0;
	m_ly = 0;
	m_lyc = 0;
	m_bgp = 0xFC;
	m_obp0 = 0;
	m_obp1 = 0;
	m_wy = 0;
	m_wx = 0;
}

//static std::ofstream //drawingLog{"drawing_log.txt"};
//static uint64_t tCycle{80};

void PPU::mCycle()
{
	if(!(m_lcdc & 0x80)) return;

	if(constexpr int lastVBlankScanline{153}; m_ly == lastVBlankScanline) m_ly = 0;
	updateCoincidenceFlag();
	m_stat = (m_stat & 0b11111100) | m_mode; //bits 1-0 of stat store the current mode
	handleStatInterrupt();
	if(m_vblankInterruptNextCycle)
	{
		requestVBlankInterrupt();
		m_statInterrupt.sources[oamScan] = false; //reset this in case it was briefly true
		m_vblankInterruptNextCycle = false;
	}

	if(m_reEnableDelay > 0)
	{
		if(--m_reEnableDelay == 0) updateMode(drawing);
		return;
	}
	else m_reEnabling = false;

	++m_cycleCounter;
	switch(m_mode)
	{
	case oamScan:
		oamScanCycle();
		break;
	case drawing:
		drawingCycle();
		break;
	case hBlank:
		hBlankCycle();
		break;
	case vBlank:
		vBlankCycle();
		break;
	}
}

PPU::Mode PPU::getMode() const
{
	if((m_mode == drawing || m_mode == oamScan) && !m_reEnabling) return m_mode;
	else return static_cast<Mode>(m_stat & 0b11);
}

const uint16* PPU::getLcdBuffer() const
{
	return m_lcdBuffer.data();
}

uint8 PPU::read(const Index index) const
{
	switch(index)
	{
	case lcdc: return m_lcdc;
	case stat: return m_stat;
	case scy: return m_scy;
	case scx: return m_scx;
	case ly: return m_ly;
	case lyc: return m_lyc;
	case bgp: return m_bgp;
	case obp0: return m_obp0;
	case obp1: return m_obp1;
	case wy: return m_wy;
	case wx: return m_wx;
	default: return 0xFF;
	}
}

void PPU::write(const Index index, const uint8 value)
{
	switch(index)
	{
	case lcdc: 
		if(constexpr int maxReEnableDelay{19}; !(m_lcdc & 0x80) && value & 0x80) 
		{
			m_reEnabling = true;
			m_reEnableDelay = maxReEnableDelay;
		}
		m_lcdc = value; 
		if(!(m_lcdc & 0x80))
		{
			updateMode(hBlank);
			m_stat = (m_stat & 0b1111'1100) | m_mode; //manually update stat mode bits because if the ppu is disabled they wont update
			m_ly = 0;
			m_cycleCounter = 20;
		}
		m_fetcher.updateTilemap();
		break;
	case stat: 
		m_stat = ((m_stat & 0b111) | (value & ~0b111)) | 0x80;
		setStatModeSources();
		break;
	case scy: m_scy = value; break;
	case scx: m_scx = value; break;
	case ly: break;
	case lyc: m_lyc = value; break;
	case bgp: 
		m_oldBgp = m_bgp;
		m_bgp = value;
		break;
	case obp0: m_obp0 = value; break;
	case obp1: m_obp1 = value; break;
	case wy: m_wy = value; break;
	case wx: m_wx = value; break;
	}
}

void PPU::handleStatInterrupt()
{
	const bool statResult{m_statInterrupt.sources[StatInterrupt::hBLank]
						 || m_statInterrupt.sources[StatInterrupt::vBlank]
						 || m_statInterrupt.sources[StatInterrupt::oamScan]
						 || m_statInterrupt.sources[StatInterrupt::lyCompare]};
	if(!m_statInterrupt.previousResult && statResult) requestStatInterrupt(); //if there was a rising edge
	m_statInterrupt.previousResult = statResult;
}

void PPU::setStatModeSources()
{
	for(int i{0}; i < 3; ++i)
	{
		if(m_stat & (1 << (3 + i)) && i == m_mode) m_statInterrupt.sources[i] = true;  //at bits 3-4-5 are stored the stat condition enable for mode 0, 1 and 2 respectively
		else m_statInterrupt.sources[i] = false;
	}
}

void PPU::updateMode(const Mode mode)
{
	m_mode = mode; 
	if(m_mode == drawing) 
	{
		m_pixelsToDiscard = m_scx & 7;
		//drawingLog << "pixels to discard this scanline: " << m_pixelsToDiscard << '\n';
		m_oldBgp = 0;
	}
	setStatModeSources();
}

void PPU::updateCoincidenceFlag(bool set)
{
	m_stat = (m_stat & ~0b100) | (set ? ((m_ly == m_lyc) << 2) : 0);
	
	if(m_stat & 0x40 && m_stat & 0b100) m_statInterrupt.sources[StatInterrupt::Source::lyCompare] = true;
	else m_statInterrupt.sources[StatInterrupt::lyCompare] = false;
}

void PPU::oamScanCycle()
{
	constexpr int spriteBufferMaxSize{10};

	for(int i{0}; i < 2; ++i)
	{
		if(m_spriteBuffer.size() < spriteBufferMaxSize)
		{
			Sprite sprite{};
			m_bus.fillSprite(m_spriteAddress, sprite);
			tryAddSpriteToBuffer(sprite);
			m_spriteAddress += 4; //go to next sprite
		}
	}

	constexpr int oamScanEndCycle{20};
	if(m_cycleCounter == oamScanEndCycle || m_cycleCounter == 19 && m_ly == 0)
	{
		//here if left xPosition is equal to the right's one, 
		//left goes up because the left one has higher priority since its first in oam memory
		std::ranges::sort(m_spriteBuffer, [](Sprite left, Sprite right)
			{
				return left.xPosition >= right.xPosition;
			});
		m_spriteAddress = oamMemoryStart;
		updateMode(drawing);
	}
}

void PPU::tryAddSpriteToBuffer(const Sprite sprite)
{
	constexpr uint8 tallSpriteMode{0b100};
	if(m_ly + 16 >= sprite.yPosition 
		&& m_ly + 16 < sprite.yPosition + (m_lcdc & tallSpriteMode ? 16 : 8))
	{
		m_spriteBuffer.push_back(sprite);
	}
}

void PPU::drawingCycle()
{
	for(int i{0}; i < 4; ++i)
	{
		//++tCycle;
		//drawingLog << "t-cycle: " << std::dec << tCycle << '\n';

		m_fetcher.cycle();
		tryToPushPixel();
		//drawingLog << "\n\n";
		m_oldBgp = 0;
		if(m_xPosition == screenWidth)
		{
			m_xPosition = 0;
			m_fetcher.resetEndScanline();
			clearFifos();
			m_spriteBuffer.clear();
			updateMode(hBlank);
			//tCycle = 80;
			//drawingLog << '\n';
			break;
		}
	}
}

void PPU::tryToPushPixel()
{
	//drawingLog << "pushing state:\n";
	if(!m_pixelFifoBackground.empty() && m_fetcher.m_spriteFetchDelay == 0)
	{
		if(m_pixelsToDiscard == 0)
		{
			Pixel pixel{};
			bool pushSpritePixel{shouldPushSpritePixel()};
			if(pushSpritePixel)
			{
				pixel = m_pixelFifoSprite.front();
				pixel.paletteValue = pixel.spritePalette ? m_obp1 : m_obp0;
			}
			else //background/window
			{
				pixel = m_pixelFifoBackground.front();
				constexpr uint8 backgroundEnable{0b1};
				pixel.paletteValue = m_lcdc & backgroundEnable ? m_bgp : 0;
				pixel.paletteValue |= m_oldBgp;
			}

			m_lcdBuffer[m_xPosition + screenWidth * m_ly] = colors[(pixel.paletteValue >> (pixel.colorIndex << 1)) & 0b11];
			//drawingLog << "	pushed pixel at x:" << std::dec << m_xPosition << ", y: " << static_cast<int>(m_ly)<< "\n	pixel color: " << std::hex << color
				//<< "\n	bgp value: " << static_cast<int>(m_bgp);
			++m_xPosition;
		}
		else 
		{
			--m_pixelsToDiscard;
			//drawingLog << "discarded a pixel";
		}

		m_pixelFifoBackground.pop_front();
		if(!m_pixelFifoSprite.empty()) m_pixelFifoSprite.pop_front();
	}
	//else //drawingLog << "	sleeping";
}

bool PPU::shouldPushSpritePixel() const
{
	constexpr uint8 spriteEnable{0b10};
	return !m_pixelFifoSprite.empty()
			&& (m_lcdc & spriteEnable
			&& m_pixelFifoSprite.front().colorIndex != 0
			&& (!m_pixelFifoSprite.front().backgroundPriority
			|| m_pixelFifoBackground.front().colorIndex == 0));
}

void PPU::clearFifos()
{
	m_pixelFifoBackground.clear();
	m_pixelFifoSprite.clear();
}

void PPU::hBlankCycle()
{
	if(m_cycleCounter == scanlineEndCycle)
	{
		++m_ly;
		updateCoincidenceFlag(false);
		updateMode(oamScan);
		m_cycleCounter = 0;
	}
	constexpr uint16 firstVBlankScanline{144};
	if(m_ly == firstVBlankScanline) //if this next scanline is the first of vBlank, vBlank for another 10 scanlines
	{
		m_vblankInterruptNextCycle = true;
		updateMode(vBlank);
		constexpr uint8 statOamSourceEnable{0b10'0000};
		if(m_stat & statOamSourceEnable) m_statInterrupt.sources[oamScan] = true; //re enable oam scan source if the corresponding stat bit is active as updateMode(vBlank) cleared it
	}
}

void PPU::vBlankCycle()
{
	if(m_cycleCounter == scanlineEndCycle)
	{
		++m_ly;
		updateCoincidenceFlag(false);
		m_cycleCounter = 0;
	}
	constexpr uint16 lastVBlankScanline{1}; //because at line 153 it goes back to 0 after 4 t-cycles
	if(m_ly == lastVBlankScanline)
	{
		m_ly = 0;
		updateMode(oamScan);
		m_fetcher.reset();
	}
}

void PPU::requestStatInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF, Bus::Component::ppu) | 0b10, Bus::Component::ppu);
}

void PPU::requestVBlankInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF, Bus::Component::ppu) | 0b1, Bus::Component::ppu);
}