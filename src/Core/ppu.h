#pragma once 
#include "../type_alias.h"
#include "../hardware_registers.h"

class Gameboy;

class PPU
{
public:
	PPU(Gameboy& gameboy);

	enum Index
	{
		LCDC,
		STAT,
		SCY,
		SCX,
		LY,
		LYC,
		BGP,
		OBP0,
		OBP1,
		WY,
		WX,
	};

	void cycle();
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);

private:
	enum Mode
	{
		H_BLANK,
		V_BLANK,
		OAM_SCAN,
		DRAWING,
	};

	struct Sprite
	{
		uint8 yPosition{}; //byte 0 
		uint8 xPosition{}; //byte 1
		uint8 tileIndex{}; //byte 2
		uint8 flags{}; //byte 3
	};

	Sprite fetchSprite();

	static constexpr int OAM_SCAN_DURATION{20};
	static constexpr int MINIMUM_DRAWING_DURATION{43};
	static constexpr int SCANLINE_DURATION{114};
	static constexpr unsigned int OAM_MEMORY_START{0xFE00};

	Gameboy& m_gameboy;
	Mode m_currentMode;

	std::array<Sprite, 10> m_spriteBuffer;
	uint16 m_currentSpriteAddress;
	int m_cycleCounter;

	uint8 m_lcdc; //LCD control
	uint8 m_stat; //LDC status
	uint8 m_scy; //viewport y position
	uint8 m_scx; //viewport x position
	uint8 m_ly; //LCD y coordinate (current scanline number)
	uint8 m_lyc; //LY compare
	uint8 m_bgp; //BG palette data
	uint8 m_obp0; //OBJ palette 0 data
	uint8 m_obp1; //OBJ palette 1 data
	uint8 m_wy; //window y position
	uint8 m_wx; //window x position plus 7
};

