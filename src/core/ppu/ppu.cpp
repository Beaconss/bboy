#include "ppu.h"
#include "../memory_bus.h"

PPU::PPU(MemoryBus& bus)
	: m_bus{bus}
	, m_platform{Platform::getInstance()}
	, m_currentMode{V_BLANK}
	, m_statInterrupt{}
	, m_lcdBuffer{}
	, m_spriteBuffer{}
	, m_pixelFifoBackground{}
	, m_pixelFifoSprite{}
	, m_currentSpriteAddress{OAM_MEMORY_START}
	, m_fetcher{*this}
	, m_tCycleCounter{}
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

	constexpr int SCANLINE_END_CYCLE{456};

	++m_tCycleCounter;
	switch(m_currentMode)
	{
	case OAM_SCAN:
	{
		if(m_tCycleCounter % 2 == 0)
		{
			tryAddSpriteToBuffer(fetchSprite());

			constexpr int OAM_SCAN_END_CYCLE{80};
			if(m_tCycleCounter == OAM_SCAN_END_CYCLE)
			{
				m_currentSpriteAddress = OAM_MEMORY_START;
				switchMode(DRAWING);
			}
		}
		break;
	}
	case DRAWING:
	{
		//TODO: background scrolling

		m_fetcher.cycle();

		if(!m_pixelFifoBackground.empty())
		{
			//this gets the right bits of the palette based on the color index of the pixel and 
			//use this as an index for the rgb332 color array
			const Pixel pixel{m_pixelFifoBackground.front()};
			const uint16 color{colors[(m_bgp >> (pixel.colorIndex * 2)) & 0b11]};
			m_lcdBuffer[pixel.xPosition + SCREEN_WIDTH * m_ly] = color;

			m_pixelFifoBackground.pop();

			m_fetcher.checkWindowReached();
		}

		if(m_fetcher.getXPosCounter() >= SCREEN_WIDTH) //when the end of the screen is reached, clear all and go to next mode
		{
			m_spriteBuffer.clear();
			m_fetcher.clearEndScanline();
			clearBackgroundFifo();
			clearSpriteFifo();
			switchMode(H_BLANK);
		}
		break;
	}
	case H_BLANK:
	{
		constexpr int FIRST_V_BLANK_SCANLINE{144};
		if(m_tCycleCounter == SCANLINE_END_CYCLE)
		{
			++m_ly;
			updateCoincidenceFlag();
			m_tCycleCounter = 0;
			switchMode(OAM_SCAN);
		}

		if(m_ly == FIRST_V_BLANK_SCANLINE) //if this next scanline is the first of V_BLANK, V_BLANK for another 10 scanlines
		{
			requestVBlankInterrupt();
			m_fetcher.clearEndFrame();
			switchMode(V_BLANK);
		}
		break;
	}
	case V_BLANK:
	{
		if(m_tCycleCounter == SCANLINE_END_CYCLE) 
		{
			++m_ly;
			updateCoincidenceFlag();
			m_tCycleCounter = 0;
		}
		if(m_ly == 154)
		{
			m_platform.updateScreen(m_lcdBuffer.data());
			m_platform.render();
			
			m_ly = 0;
			updateCoincidenceFlag();
			m_tCycleCounter = 0;
			switchMode(OAM_SCAN);
		}
		break;
	}
	}

	bool statResult {m_statInterrupt.calculateResult()};
	if(!m_statInterrupt.previousResult && statResult) requestStatInterrupt();

	m_statInterrupt.previousResult = statResult;
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
	case LCDC: m_lcdc = value; break;
	case STAT: m_stat = ((m_stat & 0b111) | (value & ~0b111)) | 0x80; break; //bit 0-1-2 are read only and bit 7 is always 1
	case SCY: m_scy = value; break;
	case SCX: m_scx = value; break;
	case LY: break;
	case LYC: m_lyc = value; updateCoincidenceFlag(); break;
	case BGP: m_bgp = value; break;
	case OBP0: m_obp0 = value; break;
	case OBP1: m_obp1 = value; break;
	case WY: m_wy = value; break;
	case WX: m_wx = value; break;
	}
}

void PPU::switchMode(const Mode mode)
{
	m_currentMode = mode;
	m_stat = (m_stat & ~0b11) | static_cast<uint8>(mode); //bits 1-0 of stat store the current mode

	if(mode != DRAWING && m_stat & (1 << (3 + mode)))  //at bits 3-4-5 are stored the stat condition enable for mode 0, 1 and 2 respectively
	{
		__debugbreak();
		//requestStatInterrupt();
	}
}

void PPU::updateCoincidenceFlag()
{
	m_stat = (m_stat & ~0b100) | (m_ly == m_lyc ? 0b100 : 0);
	if(m_stat & 0x40 && m_stat & 0b100)
	{
		//__debugbreak();
		m_statInterrupt.lyCompareSource = true;
	}
	else m_statInterrupt.lyCompareSource = false;
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
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF) | 0b10);
}

void PPU::requestVBlankInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF) | 0b1);
}

bool PPU::StatInterrupt::calculateResult()
{
	return HBlankSource || VBlankSource || OamScanSource || lyCompareSource;
}
