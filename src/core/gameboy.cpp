#include <core/gameboy.h>

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{m_bus}
	, m_apu{}
	, m_timers{m_bus}
	, m_input{}
{
	//std::thread audioThread(a);
	//audioThread.detach();
}

Gameboy::~Gameboy()
{
	reset();
}

void Gameboy::frame()
{
	constexpr int CYCLES_PER_FRAME{17556};
	for(int i{}; i < CYCLES_PER_FRAME; ++i) cycle();
}

void Gameboy::cycle() //1 machine cycle
{
	m_cpu.cycle(); 
	m_bus.handleDmaTransfer();
	for(int i{0}; i < 4; ++i) m_timers.cycle();
	m_ppu.cycle();
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