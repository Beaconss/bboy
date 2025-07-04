#include "ppu.h"
#include "../memory_bus.h"

PPU::PPU(MemoryBus& bus)
	: m_bus{bus}
	, m_platform{Platform::getInstance()}
	, m_currentMode{V_BLANK}
	, m_lcdBuffer{}
	, m_spriteBuffer{}
	, m_pixelFifoBackground{}
	, m_pixelFifoSprite{}
	, m_currentSpriteAddress{OAM_MEMORY_START}
	, m_fetcher{*this}
	, m_tCycleCounter{}
	, m_lcdc{0x90}
	, m_stat{0x81}
	, m_scy{}
	, m_scx{}
	, m_ly{0x91}
	, m_lyc{}
	, m_bgp{0xFC}
	, m_obp0{}
	, m_obp1{}
	, m_wy{}
	, m_wx{7}
{
	m_spriteBuffer.reserve(10); //sprite buffer max size is 10
	m_platform.updateScreen(m_lcdBuffer.data());
}

//time to refactor this to make it t-cycle and not m-cycle
void PPU::cycle()
{
	if(!(m_lcdc & 0b10000000)) //if bit 7 is disabled the ppu is not active
	{
		//std::fill(std::begin(m_lcdBuffer), std::end(m_lcdBuffer), 0); 
		//m_platform.updateScreen(m_lcdBuffer.data()); //this seems pretty useless
		m_tCycleCounter = 0;
		return;
	}

	constexpr int SCANLINE_END_CYCLE{456};
	++m_tCycleCounter;
	switch(m_currentMode)
	{
	case OAM_SCAN:
	{
		if(m_tCycleCounter % 2 == 0) 
		{
			tryAddSpriteToBuffer(fetchSprite()); //every 2 cycles it checks one sprites

			constexpr int OAM_SCAN_END_CYCLE{80};
			if(m_tCycleCounter == OAM_SCAN_END_CYCLE) //80 is even so I avoid checking in odd cycles 
			{
				m_currentMode = DRAWING; //go to next mode
				m_currentSpriteAddress = OAM_MEMORY_START; //reset to first sprite
			}
		}
		break;
	}
	case DRAWING:
	{
		//TODO: STAT interrupt, background scrolling

		m_fetcher.cycle();

		if(!m_pixelFifoBackground.empty()) //if fetcher has pixels push them to the screen
		{
			//this gets the right bits of the palette based on the color index of the pixel and 
			//use this as an index for the rgb332 color array
			const Pixel pixel{m_pixelFifoBackground.front()};
			const uint8 color{colors[(m_bgp >> (pixel.data * 2)) & 0b11]};
			m_lcdBuffer[pixel.xPosition + SCREEN_WIDTH * m_ly] = color;
			
			m_pixelFifoBackground.pop();
		}

		if(m_fetcher.getXPosCounter() >= SCREEN_WIDTH) //when the end of the screen is reached, clear all and go to next mode
		{
			m_currentMode = H_BLANK;
			m_spriteBuffer.clear();
			m_fetcher.clear();
			clearBackgroundFifo();
			clearSpriteFifo();
		}
		break;
	}
	case H_BLANK:
	{
		constexpr int FIRST_V_BLANK_SCANLINE{144};
		if(m_tCycleCounter == SCANLINE_END_CYCLE) //when scanline finishes
		{
			++m_ly; //update current scanline number
			m_tCycleCounter = 0;
			m_currentMode = OAM_SCAN; //start the next one
		}
		if(m_ly == FIRST_V_BLANK_SCANLINE) //except if this next scanline is the first V_BLANK for another 10 scanlines
		{
			m_currentMode = V_BLANK;
			vBlankInterrupt();
			m_fetcher.clearWindowLineCounter();
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
		if(m_ly == 154)
		{
			m_platform.updateScreen(m_lcdBuffer.data());
			m_platform.render();
			m_ly = 0;
			m_tCycleCounter = 0;
			m_currentMode = OAM_SCAN;
		}
		break;
	}
	}
}

PPU::Mode PPU::getCurrentMode() const
{
	return m_currentMode;
}

PPU::Sprite PPU::fetchSprite()
{
	//TODO: fix for tall sprite mode
	Sprite sprite;
	sprite.yPosition = m_bus.read(m_currentSpriteAddress);
	sprite.xPosition = m_bus.read(m_currentSpriteAddress + 1);
	sprite.tileIndex = m_bus.read(m_currentSpriteAddress + 2);
	sprite.flags = m_bus.read(m_currentSpriteAddress + 3);
	m_currentSpriteAddress += 4; //go to next sprite
	return sprite;
}

void PPU::tryAddSpriteToBuffer(const Sprite& sprite)
{
	constexpr int SPRITE_BUFFER_MAX_SIZE{10};
	constexpr uint8 TALL_SPRITE_MODE{0b100};

	if(m_spriteBuffer.size() < SPRITE_BUFFER_MAX_SIZE &&
		sprite.xPosition > 0 &&
		m_ly + 16 >= sprite.yPosition &&
		m_ly + 16 < sprite.yPosition + ((m_lcdc & TALL_SPRITE_MODE) ? 16 : 8)) //16 or 8 depending if tall sprite mode is enabled
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

void PPU::vBlankInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF) | 1); //bit 0 is vBlank interrupt
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
	default: return 0;
	}
}

void PPU::write(const Index index, const uint8 value)
{
	switch(index)
	{
	case LCDC: m_lcdc = value; break;
	case STAT: m_stat = value; break;
	case SCY: m_scy = value; break;
	case SCX: m_scx = value; break;
	case LY: break; //read only
	case LYC: m_lyc = value; break;
	case BGP: m_bgp = value; break;
	case OBP0: m_obp0 = value; break;
	case OBP1: m_obp1 = value; break;
	case WY: m_wy = value; break;
	case WX: m_wx = value; break;
	}
}