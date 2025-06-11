#include "ppu.h"
#include "gameboy.h"

PPU::PPU(Gameboy& gameboy)
	: m_gameboy{gameboy}
	, m_currentMode{OAM_SCAN} //even though OAM_SCAN is mode 2 the ppu starts from it
	, m_spriteBuffer{10}
	, m_currentSpriteAddress{OAM_MEMORY_START}
	, m_cycleCounter{}
	, m_lcdc{}
	, m_stat{}
	, m_scy{}
	, m_scx{}
	, m_ly{}
	, m_lyc{}
	, m_bgp{}
	, m_obp0{}
	, m_obp1{}
	, m_wy{}
	, m_wx{}
{
	m_spriteBuffer.reserve(10); //sprite buffer max size is 10
}

void PPU::cycle()
{
	constexpr int SCANLINE_DURATION{114};
	switch(m_currentMode)
	{
	case OAM_SCAN:
	{
		++m_cycleCounter;
		Sprite sprite1{fetchSprite()};
		Sprite sprite2{fetchSprite()};
		tryAddSpriteToBuffer(sprite1);
		tryAddSpriteToBuffer(sprite2); //each cycle it checks two sprites

		constexpr int OAM_SCAN_DURATION{20};
		if(m_cycleCounter == OAM_SCAN_DURATION)
		{
			m_currentMode = DRAWING; //go to next mode
			m_currentSpriteAddress = OAM_MEMORY_START; //reset to first sprite
		}
		break;
	}
	case DRAWING:
	{
		++m_cycleCounter;

		constexpr int MINIMUM_DRAWING_DURATION{43};
		//if(m_cycleCounter == MINIMUM_DRAWING_DURATION + extraCycles) //the amount of cycles changes depending on some factors so I will need to calculate them 
		{
			m_currentMode = H_BLANK;
			m_spriteBuffer.clear(); //reset sprite buffer for next scanline
		}
		break;
	}
	case H_BLANK:
	{
		++m_cycleCounter;

		if(m_cycleCounter == SCANLINE_DURATION) //when scanline finishes
		{
			++m_ly; //update current scanline number
			m_cycleCounter = 0;
			m_currentMode = OAM_SCAN; //start the next one
		}
		if(m_ly == 144)
		{
			m_currentMode = V_BLANK; //except if its scanline 144, in that case V_BLANK for another 10 scanlines
			vBlankInterrupt();
		}
		break;
	}
	case V_BLANK:
	{
		++m_cycleCounter;

		if(m_cycleCounter == SCANLINE_DURATION) ++m_ly;
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

PPU::Sprite PPU::fetchSprite()
{
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
		m_spriteBuffer.emplace_back(sprite);
	}
}

void PPU::vBlankInterrupt() const
{
	m_gameboy.writeMemory(hardwareReg::IF, m_gameboy.readMemory(hardwareReg::IF) | 1); //bit 0 is vBlank interrupt
}