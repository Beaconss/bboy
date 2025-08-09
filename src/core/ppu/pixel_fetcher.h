#pragma once
#include "../../type_alias.h"

#include <iostream>
#include <optional>

class PPU;

class PixelFetcher
{
private:
	enum Mode
	{
		BACKGROUND,
		WINDOW,
		SPRITE,
	};

public:
	PixelFetcher(PPU& ppu);

	void cycle();
	void update(std::optional<Mode> mode = std::nullopt);
	void checkWindowReached();
	void clearEndScanline();
	void clearEndFrame();

private:
	

	enum Step
	{
		FETCH_TILE_NO,
		FETCH_TILE_DATA_LOW,
		FETCH_TILE_DATA_HIGH,
		PUSH_TO_FIFO,
	};

	void pushToBackgroundFifo();

	PPU& m_ppu;
	bool m_firstFetchCompleted;
	Mode m_mode;
	Step m_step;
	uint8 m_stepCycle;

	uint8 m_xPositionCounter;
	uint16 m_backGroundTileMap;
	uint8 m_tileNumber;
	uint8 m_tileDataLow;
	uint8 m_tileDataHigh;
	uint16 m_tileAddress;

	int16 m_windowLineCounter;
	bool m_wyLyCondition;
};

