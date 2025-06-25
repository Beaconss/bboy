#pragma once
#include "../type_alias.h"
#include "../hardware_registers.h"

class Gameboy;

class Timers
{
public:
	Timers(Gameboy& gameboy);

	enum Index
	{
		DIV,
		TIMA,
		TMA,
		TAC,
	};

	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);
	void cycle();
	void timerInterrupt() const;

private:
	enum TimaFrequency
	{
		FREQUENCY_256,
		FREQUENCY_4,
		FREQUENCY_16,
		FREQUENCY_64,
	};

	Gameboy& m_gameboy;
	int m_cycleCounter;
	uint8 m_div; //divider register
	uint8 m_tima; //timer counter
	uint8 m_tma; //timer modulo
	uint8 m_tac; //timer control

	bool m_updateTimaNextCycle; 
};

