#include "gameboy.h"

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{m_bus}
	, m_timers{m_bus}
	, m_input{}
{
}

Gameboy::~Gameboy()
{
	reset();
}

void Gameboy::cycle() //1 machine cycle
{
	m_bus.handleDmaTransfer();
	m_cpu.cycle(); 
	for(int i{0}; i < 4; ++i)  //both timers and ppu works with t-cycles, so 4 for machine cycle
	{
		m_timers.cycle();
		m_ppu.cycle();
	}
}

void Gameboy::reset()
{
	m_bus.reset();
	m_cpu.reset();
	m_ppu.reset();
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