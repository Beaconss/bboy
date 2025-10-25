#include <core/gameboy.h>

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{std::make_unique<PPU>(m_bus)}
	, m_apu{}
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

void Gameboy::frame(float frameTime)
{
	m_apu.setupFrame(frameTime);
	static constexpr int M_CYCLES_PER_FRAME{17556};
	for(;m_currentCycle <= M_CYCLES_PER_FRAME; ++m_currentCycle) mCycle();
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
	m_bus.loadCartridge(filePath);
}

void Gameboy::nextCartridge()
{
	reset();
	m_bus.nextCartridge();
}

const bool Gameboy::hasCartridge() const
{
	return m_bus.hasCartridge();
}

const PPU& Gameboy::getPPU() const
{
	return *m_ppu;
}