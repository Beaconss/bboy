#include <core/input.h>

Input::Input()
	: m_p1{0xCF}
	, m_inputBuffer{SDL_GetKeyboardState(0)}
{
}

uint8 Input::read() const
{
	constexpr uint8 SELECT_BUTTONS{0b10'0000};
	constexpr uint8 SELECT_DPAD{0b1'0000};

	if(!(m_p1 & SELECT_BUTTONS))
	{
		return (((!m_inputBuffer[SDL_SCANCODE_A]) << 3)
					| ((!m_inputBuffer[SDL_SCANCODE_S]) << 2)
					| ((!m_inputBuffer[SDL_SCANCODE_X]) << 1)
					| (!m_inputBuffer[SDL_SCANCODE_Z]));
	}
	else if(!(m_p1 & SELECT_DPAD))
	{
		return (((!m_inputBuffer[SDL_SCANCODE_DOWN]) << 3)
					| ((!m_inputBuffer[SDL_SCANCODE_UP]) << 2)
					| ((!m_inputBuffer[SDL_SCANCODE_LEFT]) << 1)
					| (!m_inputBuffer[SDL_SCANCODE_RIGHT]));
	}
	else return 0b1111;
}

void Input::write(const uint8 value)
{
	m_p1 = (m_p1 & 0b1100'1111) | (value & 0b0011'0000); //every bit except 4 and 5 are read-only
}


