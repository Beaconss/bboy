#pragma once
#include <type_alias.h>

#include <iostream>

class PPU;

class PixelFetcher
{
private:
	PixelFetcher(PPU& ppu);
	friend class PPU;

	enum Step
	{
		fetchTileNo = 2,
		fetchTileDataLow = 4,
		fetchTileDataHigh = 6,
		pushToFifo = 7,
	};

	void reset();
	void resetEndScanline();
	void cycle();
	void updateTilemap();
	void checkForWindow();
	void checkForSprite();
	void pushToBackgroundFifo();

	template<typename T>
	void pushToSpriteFifo(const T sprite);

	PPU& m_ppu;
	bool m_isFetchingWindow;
	bool m_firstFetchCompleted;
	int m_backgroundCycleCounter;
	int m_spriteFetchDelay;

	int m_tileX;
	uint16 m_tilemap;
	uint8 m_tileNumber;
	uint8 m_tileDataLow;
	uint8 m_tileDataHigh;
	uint16 m_tileAddress;

	int m_windowLineCounter;
	bool m_wyLyCondition;
};

