#include <core/ppu/ppu.h>
#include <core/ppu/pixel_fetcher.h>

PixelFetcher::PixelFetcher(PPU& ppu)
	: m_ppu{ppu}
	, m_isFetchingWindow{}
	, m_firstFetchCompleted{}
	, m_backgroundCycleCounter{}
	, m_spriteFetchDelay{}
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

void PixelFetcher::reset()
{
	m_firstFetchCompleted = false;
	m_isFetchingWindow = false;
	updateTilemap();
	m_backgroundCycleCounter = 0;
	m_spriteFetchDelay = 0;
	m_tileX = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
	m_windowLineCounter = -1; //start from -1 because at the first window scanline its 0, then 1, 2 etc
	m_wyLyCondition = false;
}

void PixelFetcher::resetEndScanline()
{
	m_firstFetchCompleted = false;
	m_isFetchingWindow = false;
	updateTilemap();
	m_backgroundCycleCounter = 0;
	m_spriteFetchDelay = 0;
	m_tileX = 0;
	m_tileNumber = 0;
	m_tileDataLow = 0;
	m_tileDataHigh = 0;
	m_tileAddress = 0;
}

template<>
void PixelFetcher::pushToSpriteFifo(const PPU::Sprite sprite) //this has to be here because since it gets used in cycle it needs to be instantiated before that
{
	if(constexpr uint8 xFlipFlag{0b10'0000}; sprite.flags & xFlipFlag)
	{		
		auto flipByte{[](uint8 byte) 
				{
					byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
					byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
					byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
					return byte;
				}};
		m_tileDataLow = flipByte(m_tileDataLow);
		m_tileDataHigh = flipByte(m_tileDataHigh);
	}

	int pixelsOffScreenLeft{8 - (sprite.xPosition > 8 ? 8 : sprite.xPosition)}; //cap to 8
	int pixelsAlreadyInFifo{static_cast<uint8>(m_ppu.m_pixelFifoSprite.size())};
	PPU::Pixel pixel{};
	for(int i{7}; i >= 0; --i)
	{
		if(pixelsOffScreenLeft > 0)
		{
			--pixelsOffScreenLeft;
			continue;
		}

		constexpr uint8 paletteFlag{0b1'0000};
		constexpr uint8 backgroundPriorityFlag{0x80};
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		pixel.spritePalette = sprite.flags & paletteFlag;
		pixel.backgroundPriority = sprite.flags & backgroundPriorityFlag;

		if(pixelsAlreadyInFifo == 0) m_ppu.m_pixelFifoSprite.push_back(pixel);
		else if(const int realIndex{i ^ 7}; (realIndex < m_ppu.m_pixelFifoSprite.size())
			&& (m_ppu.m_pixelFifoSprite[realIndex].colorIndex == 0 || m_ppu.m_pixelFifoSprite[realIndex].backgroundPriority))
		{
			m_ppu.m_pixelFifoSprite[realIndex] = pixel;
			--pixelsAlreadyInFifo;
		}
		else --pixelsAlreadyInFifo;
	}

	m_ppu.m_spriteBuffer.pop_back();
}

void PixelFetcher::cycle()
{
	//drawingLog << "fetcher state:\n";
	checkForWindow();
	checkForSprite();

	if(m_spriteFetchDelay > 0)
	{
		if(--m_spriteFetchDelay == 0)
		{
			const PPU::Sprite sprite{m_ppu.m_spriteBuffer.back()};

			constexpr uint8 tallSpriteMode{0b100};
			const bool tallSprite{static_cast<bool>(m_ppu.m_lcdc & tallSpriteMode)};

			uint8 row = (m_ppu.m_ly - sprite.yPosition) & (tallSprite ? 15 : 7);
			constexpr uint8 yFlipFlag{0b100'0000};
			if(sprite.flags & yFlipFlag) row ^= tallSprite ? 15 : 7;
			
			const uint16 m_tileNumber = tallSprite ? sprite.tileNumber & 0xFE : sprite.tileNumber;

			const uint16 tileAddr = 0x8000 + (m_tileNumber * 16) + (2 * row);
			m_tileDataLow = m_ppu.m_bus.read(tileAddr, Bus::Component::PPU);
			m_tileDataHigh = m_ppu.m_bus.read(tileAddr + 1, Bus::Component::PPU);

			pushToSpriteFifo(sprite);
			checkForSprite();
		}
	}
	else
	{
		if(m_backgroundCycleCounter < pushToFifo) ++m_backgroundCycleCounter;
		switch(m_backgroundCycleCounter)
		{
		case fetchTileNo:
		{
			constexpr uint8 tilesPerRow{32};
			constexpr uint8 pixelsPerTile{8};
			constexpr uint16 tilemapSize{0x3FF};

			const uint16 offset{m_isFetchingWindow ?
							static_cast<uint16>((m_tileX
							+ (tilesPerRow * (m_windowLineCounter / pixelsPerTile)))
							& tilemapSize)
							:
							static_cast<uint16>(((((m_tileX + (m_ppu.m_scx / pixelsPerTile)) & 0x1F)
							+ (tilesPerRow * ((m_ppu.m_ly + m_ppu.m_scy) / pixelsPerTile)))
							& tilemapSize))};

			m_tileNumber = m_ppu.m_bus.read(m_tilemap + offset, Bus::Component::PPU);
			//drawingLog << "	fetched tile number\n";
		}
		break;
		case fetchTileDataLow:
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
			//drawingLog << "	fetched tile data low\n";
		}
		break;
		case fetchTileDataHigh:
			m_tileDataHigh = m_ppu.m_bus.read(m_tileAddress + 1, Bus::Component::PPU);
			if(!m_firstFetchCompleted)
			{
				m_backgroundCycleCounter = 0;
				m_firstFetchCompleted = true;
			}
			//drawingLog << "	fetched tile data high\n";
			break;
		case pushToFifo:
		{
			if(!m_ppu.m_pixelFifoBackground.empty()) //if its not empty repeat this step until it is
			{
				//drawingLog << "	waiting for fifo to be empty\n";
				break;
			}
			pushToBackgroundFifo();
			m_backgroundCycleCounter = 0;
			//drawingLog << "	pixels pushed to background fifo\n";
			break;
		}
		default: 
			//drawingLog << "	sleeping\n";
			break;
		}
	}
}

void PixelFetcher::updateTilemap()
{
	//which background tilemap is used is determined at bit 6 for window and bit 3 for background
	if(m_isFetchingWindow) m_tilemap = m_ppu.m_lcdc & 0b100'0000 ? 0x9C00 : 0x9800; 
	else m_tilemap = m_ppu.m_lcdc & 0b1000 ? 0x9C00 : 0x9800;
}

void PixelFetcher::pushToBackgroundFifo()
{
	PPU::Pixel pixel{};
	for(int i{7}; i >= 0; --i)
	{
		pixel.colorIndex = static_cast<uint8>(((m_tileDataLow >> i) & 0b1) | (((m_tileDataHigh >> i) & 0b1) << 1));
		m_ppu.m_pixelFifoBackground.push_back(pixel);
	}
	++m_tileX;
}

void PixelFetcher::checkForWindow()
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

void PixelFetcher::checkForSprite()
{
	//since the sprite buffer is sorted in descending order the last element is the one with the lowest x
	constexpr int spriteEnable{0b10};
	if(m_ppu.m_lcdc & spriteEnable && m_spriteFetchDelay == 0 && !m_ppu.m_spriteBuffer.empty() && m_ppu.m_spriteBuffer.back().xPosition <= (m_ppu.m_xPosition + 8))
	{
		//once it gets fetched the sprite gets popped
		constexpr int maxSpriteFetchDelay{8};
		m_spriteFetchDelay = maxSpriteFetchDelay;
		m_backgroundCycleCounter = 0;
	}
}