#pragma once 
#include "../../type_alias.h"
#include "../../hardware_registers.h"
#include "../../platform.h"
#include "pixel_fetcher.h"

#include <vector>
#include <array>
#include <queue>

/*
from my understanding since every pixel has a color ID, which maps to 2 specific bits in the corresponding palette,
I can make a function that takes the id and extract the 2 specific bits which are then used as an index for
the colors array, probably in the ppu
*/

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

	enum Mode
	{
		H_BLANK,
		V_BLANK,
		OAM_SCAN,
		DRAWING,
	};

	void cycle();
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);
	Mode getCurrentMode() const;

private:
	struct Sprite
	{
		uint8 yPosition{}; //byte 0 
		uint8 xPosition{}; //byte 1
		uint8 tileIndex{}; //byte 2
		uint8 flags{}; //byte 3
		//todo: add each flag
	};

	struct Pixel
	{
		uint8 data{}; //color index at bit 0 and 1. background priority at bit 7 for sprite pixels
		uint8 xPosition{};
	};
	/* for now I keep this here, if I realize i dont need it I will delete this
	enum Color
	{
		WHITE,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK
	};
	*/

	friend PixelFetcher;
	Sprite fetchSprite();
	void tryAddSpriteToBuffer(const Sprite& sprite);
	void vBlankInterrupt() const;

	static constexpr std::array<uint8, 4> colors //in rgb332, use color id as index
	{
		0b11111111,
		0b11011010,
		0b01001001,
		0b0,
	};

	static constexpr uint16 OAM_MEMORY_START{0xFE00};

	Gameboy& m_gameboy;
	Platform& m_platform;
	Mode m_currentMode;
	
	std::array<uint8, SCREEN_WIDTH * SCREEN_HEIGHT + 1> m_lcdBuffer;
	std::vector<Sprite> m_spriteBuffer;
	std::queue<Pixel> m_pixelFifoBackground;
	std::queue<Pixel> m_pixelFifoSprite;
	uint16 m_currentSpriteAddress;
	PixelFetcher m_fetcher;
	uint16 m_tCycleCounter; //max value is 456 so uint16 is fine
	uint8 m_drawingDelay;

	uint8 m_lcdc; //LCD control
	uint8 m_stat; //LDC status
	uint8 m_scy; //viewport y position
	uint8 m_scx; //viewport x position
	uint8 m_ly; //LCD y coordinate (current scanline number)
	uint8 m_lyc; //LY compare
	uint8 m_bgp; //background palette data
	uint8 m_obp0; //sprite palette data 0
	uint8 m_obp1; //sprite palette data 1
	uint8 m_wy; //window y position
	uint8 m_wx; //window x position plus 7
};

