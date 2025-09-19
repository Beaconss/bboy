#include <core/ppu/ppu.h>
#include <core/ppu/pixel_fetcher.h>

template<>
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

template<typename T>
void PixelFetcher<T>::reset()
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

template<>
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

template<>
void PixelFetcher<PPU::Sprite>::cycle()
{
	constexpr uint16 TILEMAP_SIZE{0x3FF};
	constexpr uint8 PIXELS_PER_TILE{8};
	constexpr uint8 TILES_PER_ROW{32};
	
	checkForWindow();
	checkForSprite();

	if(!m_spriteBeingFetched)
	{
		if(m_backgroundCycleCounter < PUSH_TO_FIFO) ++m_backgroundCycleCounter;
		switch(m_backgroundCycleCounter)
		{
		case FETCH_TILE_NO:
		{
			const uint16 offset{m_isFetchingWindow ? 
							static_cast<uint16>((m_tileX
							+ (TILES_PER_ROW * (m_windowLineCounter / PIXELS_PER_TILE)))
							& TILEMAP_SIZE)
							:
							static_cast<uint16>(((((m_tileX + (m_ppu.m_scx / PIXELS_PER_TILE)) & 0x1F)
							+ (TILES_PER_ROW * ((m_ppu.m_ly + m_ppu.m_scy) / PIXELS_PER_TILE)))
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
							+ (2 * (m_windowLineCounter & 7))
							:
							0x8000 + (m_tileNumber * 16)
							+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) & 7));
				m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, Bus::Component::PPU);
			}
			else
			{
				//8800 method
				m_tileAddress = m_isFetchingWindow ?
							0x9000 + (static_cast<int8>(m_tileNumber) * 16)
							+ (2 * (m_windowLineCounter & 7))
							:
							0x9000 + (static_cast<int8>(m_tileNumber) * 16)
							+ (2 * ((m_ppu.m_ly + m_ppu.m_scy) & 7));
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
			break;
		default: break;
		}
	}
	else
	{
		++m_spriteCycleCounter;
		switch(m_spriteCycleCounter)
		{
		case FETCH_TILE_DATA_LOW:
		{
			constexpr uint8 TALL_SPRITE_MODE{0b100};
			bool tallSprite{static_cast<bool>(m_ppu.m_lcdc & TALL_SPRITE_MODE)};
			
			m_tileNumber = tallSprite ? m_spriteBeingFetched->tileNumber & 0xFE : m_spriteBeingFetched->tileNumber;
			
			uint8 row = (m_ppu.m_ly - m_spriteBeingFetched->yPosition) & (tallSprite ? 15 : 7);
			constexpr uint8 Y_FLIP_FLAG{0b100'0000};
			if(m_spriteBeingFetched->flags & Y_FLIP_FLAG) row ^= tallSprite ? 15 : 7;
			
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
			checkForSprite();
			break;
		default: break;
		}
	}
}

template<typename T>
void PixelFetcher<T>::updateTilemap()
{
	//which background tilemap is used is determined at bit 6 for window and bit 3 for background
	if(m_isFetchingWindow) m_tilemap = m_ppu.m_lcdc & 0b100'0000 ? 0x9C00 : 0x9800; 
	else m_tilemap = m_ppu.m_lcdc & 0b1000 ? 0x9C00 : 0x9800;
}

template<typename T>
void PixelFetcher<T>::pushToBackgroundFifo()
{
	PPU::Pixel pixel{};
	for(int i{7}; i >= 0; --i)
	{
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		m_ppu.m_pixelFifoBackground.push_back(pixel);
	}
	++m_tileX;
}

template<typename T>
void PixelFetcher<T>::pushToSpriteFifo()
{
	constexpr uint8 X_FLIP_FLAG{0b10'0000};
	if(m_spriteBeingFetched->flags & X_FLIP_FLAG)
	{
		m_tileDataLow = flipByte(m_tileDataLow);
		m_tileDataHigh = flipByte(m_tileDataHigh);
	}

	int pixelsOffScreenLeft{8 - (m_spriteBeingFetched->xPosition > 8 ? 8 : m_spriteBeingFetched->xPosition)}; //cap to 8
	int pixelsAlreadyInFifo{static_cast<uint8>(m_ppu.m_pixelFifoSprite.size())};
	PPU::Pixel pixel{};
	for(int i{7}; i >= 0; --i) //normal rendering
	{
		if(pixelsOffScreenLeft)
		{
			--pixelsOffScreenLeft;
			continue;
		}
		
		constexpr uint8 PALETTE_FLAG{0b1'0000};
		constexpr uint8 BACKGROUND_PRIORITY_FLAG{0x80};
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		pixel.spritePalette = m_spriteBeingFetched->flags & PALETTE_FLAG;
		pixel.backgroundPriority = m_spriteBeingFetched->flags & BACKGROUND_PRIORITY_FLAG;

		if(pixelsAlreadyInFifo == 0) m_ppu.m_pixelFifoSprite.push_back(pixel);
		else if(const int realIndex{i ^ 7}; (realIndex < m_ppu.m_pixelFifoSprite.size())
				&& (m_ppu.m_pixelFifoSprite[realIndex].colorIndex == 0 || m_ppu.m_pixelFifoSprite[realIndex].backgroundPriority))
		{
			m_ppu.m_pixelFifoSprite[realIndex] = pixel;
			--pixelsAlreadyInFifo;
		}
		else --pixelsAlreadyInFifo;
	}
	m_spriteBeingFetched = nullptr;
	m_ppu.m_spriteBuffer.pop_back();
}

template<typename T>
uint8 PixelFetcher<T>::flipByte(uint8 byte) const
{
	byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
	byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
	byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
	return byte;
}

template<typename T>
void PixelFetcher<T>::checkForWindow()
{
	if(m_ppu.m_wy == m_ppu.m_ly) m_wyLyCondition = true;
	if((m_ppu.m_xPosition >= (m_ppu.m_wx - 7))
		&& m_wyLyCondition
		&& m_ppu.m_lcdc & 0b100000
		&& !m_isFetchingWindow)
	{
		++m_windowLineCounter;
		m_isFetchingWindow = true;
		updateTilemap();
		m_backgroundCycleCounter = 0;
		m_tileX = 0;
		m_ppu.clearFifos();
		m_ppu.m_pixelsToDiscard = std::min(m_ppu.m_wx - 7, 0) * -1;
	}
}

template<typename T>
void PixelFetcher<T>::checkForSprite()
{
	//since the sprite buffer is sorted in descending order the last element is the one with the lowest x
	if(m_ppu.m_lcdc & 0b10 && !m_ppu.m_spriteBuffer.empty() && !m_spriteBeingFetched && m_ppu.m_spriteBuffer.back().xPosition <= (m_ppu.m_xPosition + 8))
	{
		//once the pipeline finishes the sprite gets popped
		m_spriteBeingFetched = &m_ppu.m_spriteBuffer.back();
		m_backgroundCycleCounter = 0;
	}
}