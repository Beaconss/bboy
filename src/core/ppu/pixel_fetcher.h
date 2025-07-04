#pragma once
#include "../../type_alias.h"

#include <iostream>
#include <iomanip>

class PPU;

class PixelFetcher
{
public:
	PixelFetcher(PPU& ppu);

	void cycle();
	uint8 getXPosCounter() const;
	void clearWindowLineCounter(); //this needs to be reset separately
	void clear();

private:
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

	void pushPixelsToFifo();

	PPU& m_ppu;
	bool m_firstFetchOnScanline;
	Mode m_currentMode;
	Step m_currentStep;
	uint8 m_stepCycle;

	uint8 m_xPosCounter;
	uint8 m_windowLineCounter;

	uint8 m_tileNumber;
	uint8 m_tileDataLow;
	uint8 m_tileDataHigh;
	uint16 m_tileAddress;
};

