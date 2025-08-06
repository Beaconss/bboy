#include "gameboy.h"

Gameboy::Gameboy()
	: m_bus{*this}
	, m_cpu{m_bus}
	, m_ppu{m_bus}
	, m_timers{m_bus}
{
}

void Gameboy::cycle() //1 machine cycle
{
	m_bus.cycle();
	for(int i{0}; i < 4; ++i)  //both timers and ppu works with t-cycles, so 4 for machine cycle
	{
		m_timers.cycle();
		m_ppu.cycle();
	}
	m_cpu.cycle(); 
}

