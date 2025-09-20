#pragma once 
#include <type_alias.h>
#include <hardware_registers.h>
#include <core/bus.h>
#include <core/ppu/pixel_fetcher.h>

#include <vector>
#include <array>
#include <queue>
#include <ranges>
#include <algorithm>

constexpr int SCREEN_WIDTH{160};
constexpr int SCREEN_HEIGHT{144};

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

	struct Sprite
	{
		uint8 yPosition{0xFF}; //byte 0(0xFF is offscreen) 
		uint8 xPosition{}; //byte 1
		uint8 tileNumber{}; //byte 2
		uint8 flags{}; //byte 3
		//bit 7: background priority
		//bit 6: y-flip
		//bit 5: x-flip
		//bit 4: palette
		//other bits are cgb only
	};

	void reset();
	void cycle();

	PPU::Mode getMode() const;
	const uint16* getLcdBuffer() const;
	
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);
private:
	friend class PixelFetcher;

	struct Pixel
	{
		//default to 0xFF because its not a valid color index, so when resetting the lcd pixel buffer each pixel is invalidated
		uint8 colorIndex{0xFF}; 
		bool spritePalette{}; //obp1 if true, obp0 if false
		bool backgroundPriority{};
		uint8 paletteValue{};
	};

	struct StatInterrupt
	{
		enum Source
		{
			H_BLANK,
			V_BLANK,
			OAM_SCAN,
			LY_COMPARE,
		};
		std::array<bool, 4> sources{};
		bool previousResult{false};
	};
	void handleStatInterrupt();
	void setStatModeSources();

	void updateMode(const Mode mode);
	void updateCoincidenceFlag(bool set = true);

	void oamScanCycle();
	void tryAddSpriteToBuffer(const Sprite sprite);

	void pushToLcd();
	bool shouldPushSpritePixel() const;

	void hBlankCycle();
	void vBlankCycle();

	void clearFifos();

	void requestStatInterrupt() const;
	void requestVBlankInterrupt() const;

	static constexpr std::array<uint16, 4> colors //in rgb565
	{
		0xFFFF,
		0xC618,
		0x4208,
		0x0,
	};

	static constexpr uint16 OAM_MEMORY_START{0xFE00};

	Bus& m_bus;
	PixelFetcher m_fetcher;
	StatInterrupt m_statInterrupt;
	Mode m_mode;
	
	int m_cycleCounter;
	bool m_vblankInterruptNextCycle;
	bool m_firstDrawingCycleDone;
	bool m_reEnabling;
	int m_reEnableDelay;

	std::array<uint16, SCREEN_WIDTH * SCREEN_HEIGHT> m_lcdBuffer;
	std::array<Pixel, SCREEN_WIDTH* SCREEN_HEIGHT> m_lcdPixels;
	int m_xPosition; //x position of the pixel to output
	int m_pixelsToDiscard;

	std::vector<Sprite> m_spriteBuffer;
	uint16 m_spriteAddress;

	std::deque<Pixel> m_pixelFifoBackground;
	std::deque<Pixel> m_pixelFifoSprite;
	
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