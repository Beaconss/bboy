#pragma once
#include "type_alias.h"
#include <array>

class MMU;
class Timers
{
public:
	Timers(MMU& bus);

	void reset();
	void mCycle();

	uint8 getDiv() const;
	uint8 getTima() const;
	uint8 getTma() const;
	uint8 getTac() const;
	void setDiv();
	void setTima(uint8 value);
	void setTma(uint8 value);
	void setTac(uint8 value);

private:
	static constexpr std::array<uint16, 4> timaBitPositions
	{
		1024 >> 1,
		16 >> 1,
		64 >> 1,
		256 >> 1
	};

	void requestTimerInterrupt() const;

	MMU& m_bus;

	uint8 m_timaResetCounter;
	bool m_lastAndResult;

	uint16 m_div; //divider register
	uint8 m_tima; //timer counter
	uint8 m_tma; //timer modulo
	uint8 m_tac; //timer control
};