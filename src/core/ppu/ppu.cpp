#include "ppu.h"

PPU::PPU(Bus& bus)
	: m_bus{bus}
	, m_platform{Platform::getInstance()}
	, m_fetcher{*this}
	, m_statInterrupt{}
	, m_mode{V_BLANK}
	, m_tCycleCounter{}
	, m_vblankInterruptNextCycle{}
	, m_lcdBuffer{}
	, m_xPosition{}
	, m_backgroundPixelsToDiscard{}
	, m_spriteBuffer{}
	, m_spriteAddress{OAM_MEMORY_START}
	, m_pixelFifoBackground{}
	, m_pixelFifoSprite{}
	, m_lcdc{0x91}
	, m_stat{0x85}
	, m_scy{}
	, m_scx{}
	, m_ly{}
	, m_lyc{}
	, m_bgp{0xFC}
	, m_obp0{}
	, m_obp1{}
	, m_wy{}
	, m_wx{}
{
	m_spriteBuffer.reserve(10);
	m_platform.updateScreen(m_lcdBuffer.data());
}

void PPU::cycle()
{
	if(!(m_lcdc & 0x80)) //bit 7 is ppu enable
	{
		//std::fill(std::begin(m_lcdBuffer), std::end(m_lcdBuffer), 0); 
		//m_platform.updateScreen(m_lcdBuffer.data());
		return;
	}

	++m_tCycleCounter;
	if(m_tCycleCounter % 4 == 1) //every m-cycle
	{
		m_stat = (m_stat & ~0b11) | m_mode; //bits 1-0 of stat store the current mode
		updateCoincidenceFlag();
		handleStatInterrupt();
		

		if(m_vblankInterruptNextCycle)
		{
			requestVBlankInterrupt();
			m_statInterrupt.sources[OAM_SCAN] = false; //reset this in case it was briefly true
			m_vblankInterruptNextCycle = false;
		}
	}

	constexpr uint16 SCANLINE_END_CYCLE{456};
	switch(m_mode)
	{
	case OAM_SCAN:
	{
		constexpr int SPRITE_BUFFER_MAX_SIZE{10};

		if(m_tCycleCounter % 2 == 0 && m_spriteBuffer.size() < SPRITE_BUFFER_MAX_SIZE)
		{
			tryAddSpriteToBuffer(fetchSprite());
		}

		constexpr uint16 OAM_SCAN_END_CYCLE{80};
		if(m_tCycleCounter == OAM_SCAN_END_CYCLE)
		{
			m_spriteAddress = OAM_MEMORY_START;
			switchMode(DRAWING);
			m_backgroundPixelsToDiscard = m_scx % 8; //i think this should be in the first cycle of drawing so the next one but the test pass so if it doesnt create problems
		}
		break;
	}
	case DRAWING:
	{
		m_fetcher.cycle();

		if(!m_pixelFifoBackground.empty())
		{
			if(m_backgroundPixelsToDiscard == 0)
			{
				//this gets the right bits of the palette based on the color index of the pixel and 
				//use this as an index for the rgb332 color array(lcdc bit 1 is background/window enable)
				const Pixel pixel{m_lcdc & 1 ? m_pixelFifoBackground.front() : Pixel{0}};
				const uint8 color{colors[(m_bgp >> (pixel.colorIndex * 2)) & 0b11]};
				m_lcdBuffer[m_xPosition + (SCREEN_WIDTH * m_ly)] = color;
				++m_xPosition;
			}
			else --m_backgroundPixelsToDiscard;

			m_pixelFifoBackground.pop();
			m_fetcher.checkWindowReached();
		}

		if(m_xPosition == SCREEN_WIDTH) //when the end of the screen is reached, clear all and go to next mode
		{
			m_xPosition = 0;
			m_fetcher.clearEndScanline();
			clearBackgroundFifo();
			clearSpriteFifo();
			m_spriteBuffer.clear(); 
			switchMode(H_BLANK);
		}
		break;
	}
	case H_BLANK:
	{
		if(m_tCycleCounter == SCANLINE_END_CYCLE)
		{
			++m_ly;
			switchMode(OAM_SCAN);
			m_tCycleCounter = 0;
		}
		constexpr uint16 FIRST_V_BLANK_SCANLINE{144};
		if(m_ly == FIRST_V_BLANK_SCANLINE) //if this next scanline is the first of V_BLANK, V_BLANK for another 10 scanlines
		{
			m_vblankInterruptNextCycle = true;
			m_fetcher.clearEndFrame();
			switchMode(V_BLANK);
			if(m_stat & 0b10'0000) m_statInterrupt.sources[OAM_SCAN] = true; //re enable oam scan source if the corresponding stat bit is active as switchMode(V_BLANK) cleared it
		}
		break;
	}
	case V_BLANK:
	{
		if(m_tCycleCounter == SCANLINE_END_CYCLE)
		{
			++m_ly;
			m_tCycleCounter = 0;
		}
		constexpr uint16 LAST_VBLANK_SCANLINE{154};
		if(m_ly == LAST_VBLANK_SCANLINE)
		{
			m_platform.updateScreen(m_lcdBuffer.data());
			m_platform.render();
			m_ly = 0;
			switchMode(OAM_SCAN);
		}
		break;
	}
	}
}

PPU::Mode PPU::getCurrentMode() const
{
	return static_cast<Mode>(m_stat & 0b11);
}

bool PPU::isEnabled() const
{
	return m_lcdc & 0x80;
}

uint8 PPU::read(const Index index) const
{
	switch(index)
	{
	case LCDC: return m_lcdc;
	case STAT: return m_stat;
	case SCY: return m_scy;
	case SCX: return m_scx;
	case LY: return m_ly;
	case LYC: return m_lyc;
	case BGP: return m_bgp;
	case OBP0: return m_obp0;
	case OBP1: return m_obp1;
	case WY: return m_wy;
	case WX: return m_wx;
	default: return 0xFF;
	}
}

void PPU::write(const Index index, const uint8 value)
{
	switch(index)
	{
	case LCDC: 
		m_lcdc = value; 
		m_fetcher.update();
		break;
	case STAT: 
		m_stat = ((m_stat & 0b111) | (value & ~0b111)) | 0x80; //bit 0-1-2 are read only and bit 7 is always 1
		setStatModeSources();
		break;
	case SCY: m_scy = value; break;
	case SCX: m_scx = value; break;
	case LY: break;
	case LYC: m_lyc = value; break;
	case BGP: m_bgp = value; break;
	case OBP0: m_obp0 = value; break;
	case OBP1: m_obp1 = value; break;
	case WY: m_wy = value; break;
	case WX: m_wx = value; break;
	}
}

//this sets the sources so that it is done only when needed, regarding the mode, the actual mode bits in the stat register are updated every m-cycle based on this mode
void PPU::switchMode(const Mode mode)
{
	m_mode = mode; 
	setStatModeSources();
}

void PPU::updateCoincidenceFlag()
{
	m_stat = (m_stat & ~0b100) | (m_ly == m_lyc ? 0b100 : 0);
	if(m_stat & 0x40 && m_stat & 0b100)
	{
		m_statInterrupt.sources[StatInterrupt::LY_COMPARE] = true;
	}
	else m_statInterrupt.sources[StatInterrupt::LY_COMPARE] = false;
}

PPU::Sprite PPU::fetchSprite()
{
	//TODO: fix for tall sprite mode
	Sprite sprite;
	sprite.yPosition = m_bus.read(m_spriteAddress, Bus::Component::PPU);
	sprite.xPosition = m_bus.read(m_spriteAddress + 1, Bus::Component::PPU);
	sprite.tileIndex = m_bus.read(m_spriteAddress + 2, Bus::Component::PPU);
	sprite.flags = m_bus.read(m_spriteAddress + 3, Bus::Component::PPU);
	m_spriteAddress += 4; //go to next sprite
	return sprite;
}

void PPU::tryAddSpriteToBuffer(const Sprite& sprite)
{
	constexpr uint8 TALL_SPRITE_MODE{0b100};

	if( sprite.xPosition > 0 &&
		m_ly + 16 >= sprite.yPosition &&
		m_ly + 16 < sprite.yPosition + ((m_lcdc & TALL_SPRITE_MODE) ? 16 : 8))
	{
		m_spriteBuffer.push_back(sprite);
	}
}

void PPU::clearBackgroundFifo()
{
	std::queue<Pixel> emptyBackgroundFifo{};
	m_pixelFifoBackground.swap(emptyBackgroundFifo);
}

void PPU::clearSpriteFifo()
{
	std::queue<Pixel> emptySpriteFifo{};
	m_pixelFifoSprite.swap(emptySpriteFifo);
}

void PPU::requestStatInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF, Bus::Component::PPU) | 0b10, Bus::Component::PPU);
}

void PPU::requestVBlankInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF, Bus::Component::PPU) | 0b1, Bus::Component::PPU);
}

void PPU::handleStatInterrupt()
{
	const bool statResult{m_statInterrupt.sources[StatInterrupt::H_BLANK]
						 || m_statInterrupt.sources[StatInterrupt::V_BLANK]
						 || m_statInterrupt.sources[StatInterrupt::OAM_SCAN]
						 || m_statInterrupt.sources[StatInterrupt::LY_COMPARE]};
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