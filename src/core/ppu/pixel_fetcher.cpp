#include "pixel_fetcher.h"
#include "core/mmu.h"
#include "ppu.h"

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

void PixelFetcher::cycle()
{
  checkForWindow();
  checkForSprite();

  if(m_spriteFetchDelay > 0)
  {
    if(--m_spriteFetchDelay == 0)
    {
      const Sprite sprite{m_ppu.m_spriteBuffer.back()};

      constexpr uint8 tallSpriteMode{0b100};
      const bool tallSprite{static_cast<bool>(m_ppu.m_lcdc & tallSpriteMode)};

      uint8 row = (m_ppu.m_ly - sprite.yPosition) & (tallSprite ? 15 : 7);
      constexpr uint8 yFlipFlag{0b100'0000};
      if(sprite.flags & yFlipFlag) row ^= tallSprite ? 15 : 7;

      const uint16 m_tileNumber = tallSprite ? sprite.tileNumber & 0xFE : sprite.tileNumber;

      const uint16 tileAddr = 0x8000 + (m_tileNumber * 16) + (2 * row);
      m_tileDataLow = m_ppu.m_bus.read(tileAddr, MMU::Component::ppu);
      m_tileDataHigh = m_ppu.m_bus.read(tileAddr + 1, MMU::Component::ppu);

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

      const uint16 offset{
        m_isFetchingWindow
          ? static_cast<uint16>((m_tileX + (tilesPerRow * (m_windowLineCounter / pixelsPerTile))) & tilemapSize)
          : static_cast<uint16>(((((m_tileX + (m_ppu.m_scx / pixelsPerTile)) & 0x1F) +
                                  (tilesPerRow * ((m_ppu.m_ly + m_ppu.m_scy) / pixelsPerTile))) &
                                 tilemapSize))};

      m_tileNumber = m_ppu.m_bus.read(m_tilemap + offset, MMU::Component::ppu);
    }
    break;
    case fetchTileDataLow:
    {
      constexpr uint8 fetchMetod{0b1'0000};
      if(m_ppu.m_lcdc & fetchMetod)
      {
        m_tileAddress = m_isFetchingWindow ? 0x8000 + (m_tileNumber * 16) + (2 * (m_windowLineCounter & 7))
                                           : 0x8000 + (m_tileNumber * 16) + (2 * ((m_ppu.m_ly + m_ppu.m_scy) & 7));
        m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, MMU::Component::ppu);
      }
      else
      {
        m_tileAddress = m_isFetchingWindow
                          ? 0x9000 + (static_cast<int8>(m_tileNumber) * 16) + (2 * (m_windowLineCounter & 7))
                          : 0x9000 + (static_cast<int8>(m_tileNumber) * 16) + (2 * ((m_ppu.m_ly + m_ppu.m_scy) & 7));
        m_tileDataLow = m_ppu.m_bus.read(m_tileAddress, MMU::Component::ppu);
      }
    }
    break;
    case fetchTileDataHigh:
      m_tileDataHigh = m_ppu.m_bus.read(m_tileAddress + 1, MMU::Component::ppu);
      if(!m_firstFetchCompleted)
      {
        m_backgroundCycleCounter = 0;
        m_firstFetchCompleted = true;
      }
      break;
    case pushToFifo:
    {
      if(!m_ppu.m_pixelFifoBackground.empty()) break;

      pushToBackgroundFifo();
      m_backgroundCycleCounter = 0;
      break;
    }
    default: break;
    }
  }
}

void PixelFetcher::updateTilemap()
{
  constexpr uint8 windowTilemap{0b100'0000};
  constexpr uint8 backgroundTilemap{0b1000};
  if(m_isFetchingWindow) m_tilemap = m_ppu.m_lcdc & windowTilemap ? 0x9C00 : 0x9800;
  else m_tilemap = m_ppu.m_lcdc & backgroundTilemap ? 0x9C00 : 0x9800;
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

void PixelFetcher::pushToSpriteFifo(const Sprite sprite)
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
    else if(const int realIndex{i ^ 7};
            (realIndex < m_ppu.m_pixelFifoSprite.size()) &&
            (m_ppu.m_pixelFifoSprite[realIndex].colorIndex == 0 || m_ppu.m_pixelFifoSprite[realIndex].backgroundPriority))
    {
      m_ppu.m_pixelFifoSprite[realIndex] = pixel;
      --pixelsAlreadyInFifo;
    }
    else --pixelsAlreadyInFifo;
  }

  m_ppu.m_spriteBuffer.pop_back();
}

void PixelFetcher::checkForWindow()
{
  if(m_ppu.m_wy == m_ppu.m_ly) m_wyLyCondition = true;
  if((m_ppu.m_xPosition >= (m_ppu.m_wx - 7)) && m_wyLyCondition && m_ppu.m_lcdc & 0b100000 && !m_isFetchingWindow)
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
  if(m_ppu.m_lcdc & spriteEnable && m_spriteFetchDelay == 0 && !m_ppu.m_spriteBuffer.empty() &&
     m_ppu.m_spriteBuffer.back().xPosition <= (m_ppu.m_xPosition + 8))
  {
    //once it gets fetched the sprite gets popped
    constexpr int maxSpriteFetchDelay{8};
    m_spriteFetchDelay = maxSpriteFetchDelay;
    m_backgroundCycleCounter = 0;
  }
}
