#include "timers.h"
#include "gameboy.h"

Timers::Timers(Gameboy& parent)
	: m_parent{parent}
	, m_div{}
	, m_tima{}
	, m_tma{}
	, m_tac{}
{
}

void Timers::write(Index index, uint8 value)
{
	switch(index)
	{
	case DIV: m_div = 0; break; //when div is written to it gets reset
	case TIMA: m_tima = value; break;
	case TMA: m_tma = value; break;
	case TAC: m_tac = value & 0x0F; break; //only lower 4 bits are used
	}
}

void Timers::update(const int cycleCounter)
{
	if(cycleCounter % 64 == 0) ++m_div; //update div every 64
	
	if(m_tac & 0b100) //if bit 2(enable bit)
	{
		switch(m_tac & 0b11)
		{
		case FREQUENCY_256: 
			if(cycleCounter % 256 == 0) 
			{
				if(++m_tima == 0x00)
				{
					m_tima = m_tma;
					requestInterrupt();
				}
				break;
			}
		case FREQUENCY_4: 
			if(cycleCounter % 4 == 0) 
			{
				if(++m_tima == 0x00)
				{
					m_tima = m_tma;
					requestInterrupt();
				}
				break;
			}
		case FREQUENCY_16: 
			if(cycleCounter % 16 == 0)
			{
				if(++m_tima == 0x00)
				{
					m_tima = m_tma;
					requestInterrupt();
				}
				break;
			}
		case FREQUENCY_64: 
			if(cycleCounter % 64 == 0) 
			{
				if(++m_tima == 0x00) //if it overflows
				{
					m_tima = m_tma;
					requestInterrupt();
				}
				break;
			}
		} 
	}
}

void Timers::requestInterrupt() const
{
	m_parent.writeMemory(hardwareReg::IF, m_parent.readMemory(hardwareReg::IF) | 0b100); //enable bit 2(timer interrupt)
}
