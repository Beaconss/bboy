#include "core/gameboy.h"
#include "config.h"
#include "platform.h"

Gameboy::Gameboy()
  : m_bus{*this}
  , m_cpu{m_bus}
  , m_ppu{m_bus, Platform::getInstance().getLcdTexturePtr(), PPU::stringToPaletteIndex(Config::getInstance().getPalette())}
  , m_apu{m_bus, Config::getInstance().getVolume()}
  , m_timers{m_bus}
  , m_input{}
  , m_currentCycle{}
{
}

void Gameboy::reset()
{
  m_bus.reset();
  m_cpu.reset();
  m_ppu.reset();
  m_apu.reset();
  m_timers.reset();
  m_currentCycle = 1;
}

Gameboy::~Gameboy()
{
  reset();
}

void Gameboy::frame()
{
  static constexpr int mCyclePerFrame{17556};
  for(; m_currentCycle <= mCyclePerFrame; ++m_currentCycle) mCycle();
  m_currentCycle = 1;
  m_bus.getCartridgeSlot().clockFrame();
  m_apu.unlockThread();
}

void Gameboy::mCycle()
{
  m_cpu.mCycle();
  m_bus.handleDmaTransfer();
  m_timers.mCycle();
  m_ppu.mCycle();
}

void Gameboy::openRom(const std::filesystem::path& filePath)
{
  reset();
  m_bus.getCartridgeSlot().loadCartridge(filePath);
}

void Gameboy::hardReset()
{
  reset();
  m_bus.getCartridgeSlot().reloadCartridge();
}

std::string Gameboy::getRomName()
{
  return m_bus.getCartridgeSlot().getCartridgeName();
}

bool Gameboy::hasRom()
{
  return m_bus.getCartridgeSlot().hasCartridge();
}

uint16 Gameboy::currentCycle() const
{
  return m_currentCycle;
}
