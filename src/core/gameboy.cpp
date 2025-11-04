#include "core/gameboy.h"
#include "hardware_registers.h"

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{std::make_unique<PPU>(m_bus)}
	, m_apu{m_bus}
	, m_timers{m_bus}
	, m_input{}
	, m_currentCycle{}
{
}

void Gameboy::reset() 
{
	m_bus.reset();
	m_cpu.reset();
	m_ppu->reset();
	m_apu.reset();
	m_timers.reset();
	m_currentCycle = 1;
}

Gameboy::~Gameboy()
{
	reset();
}

void Gameboy::frame(float frametime)
{
	static constexpr int mCyclePerFrame{17556};
	m_apu.setFrametime(frametime);
	for(;m_currentCycle <= mCyclePerFrame; ++m_currentCycle) mCycle();
	m_currentCycle = 1;
	m_apu.unlockThread();
}

void Gameboy::mCycle()
{
	m_cpu.mCycle(); 
	m_bus.handleDmaTransfer();
	m_timers.mCycle();
	m_ppu->mCycle();
}

void Gameboy::loadCartridge(const std::filesystem::path& filePath)
{
	reset();
	m_bus.getCartridgeSlot().loadCartridge(filePath);
}

void Gameboy::hardReset()
{
	reset();
	m_bus.getCartridgeSlot().reloadCartridge();
}

void Gameboy::nextCartridge()
{
	reset();
	m_bus.nextCartridge();
}

bool Gameboy::hasCartridge()
{
	return m_bus.getCartridgeSlot().hasCartridge();
}

uint16 Gameboy::currentCycle() const
{
	return m_currentCycle;
}

const PPU& Gameboy::getPPU() const
{
	return *m_ppu;
}