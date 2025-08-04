#pragma once 
#include "../../type_alias.h"
#include "../../hardware_registers.h"
#include "../../platform.h"
#include "../bus.h"
#include "pixel_fetcher.h"

#include <vector>
#include <array>
#include <queue>
#include <optional>

class PPU
{
public:
	PPU(Bus& bus);

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
	PPU::Mode getCurrentMode() const;
	bool isEnabled() const;
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);
private:
	friend class PixelFetcher;

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
	};

	struct StatInterrupt
	{
		std::array<bool, 4> sources{};
		enum Source
		{
			H_BLANK,
			V_BLANK,
			OAM_SCAN,
			LY_COMPARE,
		};

		bool calculateResult() const;
		bool previousResult{false};
	};
	void setStatModeSources();

	void switchMode(const Mode mode);
	void updateCoincidenceFlag();

	Sprite fetchSprite();
	void tryAddSpriteToBuffer(const Sprite& sprite);
	void clearBackgroundFifo();
	void clearSpriteFifo();

	void requestStatInterrupt() const;
	void requestVBlankInterrupt() const;

	static constexpr std::array<uint16, 4> colors //in rgb565
	{
		0xFFFF,
		0x7BEF,
		0x39E7,
		0x0,
	};

	static constexpr uint16 OAM_MEMORY_START{0xFE00};

	Bus& m_bus;
	Platform& m_platform;
	Mode m_currentMode;
	StatInterrupt m_statInterrupt;
	
	std::array<uint16, SCREEN_WIDTH * SCREEN_HEIGHT> m_lcdBuffer;
	std::vector<Sprite> m_spriteBuffer;
	uint16 m_currentSpriteAddress;

	std::queue<Pixel> m_pixelFifoBackground;
	std::queue<Pixel> m_pixelFifoSprite;
	PixelFetcher m_fetcher;
	
	uint16 m_tCycleCounter; //max value is 456 so uint16 is fine
	uint8 m_currentXPosition;
	uint8 m_backgroundPixelsToDiscard;

	//maybe i will delete some of these to just use the memory instead
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

