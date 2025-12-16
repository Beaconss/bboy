#pragma once
#include "core/ppu/pixel_fetcher.h"
#include "type_alias.h"
#include <array>
#include <deque>
#include <string_view>
#include <vector>

//this is here because it needs to be forward declared(to avoid template magic)
struct Sprite
{
  uint8 yPosition{0xFF}; //byte 0(0xFF is offscreen)
  uint8 xPosition{};     //byte 1
  uint8 tileNumber{};    //byte 2
  uint8 flags{};         //byte 3
  //bit 7: background priority
  //bit 6: y-flip
  //bit 5: x-flip
  //bit 4: palette
  //other bits are cgb only
};

class MMU;
class PPU
{
public:
  enum class PaletteIndex
  {
    grey,
    green,
    blue,
    max
  };

  PPU(MMU& bus, uint16* lcdTexturePtr, PaletteIndex palette = PaletteIndex::grey);

  enum Index
  {
    lcdc,
    stat,
    scy,
    scx,
    ly,
    lyc,
    bgp,
    obp0,
    obp1,
    wy,
    wx,
  };

  enum Mode
  {
    hBlank,
    vBlank,
    oamScan,
    drawing,
  };

  static PaletteIndex stringToPaletteIndex(std::string_view paletteString);

  void reset();
  void mCycle();

  PPU::Mode getMode() const;

  uint8 read(const Index index) const;
  void write(const Index index, const uint8 value);

  static constexpr int lcdWidth{160};
  static constexpr int lcdHeight{144};

private:
  friend class PixelFetcher;

  struct Pixel
  {
    uint8 colorIndex{};
    bool spritePalette{}; //obp1 if true, obp0 if false
    bool backgroundPriority{};
    uint8 paletteValue{};
  };

  struct StatInterrupt
  {
    enum Source
    {
      hBLank,
      vBlank,
      oamScan,
      lyCompare,
    };
    std::array<bool, 4> sources{};
    bool previousResult{false};
  };
  void handleStatInterrupt();
  void setStatModeSources();

  void updateMode(const Mode mode);
  void updateCoincidenceFlag(bool set = true);

  void oamScanCycle();
  void tryAddSpriteToBuffer(const Sprite sprite);

  void drawingCycle();
  void tryToPushPixel();
  bool shouldPushSpritePixel() const;
  void clearFifos();

  void hBlankCycle();
  void vBlankCycle();

  void requestStatInterrupt() const;
  void requestVBlankInterrupt() const;

  static constexpr std::array<std::array<uint16, 4>, static_cast<int>(PaletteIndex::max)> palettes //in rgb565
  {
    {{0xFFFF,0xC618,0x4208,0x0,},
    {0xdfb9, 0x8dee, 0x334a, 0x08c4},
    {0xFFFF, 0x8c5f, 0x0011, 0x0}}
  };

  static constexpr uint8 enableBit{0x80};
  static constexpr int oamScanEndCycle{20};
  static constexpr uint16 oamMemoryStart{0xFE00};
  static constexpr int scanlineEndCycle{114};

  MMU& m_bus;
  PixelFetcher m_fetcher;
  StatInterrupt m_statInterrupt;
  Mode m_mode;
  std::array<uint16, 4> m_palette;

  uint8 m_cycleCounter;
  bool m_vblankInterruptNextCycle;
  bool m_reEnabling;
  uint8 m_reEnableDelay;

  uint16* m_lcdBuffer;
  uint8 m_xPosition; //x position of the pixel to output
  uint8 m_pixelsToDiscard;

  std::vector<Sprite> m_spriteBuffer;
  uint16 m_spriteAddress;

  std::deque<Pixel> m_pixelFifoBackground;
  std::deque<Pixel> m_pixelFifoSprite;

  uint8 m_lcdc; //LCD control
  uint8 m_stat; //LDC status
  uint8 m_scy;  //viewport y position
  uint8 m_scx;  //viewport x position
  uint8 m_ly;   //LCD y coordinate (current scanline number)
  uint8 m_lyc;  //LY compare
  uint8 m_bgp;  //background palette data
  uint8 m_oldBgp;
  uint8 m_obp0; //sprite palette data 0
  uint8 m_obp1; //sprite palette data 1
  uint8 m_wy;   //window y position
  uint8 m_wx;   //window x position plus 7
};
