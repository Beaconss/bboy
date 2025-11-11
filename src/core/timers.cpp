#include "core/timers.h"
#include "hardware_registers.h"
#include "core/mmu.h"
#include "timers.h"

Timers::Timers(MMU& mmu)
	: m_bus{mmu}
	, m_timaResetCounter{}
	, m_lastAndResult{}
	, m_div{}
	, m_tima{}
	, m_tma{}
	, m_tac{}
{
	reset();
}

void Timers::reset()
{
	m_timaResetCounter = 0;
	m_lastAndResult = 0;
	m_div = 0xAB;
	m_tima = 0;
	m_tma = 0;
	m_tac = 0xF8;
}

void Timers::mCycle()
{
	for(int i{}; i < 4; ++i)
	{
		++m_div;
		if(m_timaResetCounter > 0)
		{
			if(--m_timaResetCounter == 0)
			{
				m_tima = m_tma;
				requestTimerInterrupt();
			}
		}
		
		const uint16 divBit{static_cast<uint16>(m_div & ((timaBitPositions[m_tac & 0b11])))}; //formula to extract the right bit based on the tima frequency
		const bool andResult{static_cast<bool>(divBit && (m_tac & 0b100))}; //bit 2 is the enable bit
		if(m_lastAndResult && !andResult) //falling edge 
		{
			constexpr int timaResetDelay{4};
			if(++m_tima == 0) m_timaResetCounter = timaResetDelay;
		}
		
		m_lastAndResult = andResult;
	}
}

uint8 Timers::getDiv() const
{
	return static_cast<uint8>(m_div >> 8); //in memory only div's upper 8 bits are mapped
}

uint8 Timers::getTima() const
{
    return m_tima;
}

uint8 Timers::getTma() const
{
    return m_tma;
}

uint8 Timers::getTac() const
{
    return m_tac | 0xF8;
}

void Timers::setDiv()
{
	m_div = 0;
}

void Timers::setTima(uint8 value)
{
	m_tima = value;
	m_timaResetCounter = 0;
}

void Timers::setTma(uint8 value)
{
	m_tma = value;
}

void Timers::setTac(uint8 value)
{
	m_tac = value;
}

void Timers::requestTimerInterrupt() const
{
	m_bus.write(hardwareReg::IF, m_bus.read(hardwareReg::IF, MMU::Component::timers) | 0b100, MMU::Component::timers); //bit 2 is timer interrupt
}