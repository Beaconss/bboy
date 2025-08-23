#include "pixel_fetcher.h"
#include "ppu.h"

PixelFetcher<PPU::Sprite>::PixelFetcher(PPU& ppu)
	: m_ppu{ppu}
	, m_spriteBeingFetched{}
	, m_isFetchingWindow{}
	, m_firstFetchCompleted{}
	, m_backgroundCycleCounter{}
	, m_spriteCycleCounter{}
	, m_tileX{}
	, m_tilemap{}
	, m_tileNumber{}
	, m_tileDataLow{}
	, m_tileDataHigh{}
	, m_tileAddress{}
	, m_windowLineCounter{} 
	, m_wyLyCondition{}
{
	reset();
}

template <>
void PixelFetcher<PPU::Sprite>::reset()
{
	m_spriteBeingFetched = nullptr;
	m_firstFetchCompleted = false;
	m_isFetchingWindow = false;
	updateTilemap();
	m_backgroundCycleCounter = 0;
	m_spriteCycleCounter = 0;
	m_tileX = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
	m_windowLineCounter = -1; //start from -1 because at the first window scanline its 0, then 1, 2 etc
	m_wyLyCondition = false;
}

template <>
void PixelFetcher<PPU::Sprite>::resetEndScanline()
{
	m_firstFetchCompleted = false;
	m_isFetchingWindow = false;
	updateTilemap();
	m_backgroundCycleCounter = 0;
	m_spriteCycleCounter = 0;
	m_tileX = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
}

template <>
void PixelFetcher<PPU::Sprite>::cycle()
{
	constexpr uint16 TILEMAP_SIZE{0x3FF};
	constexpr uint8 PIXELS_PER_TILE{8};
	constexpr uint8 TILES_PER_ROW{32};
	if(m_ppu.m_wy == m_ppu.m_ly) m_wyLyCondition = true;
	
	checkForWindow();
	checkForSprite();
	
	if(!m_spriteBeingFetched)
	{
		if(m_backgroundCycleCounter < PUSH_TO_FIFO) ++m_backgroundCycleCounter;
		switch(m_backgroundCycleCounter)
		{
		case FETCH_TILE_NO:
		{
			const uint16 offset{m_isFetchingWindow ? static_cast<uint16>((((m_tileX) & 0x1F)
													  + (TILES_PER_ROW * (m_windowLineCounter / PIXELS_PER_TILE)))
													  & TILEMAP_SIZE)
													  :
													  static_cast<uint16>(((((m_tileX + (m_ppu.m_scx / PIXELS_PER_TILE)) & 0x1F)
													  + (TILES_PER_ROW * (((m_ppu.m_ly + m_ppu.m_scy) & 0xFF) / PIXELS_PER_TILE)))
													  & TILEMAP_SIZE))};

			m_tileNumber = m_ppu.m_bus.read(m_tilemap + offset, Bus::Component::PPU);
		}
		break;
		case FETCH_TILE_DATA_LOW:
		{
			if(m_ppu.m_lcdc & 0b10000) //bit 4 sets the fetching method for tile data
			{
				//8000 method
				m_tileAddress = m_isFetchingWindow ?
					0x8000 + (m_tileNumber * 16)
					+ (2 * (m_windowLineCounter % 8))
					:
					0x8000 + (m_tileNumber * 16)
					+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8));

				m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
			}
			else
			{
				//8800 method
				m_tileAddress = m_isFetchingWindow ?
					0x9000 + (static_cast<int8>(m_tileNumber) * 16)
					+ (2 * (m_windowLineCounter % 8))
					:
					0x9000 + (static_cast<int8>(m_tileNumber) * 16)
					+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) % 8));

				m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
			}
		}
		break;
		case FETCH_TILE_DATA_HIGH:
			m_tileDataHigh = m_ppu.m_bus.read(m_tileAddress + 1, Bus::Component::PPU);
			if(!m_firstFetchCompleted)
			{
				m_backgroundCycleCounter = 0;
				m_firstFetchCompleted = true;
			}
			break;
		case PUSH_TO_FIFO:
			if(!m_ppu.m_pixelFifoBackground.empty()) break; //if its not empty repeat this step until it is
			pushToBackgroundFifo();
			m_backgroundCycleCounter = 0;
			cycle(); //apparently the same cycle it pushes it starts the next fetch
			break;
		default: break;
		}
	}
	else
	{
		++m_spriteCycleCounter;
		switch(m_spriteCycleCounter)
		{
		case FETCH_TILE_NO:
		{
			constexpr uint8 TALL_SPRITE_MODE{0b100};
			if(m_ppu.m_lcdc & TALL_SPRITE_MODE)
			{
				m_tileNumber = ((m_ppu.m_ly - (m_spriteBeingFetched->yPosition - 16)) < 8) ?
					(m_spriteBeingFetched->tileNumber & 0xFE) //top tile
					:
					(m_spriteBeingFetched->tileNumber | 0b1); //bottom tile
			}
			else m_tileNumber = m_spriteBeingFetched->tileNumber;
		}
		break;
		case FETCH_TILE_DATA_LOW:
		{
			constexpr uint8 Y_FLIP_FLAG{0b100'0000};
			uint8 row = m_ppu.m_ly - (m_spriteBeingFetched->yPosition - 16);
			if(m_spriteBeingFetched->flags & Y_FLIP_FLAG)
			{
				constexpr uint8 TALL_SPRITE_MODE{0b100};
				if(m_ppu.m_lcdc & TALL_SPRITE_MODE)
				{
					m_tileNumber ^= 1; //switch tile
					row ^= 15;
				}
				else row ^= 7;
			}

			m_tileAddress = 0x8000 + (m_tileNumber * 16) + (2 * row);
			m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
		}
		break;
		case FETCH_TILE_DATA_HIGH:
			m_tileDataHigh = m_ppu.m_bus.read(m_tileAddress + 1, Bus::Component::PPU);
			break;
		case PUSH_TO_FIFO:
			pushToSpriteFifo();
			m_spriteCycleCounter = 0;
			cycle();
			break;
		default: break;
		}
	}
}

template <>
void PixelFetcher<PPU::Sprite>::updateTilemap()
{
	//which background tilemap is used is stored at bit 6 for window and bit 3 for background
	if(m_isFetchingWindow) m_tilemap = m_ppu.m_lcdc & 0b100'0000 ? 0x9C00 : 0x9800; 
	else m_tilemap = m_ppu.m_lcdc & 0b1000 ? 0x9C00 : 0x9800;
}

template <>
void PixelFetcher<PPU::Sprite>::pushToBackgroundFifo()
{
	PPU::Pixel pixel{};
	for(int i{7}; i >= 0; --i)
	{
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		m_ppu.m_pixelFifoBackground.push(pixel);
	}
	++m_tileX;
}

template <>
void PixelFetcher<PPU::Sprite>::pushToSpriteFifo()
{
	uint8 pixelsAlreadyInFifo{static_cast<uint8>(m_ppu.m_pixelFifoSprite.size())};
	uint8 pixelsOffScreenLeft{static_cast<uint8>(8 - (m_spriteBeingFetched->xPosition > 8 ? 8 : m_spriteBeingFetched->xPosition))}; //cap to 8
	uint8 pixelsOffScreenRight{static_cast<uint8>((SCREEN_WIDTH + 8) - m_spriteBeingFetched->xPosition)};
	constexpr uint8 X_FLIP_FLAG{0b10'0000};

	PPU::Pixel pixel{};
	if(m_spriteBeingFetched->flags & X_FLIP_FLAG)
	{
		for(int i{0}; i <= 7; ++i) //reverse rendering
		{
			if(i == pixelsOffScreenRight) break;
			constexpr uint8 PALETTE_FLAG{0b1'0000};
			constexpr uint8 BACKGROUND_PRIORITY_FLAG{0x80};
			if(pixelsOffScreenLeft == 0)
			{
				if(pixelsAlreadyInFifo == 0)
				{
					pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
					pixel.spritePalette = m_spriteBeingFetched->flags & PALETTE_FLAG;
					pixel.backgroundPriority = m_spriteBeingFetched->flags & BACKGROUND_PRIORITY_FLAG;
					m_ppu.m_pixelFifoSprite.push(pixel);
				}
				else --pixelsAlreadyInFifo;
			}
			else --pixelsOffScreenLeft;
		}
	}
	else
	{
		pixelsOffScreenRight ^= 7;
		for(int i{7}; i >= 0; --i) //normal rendering
		{
			if(i == pixelsOffScreenRight) break;
			constexpr uint8 PALETTE_FLAG{0b1'0000};
			constexpr uint8 BACKGROUND_PRIORITY_FLAG{0x80};
			if(pixelsOffScreenLeft == 0)
			{
				if(pixelsAlreadyInFifo == 0)
				{
					pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
					pixel.spritePalette = m_spriteBeingFetched->flags & PALETTE_FLAG;
					pixel.backgroundPriority = m_spriteBeingFetched->flags & BACKGROUND_PRIORITY_FLAG;
					m_ppu.m_pixelFifoSprite.push(pixel);
				}
				else --pixelsAlreadyInFifo;
			}
			else --pixelsOffScreenLeft;
		}
	}
	m_spriteBeingFetched = nullptr;
	m_ppu.m_spriteBuffer.pop_back();
}

template <>
void PixelFetcher<PPU::Sprite>::checkForWindow()
{	
	if((m_ppu.m_xPosition >= (m_ppu.m_wx - 7))
		&& !m_isFetchingWindow
		&& m_wyLyCondition
		&& m_ppu.m_lcdc & 0b100000)
	{
		++m_windowLineCounter;
		m_isFetchingWindow = true;
		updateTilemap();
		m_backgroundCycleCounter = 0;
		m_tileX = 0;
		m_ppu.clearBackgroundFifo();
	}
}

template <>
void PixelFetcher<PPU::Sprite>::checkForSprite()
{
	//since the sprite buffer is sorted in descending order the last element is the one with the lowest x
	if(!m_ppu.m_spriteBuffer.empty() && !m_spriteBeingFetched && m_ppu.m_spriteBuffer.back().xPosition <= (m_ppu.m_xPosition + 8))
	{
		//once the pipeline finishes the sprite gets popped
		m_spriteBeingFetched = &m_ppu.m_spriteBuffer.back();
		m_backgroundCycleCounter = 0;
	}
}