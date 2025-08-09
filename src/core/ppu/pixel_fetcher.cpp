#include "pixel_fetcher.h"
#include "ppu.h"

PixelFetcher::PixelFetcher(PPU& ppu)
	: m_ppu{ppu}
	, m_firstFetchCompleted{false}
	, m_wyLyCondition{false}
	, m_mode{BACKGROUND}
	, m_step{FETCH_TILE_NO}
	, m_stepCycle{}
	, m_xPositionCounter{}
	, m_backGroundTileMap{}
	, m_windowLineCounter{-1} //start from -1 because at the first window scanline its 0, then 1, 2 etc
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
	if(m_ppu.m_wy == m_ppu.m_ly) m_wyLyCondition = true;

	switch(m_mode)
	{
	case BACKGROUND:
	case WINDOW:
	{
		switch(m_step)
		{
		case FETCH_TILE_NO:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				const uint16 offset{m_mode == BACKGROUND ? static_cast<uint16>(((((m_xPositionCounter + (m_ppu.m_scx / PIXELS_PER_TILE)) & 0x1F)
														 + (TILES_PER_ROW * (((m_ppu.m_ly + m_ppu.m_scy) & 0xFF) / PIXELS_PER_TILE)))
														 & TILEMAP_SIZE)) 
														 : 
														 static_cast<uint16>((((m_xPositionCounter) & 0x1F)
														 + (TILES_PER_ROW * (m_windowLineCounter / PIXELS_PER_TILE)))
														 & TILEMAP_SIZE)};

				m_tileNumber = m_ppu.m_bus.read(m_backGroundTileMap + offset, Bus::Component::PPU);

				m_stepCycle = 0;
				m_step = FETCH_TILE_DATA_LOW;
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
					m_tileAddress = m_mode == BACKGROUND ? 
									0x8000 + (m_tileNumber * 16)
									+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8))
									: 
									0x8000 + (m_tileNumber * 16)
									+ (2 * (m_windowLineCounter % 8));
					m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
				}
				else
				{
					//8800 method
					m_tileAddress = m_mode == BACKGROUND ? 
									0x9000 + (static_cast<int8>(m_tileNumber) * 16)
									+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8))
									: 
									0x9000 + (static_cast<int8>(m_tileNumber) * 16)
									+ (2 * (m_windowLineCounter % 8));;
					m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
				}

				m_stepCycle = 0;
				m_step = FETCH_TILE_DATA_HIGH;
			}
			break;
		}
		case FETCH_TILE_DATA_HIGH:
		{
			++m_stepCycle;
			if(m_stepCycle == 2)
			{
				m_tileDataHigh = m_ppu.m_bus.read(m_tileAddress + 1, Bus::Component::PPU);

				if(!m_firstFetchCompleted)	
				{
					m_step = FETCH_TILE_NO;
					m_firstFetchCompleted = true;
				}
				else m_step = PUSH_TO_FIFO;
				m_stepCycle = 0;
			}
			break;
		}
		case PUSH_TO_FIFO:
		{
			if(!m_ppu.m_pixelFifoBackground.empty()) break; //if its not empty repeat this step until it is
			pushToBackgroundFifo();
			m_step = FETCH_TILE_NO;
			break;
		}
		}
	break;
	}
	case SPRITE:
	{

		break;
	}
	}
}

void PixelFetcher::update(std::optional<Mode> mode)
{
	if(mode) m_mode = mode.value();

	//which background tilemap is used is stored at bit 6 for window and bit 3 for background
	if(m_mode == WINDOW) m_backGroundTileMap = m_ppu.m_lcdc & 0b100'0000 ? 0x9C00 : 0x9800;
	else m_backGroundTileMap = m_ppu.m_lcdc & 0b1000 ? 0x9C00 : 0x9800;
}

void PixelFetcher::pushToBackgroundFifo()
{
	for(int i{7}; i >= 0; --i)
	{
		PPU::Pixel pixel{};
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		m_ppu.m_pixelFifoBackground.push(pixel);
	}
	++m_xPositionCounter;
}

void PixelFetcher::checkWindowReached()
{	
	if(m_mode != WINDOW
		&& m_ppu.m_lcdc & 0b100000
		&& m_wyLyCondition
		&& m_ppu.m_xPosition >= (m_ppu.m_wx - 7))
	{
		++m_windowLineCounter;
		update(WINDOW);
		m_step = FETCH_TILE_NO;
		m_stepCycle = 0;
		m_xPositionCounter = 0;
		m_ppu.clearBackgroundFifo();
	}
}

void PixelFetcher::clearEndScanline()
{
	m_firstFetchCompleted = false;
	update(BACKGROUND);
	m_step = FETCH_TILE_NO;
	m_stepCycle = 0;
	m_xPositionCounter = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
}

void PixelFetcher::clearEndFrame()
{
	m_windowLineCounter = -1;
	m_wyLyCondition = false;
}

