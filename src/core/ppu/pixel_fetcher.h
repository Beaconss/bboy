#pragma once
#include "../../type_alias.h"

#include <iostream>
#include <optional>

class PPU;

class PixelFetcher
{
private:
	PixelFetcher(PPU& ppu);
	friend class PPU;

	enum Mode
	{
		BACKGROUND,
		WINDOW,
		SPRITE,
	};

	enum Step
	{
		FETCH_TILE_NO,
		FETCH_TILE_DATA_LOW,
		FETCH_TILE_DATA_HIGH,
		PUSH_TO_FIFO,
	};

	void cycle();
	void update(std::optional<Mode> mode = std::nullopt);
	void checkWindowReached();
	void clearEndScanline();
	void clearEndFrame();
	void pushToBackgroundFifo();
	void pushToSpriteFifo();

	PPU& m_ppu;
	bool m_firstFetchCompleted;
	Mode m_mode;
	Mode m_previousMode;
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

