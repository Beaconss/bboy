#pragma once
#include "core/apu/apu.h"
#include "core/cpu.h"
#include "core/input.h"
#include "core/mmu.h"
#include "core/ppu/ppu.h"
#include "core/timers.h"
#include "type_alias.h"

class Gameboy
{
public:
  Gameboy();
  ~Gameboy();

  void reset();
  void frame();

  void openRom(const std::filesystem::path& filePath);
  void hardReset();
  std::string getRomName();
  bool hasRom();
  uint16 currentCycle() const;

private:
  friend class MMU;
  void mCycle();

  MMU m_bus;
  CPU m_cpu;
  PPU m_ppu;
  APU m_apu;

  Timers m_timers;
  Input m_input;

  uint16 m_currentCycle;
};
