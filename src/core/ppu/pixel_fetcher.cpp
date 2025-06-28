#include "pixel_fetcher.h"
#include "../gameboy.h"
#include "ppu.h"

PixelFetcher::PixelFetcher(PPU& ppu)
	: m_ppu{ppu}
	, m_firstFetchOnScanline{true}
	, m_currentMode{BACKGROUND}
	, m_currentStep{FETCH_TILE_NO}
	, m_stepCycle{}
	, m_xPosCounter{}
	, m_windowLineCounter{}
	, m_tileNumber{}
	, m_tileDataLow{}
	, m_tileDataHigh{}
	, m_tileAddress{}
{
}

void PixelFetcher::cycle()
{
	constexpr uint16 TILEMAP_SIZE{0x3FF};
	constexpr uint8 PIXELS_PER_TILE{8};
	constexpr uint8 TILES_PER_ROW{32};
	//when first entering this m_ppu.m_tCycleCounter is at 81, because oam_scan just finished

	/*
	if(delay > 0) //to delay by tot t-cycles if needed
	{
		--delay;
		return;
	}
	*/
	switch(m_currentMode)
	{
	case BACKGROUND:
	{
		switch(m_currentStep)
		{
		case FETCH_TILE_NO:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				const uint16 tileMapAddress{static_cast<uint16>(m_ppu.m_lcdc & 0b1000 ? 0x9C00 : 0x9800)}; //bit 3 sets tilemap for background

				const uint16 offset{static_cast<uint16>((((m_xPosCounter + m_ppu.m_scx) / PIXELS_PER_TILE) & 0x1F) +
														(TILES_PER_ROW * (((m_ppu.m_ly + m_ppu.m_scy) & 0xFF) / PIXELS_PER_TILE))
														& TILEMAP_SIZE)};

				m_tileNumber = m_ppu.m_gameboy.readMemory(tileMapAddress + offset);

				m_stepCycle = 0;
				m_currentStep = FETCH_TILE_DATA_LOW;
			}
			break;
		}
		case FETCH_TILE_DATA_LOW:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				if(m_ppu.m_lcdc & 0b10000) //bit 4 sets the fetching method for tile data
				{
					//8000 method
					m_tileAddress = 0x8000 + (m_tileNumber * 16)
									+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8));
					m_tileDataLow = m_ppu.m_gameboy.readMemory(m_tileAddress);
				}
				else
				{
					//8800 method
					__debugbreak();
					m_tileAddress = 0x9000 + (static_cast<int8>(m_tileNumber * 16))
									+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8));
					m_tileDataLow = m_ppu.m_gameboy.readMemory(m_tileAddress);
				}

				m_stepCycle = 0;
				m_currentStep = FETCH_TILE_DATA_HIGH;
			}
			break;
		}
		case FETCH_TILE_DATA_HIGH:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				m_tileDataHigh = m_ppu.m_gameboy.readMemory(m_tileAddress + 1);
				m_stepCycle = 0;

				if(m_firstFetchOnScanline)	
				{
					m_currentStep = FETCH_TILE_NO;
					m_firstFetchOnScanline = false;
				}
				else m_currentStep = PUSH_TO_FIFO;
			}
			break;
		}
		case PUSH_TO_FIFO:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				if(!m_ppu.m_pixelFifoBackground.empty()) //if its not empty repeat this step until it is
				{
					m_stepCycle = 1;
					break;
				}

				pushPixelsToFifo(BACKGROUND);
				m_stepCycle = 0;
				m_currentStep = FETCH_TILE_NO;
			}
			break;
		}
		}
	break;
	}
	case WINDOW:
	{

		break;
	}
	case SPRITE:
	{

		break;
	}
	}
}

void PixelFetcher::pushPixelsToFifo(Mode mode)
{
	switch(mode)
	{
	case BACKGROUND:
		for(int i{7}; i >= 0; --i)
		{
			PPU::Pixel pixel{};
			pixel.data = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
			pixel.xPosition = m_xPosCounter++; //store position, then increment

			m_ppu.m_pixelFifoBackground.push(pixel);
		}
		break;
	case WINDOW:
		//todo I think
		break;
	case SPRITE:
		//todo
		break;
	}
}

uint8 PixelFetcher::getXPosCounter() const
{
	return m_xPosCounter;
}

void PixelFetcher::clear()
{
	m_firstFetchOnScanline = true;
	m_currentMode = BACKGROUND;
	m_currentStep = FETCH_TILE_NO;
	m_stepCycle = 0;
	m_xPosCounter = 0;
	m_windowLineCounter = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
}

