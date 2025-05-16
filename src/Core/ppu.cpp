#include "ppu.h"
#include "gameboy.h"

PPU::PPU(Gameboy& gameboy)
	: m_gameboy{gameboy}
	, m_currentMode{OAM_SCAN} //even though OAM_SCAN is mode 2 the pipeline starts from it
	, m_spriteBuffer{}
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

void PPU::cycle()
{
	switch(m_currentMode)
	{
	case OAM_SCAN:
		++m_cycleCounter;
		Sprite sprite1{fetchSprite()};
		Sprite sprite2{fetchSprite()}; //each cycle it checks two sprites

		//todo add sprites to buffer

		if(m_cycleCounter == OAM_SCAN_DURATION) 
		{
			m_currentMode = DRAWING;
			m_currentSpriteAddress = OAM_MEMORY_START; //reset to first sprite
		}
		break;
	case DRAWING:
		++m_cycleCounter;

		//if(m_cycleCounter == MINIMUM_DRAWING_DURATION + extraCycles) m_currentMode = H_BLANK; //the amount of cycles changes depending on some factors so I will need to calculate them
		break;
	case H_BLANK:
		++m_cycleCounter;

		if(m_cycleCounter == SCANLINE_DURATION) //when scanline finishes
		{
			++m_ly; //update current scanline number
			m_cycleCounter = 0;
			m_currentMode = OAM_SCAN; //start the next one
		}
		if(m_ly == 144) m_currentMode = V_BLANK; //except if its scanline 144, in that case V_BLANK for another 10 scanlines
		break;
	case V_BLANK:
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
