#pragma once
#include "type_alias.h"
#include <array>
#include <filesystem>
#include <vector>

class Cartridge
{
public:
  Cartridge(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false);
  virtual ~Cartridge() = default;

  void save(const std::filesystem::path& path);
  void loadSave(const std::filesystem::path& path);

  virtual uint8 readRom(const uint16 addr);
  virtual void writeRom(const uint16 addr, const uint8 value);
  virtual uint8 readRam(const uint16 addr);
  virtual void writeRam(const uint16 addr, const uint8 value);

protected:
  static constexpr uint16 kb2{0x800};
  static constexpr uint16 kb8{0x2000};
  static constexpr uint16 kb16{0x4000};
  static constexpr uint16 enableRamEnd{0x1FFF};

  std::vector<uint8> m_rom;
  std::vector<uint8> m_ram;
  bool m_hasRam;
  bool m_hasBattery;
  uint16 m_romBanks;
  uint16 m_ramBanks;

  bool m_externalRamEnabled;
  uint16 m_romBankIndex;
  uint8 m_ramBankIndex;

private:
  bool loadRom(const std::filesystem::path& path);
};

class CartridgeMbc1 : public Cartridge
{
public:
  CartridgeMbc1(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false);

  uint8 readRom(const uint16 addr) override final;
  void writeRom(const uint16 addr, const uint8 value) override final;
  uint8 readRam(const uint16 addr) override final;
  void writeRam(const uint16 addr, const uint8 value) override final;

private:
  uint32 m_ramSize;
  uint16 m_romBankIndexMask;
  bool m_modeFlag;
};

class CartridgeMbc3 : public Cartridge
{
public:
  CartridgeMbc3(const std::filesystem::path& path, bool hasRtc = false, bool hasBattery = false, bool hasClock = false);

  void rtcCycle();

  uint8 readRom(const uint16 addr) override final;
  void writeRom(const uint16 addr, const uint8 value) override final;
  uint8 readRam(const uint16 addr) override final;
  void writeRam(const uint16 addr, const uint8 value) override final;

private:
  enum RtcRegister
  {
    seconds,
    minutes,
    hours,
    daysLow,
    daysHigh,
    maxRtcRegister,
  };

  static constexpr int rtcRegisterOffset{8};
  static constexpr std::array<uint8, 5> rtcRegistersBitmasks //& bitmasks
    {0b0011'1111, 0b0011'1111, 0b0001'1111, 0b1111'1111, 0b1100'0001};

  bool m_hasRtc;

  RtcRegister m_mappedRtcRegister;
  std::array<uint8, 5> m_rtcRegisters;
  std::array<uint8, 5> m_rtcRegistersCopy;
  bool m_lastWriteZero;
};

class CartridgeMbc5 : public Cartridge
{
public:
  CartridgeMbc5(const std::filesystem::path& path, bool hasRam = false, bool hasBattery = false, bool hasRumble = false);

  uint8 readRom(const uint16 addr) override final;
  void writeRom(const uint16 addr, const uint8 value) override final;
  uint8 readRam(const uint16 addr) override final;
  void writeRam(const uint16 addr, const uint8 value) override final;

private:
  bool m_hasRumble;
};
