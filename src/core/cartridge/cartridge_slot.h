#pragma once
#include "type_alias.h"
#include <filesystem>

class Cartridge;
class CartridgeSlot
{
public:
  CartridgeSlot();
  ~CartridgeSlot();

  const std::string& getCartridgeName() const;

  void reset();
  void loadCartridge(const std::filesystem::path& filePath);
  void reloadCartridge();
  bool hasCartridge() const;
  void clockFrame();

  uint8 readRom(const uint16 addr) const;
  void writeRom(const uint16 addr, const uint8 value);
  uint8 readRam(const uint16 addr) const;
  void writeRam(const uint16 addr, const uint8 value);

private:
  Cartridge* m_cartridge;
  std::filesystem::path m_cartridgePath;
  std::string m_cartridgeName;

  bool m_cartridgeHasClock;
  uint8 m_frameCounter;
};
