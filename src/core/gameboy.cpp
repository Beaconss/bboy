#include <core/gameboy.h>

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{m_bus}
	, m_apu{}
	, m_timers{m_bus}
	, m_input{}
{
}

Gameboy::~Gameboy()
{
	reset();
}

void Gameboy::frame()
{
	constexpr int CYCLES_PER_FRAME{17556};
	for(int i{}; i < CYCLES_PER_FRAME; ++i) mCycle();
	m_apu.pushAudio();
}

void Gameboy::mCycle()
{
	m_cpu.mCycle(); 
	m_bus.handleDmaTransfer();
	m_timers.mCycle();
	m_ppu.mCycle();
	m_apu.mCycle();
}

void Gameboy::loadCartridge(const std::filesystem::path& filePath)
{
	reset();
	m_bus.loadCartridge(filePath);
}

void Gameboy::nextTest()
{
	reset();
	m_bus.nextTest();
}

void Gameboy::reset()
{
	m_bus.reset();
	m_cpu.reset();
	m_ppu.reset();
	m_apu.reset();
	m_timers.reset();
}

bool Gameboy::hasRom() const
{
	return m_bus.hasRom();
}

const uint16* Gameboy::getLcdBuffer() const
{
	return m_ppu.getLcdBuffer();
} 