#pragma once 
#include "../../type_alias.h"
#include "../../hardware_registers.h"
#include "../../platform.h"
#include "pixel_fetcher.h"

#include <vector>
#include <array>
#include <queue>
#include <optional>

class MemoryBus;

class PPU
{
public:
	PPU(MemoryBus& bus);

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
		//bool backgroundPriority{}; //this should hold bit 7 of the attribute byte of the sprite, when I do sprites I will decide how to do this
		uint8 colorIndex{};
		uint8 xPosition{};
	};

	friend PixelFetcher;
	void switchMode(const Mode mode);

	Sprite fetchSprite();
	void tryAddSpriteToBuffer(const Sprite& sprite);
	void clearBackgroundFifo();
	void clearSpriteFifo();

	void updateCoincidenceFlag();
	void checkLyStatInterrupt();
	void statInterrupt() const;
	void vBlankInterrupt() const;

	static constexpr std::array<uint8, 4> colors //in rgb332
	{
		0b11111111,
		0b11011010,
		0b01001001,
		0b0,
	};

	static constexpr uint16 OAM_MEMORY_START{0xFE00};

	MemoryBus& m_bus;
	Platform& m_platform;
	Mode m_currentMode;
	
	std::array<uint8, SCREEN_WIDTH * SCREEN_HEIGHT + 1> m_lcdBuffer;
	std::vector<Sprite> m_spriteBuffer;
	uint16 m_currentSpriteAddress;

	std::queue<Pixel> m_pixelFifoBackground;
	std::queue<Pixel> m_pixelFifoSprite;
	PixelFetcher m_fetcher;
	
	bool m_statBlocked;
	uint16 m_tCycleCounter; //max value is 456 so uint16 is fine

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

