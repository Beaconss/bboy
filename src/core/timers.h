#pragma once
#include "type_alias.h"
#include <array>

class Bus;
class Timers
{
public:
	Timers(Bus& bus);

	enum Index
	{
		div,
		tima,
		tma,
		tac,
	};

	void reset();
	void mCycle();
	uint8 read(const Index index) const;
	void write(const Index index, const uint8 value);

private:
	static constexpr std::array<uint16, 4> timaBitPositions
	{
		1024 >> 1,
		16 >> 1,
		64 >> 1,
		256 >> 1
	};

	void requestTimerInterrupt() const;

	Bus& m_bus;

	uint8 m_timaResetCounter;
	bool m_lastAndResult;

	uint16 m_div; //divider register
	uint8 m_tima; //timer counter
	uint8 m_tma; //timer modulo
	uint8 m_tac; //timer control
	uint8 m_oldTma;
};

