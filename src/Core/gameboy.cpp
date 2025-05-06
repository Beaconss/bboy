#include "gameboy.h"

Gameboy::Gameboy()
	: m_cpu{*this}
	, m_memory{}
	, m_timers{}
{
}

void Gameboy::cycle() //1 machine cycle
{
	++m_cpu.cyclesCounter; //since instructions reset cyclesCounter to 0 increment before the cpu cycle so its 1, then if the instruction is multi-cycle 2, 3...
	m_cpu.cycle();

	//...
}

uint8 Gameboy::readMemory(uint16 addr)
{
	return m_memory[addr];
}

void Gameboy::writeMemory(uint16 addr, uint8 value)
{
	using namespace hardwareReg;
	switch(addr)
	{
	//case 0xFF04: case 0xFF05: case 0xFF06: case 0xFF07:
	case IF: m_memory[addr] = value & 0xF0; break;
	default: m_memory[addr] = value; break;
	}
}

uint8 CPU::readMemory(uint16 addr)
{
	return m_gameboy.readMemory(addr);
}

void CPU::writeMemory(uint16 addr, uint8 value)
{
	m_gameboy.writeMemory(addr, value);
}
