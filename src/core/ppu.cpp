#include "ppu.h"
#include "gameboy.h"

PPU::PPU(Gameboy& gameboy)
	: m_gameboy{gameboy}
	, m_platform{Platform::getInstance()}
	, m_currentMode{OAM_SCAN} //even though OAM_SCAN is mode 2 the ppu starts from it
	, m_lcdBuffer{}
	, m_spriteBuffer{10}
	, m_currentSpriteAddress{OAM_MEMORY_START}
	, fetcher{}
	, m_cycleCounter{}
	, m_lcdc{0b10000000} //start with ppu active
	, m_stat{}
	, m_scy{}
	, m_scx{}
	, m_ly{}
	, m_lyc{}
	, m_bgp{}
	, m_obp0{}
	, m_obp1{}
	, m_wy{}
	, m_wx{7}
{
	m_spriteBuffer.reserve(10); //sprite buffer max size is 10
	m_platform.updateScreen(m_lcdBuffer.data());
}

void PPU::cycle()
{
	if(!(m_lcdc & 0b10000000)) //if bit 7 is disabled clear screen and return because the ppu is not active
	{
		//std::fill(std::begin(m_lcdBuffer), std::end(m_lcdBuffer), 0); 
		//m_platform.updateScreen(m_lcdBuffer.data()); //this seems pretty useless
		return;
	}

	constexpr int SCANLINE_END_CYCLE{114};
	switch(m_currentMode)
	{
	case OAM_SCAN:
	{
		++m_cycleCounter;
		Sprite sprite1{fetchSprite()};
		Sprite sprite2{fetchSprite()};
		tryAddSpriteToBuffer(sprite1);
		tryAddSpriteToBuffer(sprite2); //each cycle it checks two sprites

		constexpr int OAM_SCAN_END_CYCLE{20};
		if(m_cycleCounter == OAM_SCAN_END_CYCLE)
		{
			m_currentMode = DRAWING; //go to next mode
			m_currentSpriteAddress = OAM_MEMORY_START; //reset to first sprite
		}
		break;
	}
	case DRAWING:
	{
		++m_cycleCounter;

		constexpr int MIN_DRAWING_END_CYCLE{43};
		int extraCycles{};

		if(fetcher.doFirstTwoSteps)
		{
			//fetch tile no.
			constexpr uint8 TILE_MAP_SIZE{0x3FF};

			const bool backgroundEnabled{m_lcdc & 0b1};
			const bool windowEnabled{backgroundEnabled && m_lcdc & 0b100000};
			const bool spriteEnabled{m_lcdc & 0b10};

			const uint16 backgroundTilemap{m_lcdc & 0b1000 ? 0x9C00 : 0x9800};
			const uint16 windowTileMap{m_lcdc & 0b1000000 ? 0x9C00 : 0x9800};

			//to know if im fetching background or window I need to use:
			//scx and scy which is the screen position
			//wx - 7 and wy which is window position
			//fetcher.xPosCounter and ly which is the current fetching position
			//(I hope at least)

			const bool isFetchingWindow{windowEnabled && m_ly >= m_wy && fetcher.xPosCounter >= m_wx - 7}; //dont knwo if m_ly >= m_wy will work

			if(isFetchingWindow)
			{

			}
			else //background fetching
			{

			}

			//fetch tile data(low)

			fetcher.doFirstTwoSteps = false;
		}
		else
		{
			//fetch tile data(high)

			//push to FIFO

			fetcher.doFirstTwoSteps = true;
		}

		if(m_cycleCounter == MIN_DRAWING_END_CYCLE + extraCycles) //the amount of cycles changes depending on some factors so I will need to calculate them 
		{
			m_currentMode = H_BLANK;
			m_spriteBuffer.clear(); //reset sprite buffer for next scanline
		}
		break;
	}
	case H_BLANK:
	{
		++m_cycleCounter;

		constexpr int LAST_SCANLINE_BEFORE_V_BLANK{144};
		if(m_cycleCounter == SCANLINE_END_CYCLE) //when scanline finishes
		{
			//std::cout << "Scanline " << (int)m_ly << " finished";
			++m_ly; //update current scanline number
			m_cycleCounter = 0;
			m_currentMode = OAM_SCAN; //start the next one
		}
		if(m_ly == LAST_SCANLINE_BEFORE_V_BLANK)
		{
			m_currentMode = V_BLANK; //if its the last, V_BLANK for another 10 scanlines
			vBlankInterrupt();
		}
		break;
	}
	case V_BLANK:
	{
		++m_cycleCounter;

		if(m_cycleCounter == SCANLINE_END_CYCLE) 
		{
			//std::cout << "Scanline " << (int)m_ly << " finished\n";
			++m_ly;
			m_cycleCounter = 0;
		}
		if(m_ly == 154)
		{
			m_ly = 0;
			m_cycleCounter = 0;
			m_currentMode = OAM_SCAN;
			//renderflag = true //setup a render flag for the renderer
		}
		break;
	}
	}
}

PPU::Sprite PPU::fetchSprite()
{
	//TODO: fix for tall sprite mode
	Sprite sprite;
	sprite.yPosition = m_gameboy.readMemory(m_currentSpriteAddress);
	sprite.xPosition = m_gameboy.readMemory(m_currentSpriteAddress + 1);
	sprite.tileIndex = m_gameboy.readMemory(m_currentSpriteAddress + 2);
	sprite.flags = m_gameboy.readMemory(m_currentSpriteAddress + 3);
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

void PPU::vBlankInterrupt() const
{
	m_gameboy.writeMemory(hardwareReg::IF, m_gameboy.readMemory(hardwareReg::IF) | 1); //bit 0 is vBlank interrupt
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