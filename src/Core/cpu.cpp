#include "CPU.h"

CPU::CPU(Gameboy& gameboy)
	: cyclesCounter{}
	, m_gameboy{gameboy}
	, m_iState{}
	, m_currentInstr{nullptr}
	, m_PC{}
	, m_SP{}
	, m_registers{}
	, m_F{0xB0}
	, m_IR{}
	, m_ime{false}
	, m_imeEnableRequested{false}
	, m_imeEnableAfterNextInstruction{false}
	, m_isHalted{false}
{
	m_registers[B] = 0;
	m_registers[C] = 0x13;
	m_registers[D] = 0;
	m_registers[E] = 0xD8;
	m_registers[H] = 1;
	m_registers[L] = 0x4D;
	m_registers[A] = 1;
}

void CPU::cycle()
{
	if(handleInterrupts()) return;
	if(m_isHalted) return; //return if isHalted is still true after handling the interrupts

	if(m_imeEnableAfterNextInstruction)
	{
		m_ime = true;
		m_imeEnableAfterNextInstruction = false;
	}

	if(m_imeEnableRequested) 
	{
		m_imeEnableAfterNextInstruction = true;
		m_imeEnableRequested = false;
	}

	if(m_currentInstr) (this->*m_currentInstr)(); //call cached instruction if not nullptr
	else
	{
		fetch();
		execute();
	}
}

bool CPU::handleInterrupts()
{
	if(m_currentInstr == interruptRoutine) 
	{
		interruptRoutine();
		return true; //dont know if this should return true even if interruptRoutine finished
	}

	uint8 pendingInterrupts = readMemory(hardwareReg::IF) & readMemory(hardwareReg::IE);
	if(pendingInterrupts)
	{
		m_isHalted = false; //even if m_ime is false exit halt
		if(m_ime)
		{
			for(int i = 0; i <= 4; ++i)
			{
				if(pendingInterrupts & (1 << i))
				{
					writeMemory(hardwareReg::IF, pendingInterrupts & ~(1 << i));
					m_ime = false;
					m_iState.x = i; //save i to be used in interruptRoutine as interrupt index
					cyclesCounter = 1; //set cycles counter to 1 before interrupt routine to be sure to do every cycle
					interruptRoutine();
					return true;
				}
			}
		}
	}
	return false;
}

void CPU::interruptRoutine()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = interruptRoutine;
		break;
	case 2:
		break;
	case 3:
		writeMemory(--m_SP, getMSB(m_PC));
		break;
	case 4:
		writeMemory(--m_SP, getLSB(m_PC));
		break;
	case 5:
		m_PC = interruptHandlerAddress[m_iState.x]; //m_iState.x is interrupt index
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::fetch()
{
	m_IR = readMemory(m_PC++);
}

void CPU::execute()
{
	switch(m_IR) 
	{
		// ============ 8-bit load instructions ============
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F:
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F:
		LD_r_r2(); break;

	case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x3E:
		LD_r_n(); break;
		LD_r_n(); break;

	case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x7E:
		LD_r_HL(); break;

	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
		LD_HL_r(); break;

	case 0x36: LD_HL_n(); break;
	case 0x0A: LD_A_BC(); break;
	case 0x1A: LD_A_DE(); break;
	case 0x02: LD_BC_A(); break;
	case 0x12: LD_DE_A(); break;
	case 0xFA: LD_A_nn(); break;
	case 0xEA: LD_nn_A(); break;
	case 0xF2: LDH_A_C(); break;
	case 0xE2: LDH_C_A(); break;
	case 0xF0: LDH_A_n(); break;
	case 0xE0: LDH_n_A(); break;
	case 0x3A: LD_A_HLd(); break;
	case 0x32: LD_HLd_A(); break;
	case 0x2A: LD_A_HLi(); break;
	case 0x22: LD_HLi_A(); break;

		// ============ 16-bit load instructions ============
	case 0x01: case 0x11: case 0x21: case 0x31:
		LD_rr_nn(); break;

	case 0x08: LD_nn_SP(); break;
	case 0xF9: LD_SP_HL(); break;

	case 0xC5: case 0xD5: case 0xE5: case 0xF5:
		PUSH_rr(); break;

	case 0xC1: case 0xD1: case 0xE1: case 0xF1:
		POP_rr(); break;

	case 0xF8: LD_HL_SP_e(); break;

		// ============ 8-bit arithmetic and logical instructions ============
	case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
		ADD_r(); break;

	case 0x86: ADD_HL(); break;
	case 0xC6: ADD_n(); break;

	case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F:
		ADC_r(); break;

	case 0x8E: ADC_HL(); break;
	case 0xCE: ADC_n(); break;

	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
		SUB_r(); break;

	case 0x96: SUB_HL(); break;
	case 0xD6: SUB_n(); break;

	case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F:
		SBC_r(); break;

	case 0x9E: SBC_HL(); break;
	case 0xDE: SBC_n(); break;

	case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF:
		CP_r(); break;

	case 0xBE: CP_HL(); break;
	case 0xFE: CP_n(); break;

	case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x3C:
		INC_r(); break;

	case 0x34: INC_HL(); break;

	case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x3D:
		DEC_r(); break;

	case 0x35: DEC_HL(); break;

	case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7:
		AND_r(); break;

	case 0xA6: AND_HL(); break;
	case 0xE6: AND_n(); break;

	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7:
		OR_r(); break;

	case 0xB6: OR_HL(); break;
	case 0xF6: OR_n(); break;

	case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF:
		XOR_r(); break;

	case 0xAE: XOR_HL(); break;
	case 0xEE: XOR_n(); break;

	case 0x27: DAA(); break;
	case 0x2F: CPL(); break;
	case 0x3F: CCF(); break;
	case 0x37: SCF(); break;

		// ============ 16-bit arithmetic instructions ============
	case 0x03: case 0x13: case 0x23: case 0x33:
		INC_rr(); break;

	case 0x0B: case 0x1B: case 0x2B: case 0x3B:
		DEC_rr(); break;

	case 0x09: case 0x19: case 0x29: case 0x39:
		ADD_HL_rr(); break;

	case 0xE8: ADD_SP_e(); break;

		// ============ rotate, shift and bit operations instructions ============
	case 0x07: RLCA(); break;
	case 0x0F: RRCA(); break;
	case 0x17: RLA(); break;
	case 0x1F: RRA(); break;

	case 0xCB:
	{
		fetch();
		switch(m_IR)
		{
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07:
			RLC_r(); break;

		case 0x06: RLC_HL(); break;

		case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F:
			RRC_r(); break;

		case 0x0E: RRC_HL(); break;

		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17:
			RL_r(); break;

		case 0x16: RL_HL(); break;

		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F:
			RR_r(); break;

		case 0x1E: RR_HL(); break;

		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x27:
			SLA_r(); break;

		case 0x26: SLA_HL(); break;

		case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2F:
			SRA_r(); break;

		case 0x2E: SRA_HL(); break;

		case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x37:
			SWAP_r(); break;

		case 0x36: SWAP_HL(); break;

		case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3F:
			SRL_r(); break;

		case 0x3E: SRL_HL(); break;

		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
		case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F:
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
		case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F:
			BIT_b_r(); break;

		case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x76: case 0x7E:
			BIT_b_HL(); break;

		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
		case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F:
		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7:
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF:
		case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7:
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF:
			RES_b_r(); break;

		case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE: case 0xB6: case 0xBE:
			RES_b_HL(); break;

		case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC7:
		case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCF:
		case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD7:
		case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDF:
		case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE7:
		case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEF:
		case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF7:
		case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFF:
			SET_b_r(); break;

		case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE: case 0xF6: case 0xFE:
			SET_b_HL(); break;

		default:
			break;
		} 
		break;
	}
			 // ============ control flow instructions ============
	case 0xC3: JP_nn(); break;
	case 0xE9: JP_HL(); break;

	case 0xC2: case 0xD2: case 0xCA: case 0xDA:
		JP_cc_nn(); break;

	case 0x18: JR_e(); break;

	case 0x20: case 0x28: case 0x30: case 0x38:
		JR_cc_e(); break;

	case 0xCD: CALL_nn(); break;

	case 0xC4: case 0xD4: case 0xCC: case 0xDC:
		CALL_cc_nn(); break;

	case 0xC9: RET(); break;

	case 0xC0: case 0xD0: case 0xC8: case 0xD8:
		RET_cc(); break;

	case 0xD9: RETI(); break;

	case 0xC7: case 0xD7: case 0xE7: case 0xF7:
	case 0xCF: case 0xDF: case 0xEF: case 0xFF:
		RST_n(); break;

		// ============ other ============
	case 0x76: HALT(); break;
	case 0x10: STOP(); break;
	case 0xF3: DI(); break;
	case 0xFB: EI(); break;
	case 0x00: NOP(); break;

	default:
		std::cerr << "Invalid opcode";
		break;
	}
}

uint8 CPU::getMSB(uint16 in)
{
	return in >> 8;
}

uint8 CPU::getLSB(uint16 in)
{
	return in & 0x00FF;
}

uint16 CPU::getBC()
{
	return (m_registers[B] << 8) | m_registers[C];
}

uint16 CPU::getDE()
{
	return (m_registers[D] << 8) | m_registers[E];
}

uint16 CPU::getHL()
{
	return (m_registers[H] << 8) | m_registers[L];
}

void CPU::setBC(uint16 BC)
{
	m_registers[B] = getMSB(BC);
	m_registers[C] = getLSB(BC);
}

void CPU::setDE(uint16 DE)
{
	m_registers[D] = getMSB(DE);
	m_registers[E] = getLSB(DE);
}

void CPU::setHL(uint16 HL)
{
	m_registers[H] = getMSB(HL);
	m_registers[L] = getLSB(HL);
}

bool CPU::getFZ()
{
	return m_F & 0b10000000;
}

bool CPU::getFN()
{
	return m_F & 0b01000000;
}

bool CPU::getFH()
{
	return m_F & 0b00100000;
}

bool CPU::getFC()
{
	return m_F & 0b00010000;
}

void CPU::setFZ(bool z)
{
	if(z) m_F = m_F | 0b10000000;
	else  m_F = m_F & 0b01111111;
}

void CPU::setFN(bool n)
{
	if(n) m_F = m_F | 0b01000000;
	else  m_F = m_F & 0b10111111;
}

void CPU::setFH(bool h)
{
	if(h) m_F = m_F | 0b00100000;
	else  m_F = m_F & 0b11011111;
} 

void CPU::setFC(bool c)
{
	if(c) m_F = m_F | 0b00010000;
	else  m_F = m_F & 0b11101111;
}

void CPU::LD_r_r2()
{
	m_iState.x = (m_IR >> 3) & 0b00000111; //r
	m_iState.y = m_IR & 0b00000111; //r2

	m_registers[m_iState.x] = m_registers[m_iState.y];
	cyclesCounter = 0; //reset cycles counter at the end of each instruction
}

void CPU::LD_r_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_r_n;
		m_iState.x = (m_IR >> 3) & 0b00000111; //r
		break;
	case 2:
		m_iState.y = readMemory(m_PC++); //n
		m_registers[m_iState.x] = m_iState.y;
		cyclesCounter = 0;
		m_currentInstr = nullptr; //in multi cycles instructions reset currentInstr too
		break;
	}
}

void CPU::LD_r_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_r_HL;
		m_iState.x = (m_IR >> 3) & 0b00000111; //r
		break;
	case 2:
		m_registers[m_iState.x] = readMemory(getHL());
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_HL_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_HL_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		writeMemory(getHL(), m_registers[m_iState.x]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_HL_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_HL_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		break;
	case 3:
		writeMemory(getHL(), m_iState.x);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_A_BC()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_A_BC;
		break;
	case 2:
		m_registers[A] = readMemory(getBC());
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_A_DE()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_A_DE;
		break;
	case 2:
		m_registers[A] = readMemory(getDE());
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_BC_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_BC_A;
		break;
	case 2:
		writeMemory(getBC(), m_registers[A]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_DE_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_DE_A;
		break;
	case 2:
		writeMemory(getDE(), m_registers[A]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_A_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_A_nn;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		break;
	case 4:
		m_registers[A] = readMemory(m_iState.xx);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_nn_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_nn_A;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		break;
	case 4:
		writeMemory(m_iState.xx, m_registers[A]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LDH_A_C()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LDH_A_C;
		break;
	case 2:
		m_registers[A] = readMemory(0xFF00 | m_registers[C]); //xx is 0xFF00 as the high byte + C as the low byte
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LDH_C_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LDH_C_A;
		break;
	case 2:
		writeMemory(0xFF00 | m_registers[C], m_registers[A]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LDH_A_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LDH_A_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		break;
	case 3:
		m_registers[A] = readMemory(0xFF00 | m_iState.x);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LDH_n_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LDH_n_A;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		break;
	case 3:
		writeMemory(0xFF00 | m_iState.x, m_registers[A]);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_A_HLd()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_A_HLd;
		break;
	case 2:
		m_registers[A] = readMemory(getHL());
		if(--m_registers[L] == 0xFF) --m_registers[H]; //check for underflow
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_HLd_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_HLd_A;
		break;
	case 2:
		writeMemory(getHL(), m_registers[A]);
		if(--m_registers[L] == 0xFF) --m_registers[H];
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_A_HLi()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_A_HLi;
		break;
	case 2:
		m_registers[A] = readMemory(getHL());
		if(++m_registers[L] == 0x00) ++m_registers[H]; //check for overflow
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_HLi_A()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_HLi_A;
		break;
	case 2:
		writeMemory(getHL(), m_registers[A]);
		if(++m_registers[L] == 0x00) ++m_registers[H];
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_rr_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_rr_nn;
		m_iState.x = (m_IR >> 4) & 0b11; //rr
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		switch(m_iState.x)
		{
		case BC: setBC(m_iState.xx); break;
		case DE: setDE(m_iState.xx); break;
		case HL: setHL(m_iState.xx); break;
		case SP: m_SP = m_iState.xx; break;
		}
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_nn_SP()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_nn_SP;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		break;
	case 4:
		writeMemory(m_iState.xx, getLSB(m_SP));
		break;
	case 5:
		writeMemory(m_iState.xx + 1, getMSB(m_SP));
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_SP_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_SP_HL;
		break;
	case 2:
		m_SP = getHL();
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::PUSH_rr()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = PUSH_rr;
		m_iState.x = (m_IR >> 4) & 0b11; //rr
		break;
	case 2:
		--m_SP;
		break;
	case 3:
		switch(m_iState.x)
		{
		case BC: writeMemory(m_SP, m_registers[B]); break;
		case DE: writeMemory(m_SP, m_registers[D]); break;
		case HL: writeMemory(m_SP, m_registers[H]); break;
		case AF: writeMemory(m_SP, m_registers[A]); break;
		}
		--m_SP;
		break;
	case 4:
		switch(m_iState.x)
		{
		case BC: writeMemory(m_SP, m_registers[C]); break;
		case DE: writeMemory(m_SP, m_registers[E]); break;
		case HL: writeMemory(m_SP, m_registers[L]); break;
		case AF: writeMemory(m_SP, m_F & 0xF0); break;
		}
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::POP_rr()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = POP_rr;
		m_iState.x = (m_IR >> 4) & 0b11; //rr
		break;
	case 2:
		m_iState.xx = readMemory(m_SP++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_SP++) << 8;
		switch(m_iState.x)
		{
		case BC: setBC(m_iState.xx); break;
		case DE: setDE(m_iState.xx); break;
		case HL: setHL(m_iState.xx); break;
		case AF: 
			m_registers[A] = getMSB(m_iState.xx); 
			m_F = getLSB(m_iState.xx) & 0xF0; 
			break;
		}
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::LD_HL_SP_e()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = LD_HL_SP_e;
		break;
	case 2:
		m_iState.e = static_cast<int8>(readMemory(m_PC++)); //e
		m_iState.xx = m_SP + static_cast<uint8>(m_iState.e);
		break;
	case 3:
		setHL(m_iState.xx);

		setFZ(false);
		setFN(false);
		setFH(((m_SP & 0xF) + (static_cast<uint8>(m_iState.e) & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADD_r()
{
	m_iState.x = m_IR & 0b00000111; //r
	m_iState.xx = m_registers[A] + m_registers[m_iState.x]; //result

	setFZ(m_iState.xx == 0);
	setFN(false);
	setFH(((m_registers[A] & 0xF) + (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	cyclesCounter = 0;
}

void CPU::ADD_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADD_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		m_iState.xx = m_registers[A] + m_iState.x; //result

		setFZ(m_iState.xx == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADD_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADD_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		m_iState.xx = m_registers[A] + m_iState.x; //result

		setFZ(m_iState.xx == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADC_r()
{
	m_iState.x = m_IR & 0b00000111; //r
	m_iState.xx = m_registers[A] + m_registers[m_iState.x] + getFC(); //result

	setFZ(m_iState.xx == 0);
	setFN(false);
	setFH(((m_registers[A] & 0xF) + (m_registers[m_iState.x] & 0xF) + (getFC())) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	cyclesCounter = 0;
}

void CPU::ADC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADC_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		m_iState.xx = m_registers[A] + m_iState.x + getFC(); //result

		setFZ(m_iState.xx == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF) + (getFC())) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADC_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADC_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		m_iState.xx = m_registers[A] + m_iState.x + getFC(); //result

		setFZ(m_iState.xx == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF) + (getFC())) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SUB_r()
{
	m_iState.x = m_IR & 0b00000111; //r
	m_iState.y = m_registers[A] - m_registers[m_iState.x]; //result

	setFZ(m_iState.y == 0);
	setFN(true);
	setFH((m_registers[A] & 0xF) < (m_registers[m_iState.x] & 0xF));
	setFC(m_registers[A] < m_registers[m_iState.x]);

	m_registers[A] = m_iState.y;
	cyclesCounter = 0;
}

void CPU::SUB_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SUB_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		m_iState.y = m_registers[A] - m_iState.x; //result

		setFZ(m_iState.y == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) < (m_iState.x & 0xF));
		setFC(m_registers[A] < m_iState.x);

		m_registers[A] = m_iState.y;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SUB_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SUB_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		m_iState.y = m_registers[A] - m_iState.x; //result

		setFZ(m_iState.y == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) < (m_iState.x & 0xF));
		setFC(m_registers[A] < m_iState.x);

		m_registers[A] = m_iState.y;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SBC_r()
{
	m_iState.x = m_IR & 0b00000111; //r
	m_iState.y = m_registers[A] - m_registers[m_iState.x] - getFC(); //result

	setFZ(m_iState.y == 0);
	setFN(true);
	setFH(((m_registers[A] & 0xF) - (m_registers[m_iState.x] & 0xF) - getFC()) < 0);
	setFC((m_registers[A] - m_registers[m_iState.x] - getFC()) < 0);

	m_registers[A] = m_iState.y;
	cyclesCounter = 0;
}

void CPU::SBC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SBC_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		m_iState.y = m_registers[A] - m_iState.x - getFC(); //result

		setFZ(m_iState.y == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF) - getFC()) < 0);
		setFC((m_registers[A] - m_iState.x - getFC()) < 0);

		m_registers[A] = m_iState.y;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SBC_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SBC_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n
		m_iState.y = m_registers[A] - m_iState.x - getFC(); //result

		setFZ(m_iState.y == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF) - getFC()) < 0);
		setFC((m_registers[A] - m_iState.x - getFC()) < 0);

		m_registers[A] = m_iState.y;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::CP_r()
{
	m_iState.x = m_IR & 0b00000111; //r

	setFZ((m_registers[A] - m_registers[m_iState.x]) == 0);
	setFN(true);
	setFH((m_registers[A] & 0xF) < (m_registers[m_iState.x] & 0xF));
	setFC(m_registers[A] < m_registers[m_iState.x]);
	cyclesCounter = 0;
}

void CPU::CP_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = CP_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem

		setFZ((m_registers[A] - m_iState.x) == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) < (m_iState.x & 0xF));
		setFC(m_registers[A] < m_iState.x);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::CP_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = CP_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n

		setFZ((m_registers[A] - m_iState.x) == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) < (m_iState.x & 0xF));
		setFC(m_registers[A] < m_iState.x);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::INC_r()
{
	m_iState.x = (m_IR >> 3) & 0b00000111; //r

	setFZ((m_registers[m_iState.x] + 1) == 0);
	setFN(false);
	setFH((m_registers[m_iState.x] & 0xF) == 0xF);

	++m_registers[m_iState.x];
	cyclesCounter = 0;
}

void CPU::INC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = INC_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 3:
		writeMemory(getHL(), m_iState.x + 1);

		setFZ((m_iState.x + 1) == 0);
		setFN(false);
		setFH((m_iState.x & 0xF) == 0xF);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::DEC_r()
{
	m_iState.x = (m_IR >> 3) & 0b00000111; //r

	setFZ((m_registers[m_iState.x] - 1) == 0);
	setFN(true);
	setFH((m_registers[m_iState.x] & 0xF) == 0xF);

	--m_registers[m_iState.x];
	cyclesCounter = 0;
}

void CPU::DEC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = DEC_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 3:
		writeMemory(getHL(), m_iState.x - 1);

		setFZ((m_iState.x - 1) == 0);
		setFN(true);
		setFH((m_iState.x & 0xF) == 0xF);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::AND_r()
{
	m_iState.x = m_IR & 0b00000111; //r

	m_registers[A] &= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(true);
	setFC(false);
	cyclesCounter = 0;
}

void CPU::AND_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = AND_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem

		m_registers[A] &= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(true);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::AND_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = AND_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n

		m_registers[A] &= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(true);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::OR_r()
{
	m_iState.x = m_IR & 0b00000111; //r

	m_registers[A] |= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(false);
	setFC(false);
	cyclesCounter = 0;
}

void CPU::OR_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = OR_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem

		m_registers[A] |= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::OR_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = OR_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n

		m_registers[A] |= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::XOR_r()
{
	m_iState.x = m_IR & 0b00000111; //r

	m_registers[A] ^= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(false);
	setFC(false);
	cyclesCounter = 0;
}

void CPU::XOR_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = OR_HL;
		break;
	case 2:
		m_iState.x = readMemory(getHL()); //HL mem

		m_registers[A] ^= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::XOR_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = OR_n;
		break;
	case 2:
		m_iState.x = readMemory(m_PC++); //n

		m_registers[A] ^= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::DAA()
{
	m_iState.x = 0; //adj

	if(!getFN() && (m_registers[A] & 0xF) > 0x9 || getFH())
	{
		m_iState.x += 0x6;
	}

	if(!getFN() && m_registers[A] > 0x99 || getFC())
	{
		m_iState.x += 0x60;
		setFC(true);
	}

	if(!getFN()) m_registers[A] += m_iState.x;
	else m_registers[A] -= m_iState.x;

	setFZ(m_registers[A] == 0);
	setFH(false);
	cyclesCounter = 0;
}

void CPU::CPL()
{
	m_registers[A] = ~m_registers[A];
	setFN(true);
	setFH(true);
	cyclesCounter = 0;
}

void CPU::CCF()
{
	setFN(false);
	setFH(false);
	setFC(!getFC());
	cyclesCounter = 0;
}

void CPU::SCF()
{
	setFN(false);
	setFH(false);
	setFC(true);
	cyclesCounter = 0;
}

void CPU::INC_rr()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = INC_rr;
		m_iState.x = (m_IR >> 4) & 0b00000011; //rr
		break;
	case 2:
		switch(m_iState.x) 
		{
		case BC: setBC(getBC() + 1); break;
		case DE: setDE(getDE() + 1); break;
		case HL: setHL(getHL() + 1); break;
		case SP: if((++m_F & 0xF0) == 0x00) ++m_registers[A]; break;
		}
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::DEC_rr()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = DEC_rr;
		m_iState.x = (m_IR >> 4) & 0b00000011; //rr
		break;
	case 2:
		switch(m_iState.x)
		{
		case BC: setBC(getBC() - 1); break;
		case DE: setDE(getDE() - 1); break;
		case HL: setHL(getHL() - 1); break;
		case SP: if((--m_F & 0xF0) == 0x00) --m_registers[A]; break;
		}
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADD_HL_rr()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADD_HL_rr;
		m_iState.x = (m_IR >> 4) & 0b00000011; //rr
		break;
	case 2:
		switch(m_iState.x) //m_iState.xx is rr value in this switch
		{
		case Register16bit::BC:
			m_iState.xx = getBC();
			break;
		case Register16bit::DE:
			m_iState.xx = getDE();
			break;
		case Register16bit::HL:
			m_iState.xx = getHL();
			break;
		case Register16bit::SP:
			m_iState.xx = m_SP;
			break;
		}
		m_iState.xxx = getHL() + m_iState.xx; //result

		setHL(static_cast<uint16>(m_iState.xxx));

		setFN(false);
		setFH(((getHL() & 0xFFF) + (m_iState.xx & 0xFFF)) & 0x1000);
		setFC(m_iState.xxx & 0x10000);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::ADD_SP_e()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = ADD_SP_e;
		break;
	case 2:
		m_iState.e = static_cast<uint8>(readMemory(m_PC++)); //e
		break;
	case 3:
		break;
	case 4:
		m_iState.xx = m_SP + m_iState.e;

		setFZ(false);
		setFN(false);
		setFH(((m_SP & 0xF) + (m_iState.e & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_SP = m_iState.xx;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RLCA()
{
	m_iState.x = m_registers[A] >> 7; //bit out

	m_registers[A] = (m_registers[A] << 1) | m_iState.x;

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	cyclesCounter = 0;
}

void CPU::RRCA()
{
	m_iState.x = (m_registers[A] & 1) << 7; //bit out

	m_registers[A] = (m_registers[A] >> 1) | m_iState.x;

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	cyclesCounter = 0;
}

void CPU::RLA()
{
	m_iState.x = m_registers[A] >> 7; //bit out

	m_registers[A] = (m_registers[A] << 1) | static_cast<uint8>(getFC());

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	cyclesCounter = 0;
}

void CPU::RRA()
{
	m_iState.x = m_registers[A] & 1; //bit out

	m_registers[A] = (m_registers[A] >> 1) | (static_cast<uint8>(getFC()) << 7);

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	cyclesCounter = 0;
}

void CPU::RLC_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RLC_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] << 1) | m_iState.y;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RLC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RLC_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | m_iState.y; //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RRC_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RRC_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = (m_registers[m_iState.x] & 1) << 7; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | m_iState.y;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RRC_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RRC_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = (m_iState.x & 1) << 7; //bit out
		m_iState.z = (m_iState.x >> 1) | (m_iState.y << 7); //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RL_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RL_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out
		m_registers[m_iState.x] = (m_registers[m_iState.x] << 1) | static_cast<uint8>(getFC());

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RL_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RL_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | static_cast<uint8>(getFC()); //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RR_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RR_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | (static_cast<uint8>(getFC()) << 7);

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RR_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RR_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = (m_iState.x >> 1) | (static_cast<uint8>(getFC()) << 7); //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SLA_r() //for some reason this doesnt actually do an arithmetic shift
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SLA_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out

		m_registers[m_iState.x] = m_registers[m_iState.x] << 1;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SLA_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SLA_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = m_iState.x << 1; //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SRA_r() //this actually does an arithmetic shift
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SRA_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | (m_registers[m_iState.x] & 0x80); //registers[x] & 0x80 = sign bit

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SRA_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SRA_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL());
		break;
	case 4:
		m_iState.y = m_iState.x & 1; // bit out
		m_iState.z = (m_iState.x >> 1) | (m_iState.x & 0x80); //z = result, x & 0x80 = sign bit

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::SWAP_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SWAP_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r

		m_registers[m_iState.x] = ((m_registers[m_iState.x] & 0xF0) >> 4) | ((m_registers[m_iState.x] & 0x0F) << 4);

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::SWAP_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SWAP_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = ((m_iState.x & 0xF0) >> 4) | ((m_iState.x & 0x0F) << 4); //result

		writeMemory(getHL(), m_iState.y);

		setFZ(m_iState.y == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SRL_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SRL_r;
		break;
	case 2:
		m_iState.x = m_IR & 0b00000111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = m_registers[m_iState.x] >> 1;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::SRL_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SRL_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = m_iState.x >> 1; //result

		writeMemory(getHL(), m_iState.z);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::BIT_b_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = BIT_b_r;
		break;
	case 2:
		m_iState.x = (m_IR >> 3) & 0b00000111; //b
		m_iState.y = m_IR & 0b00000111; //r

		setFZ((m_registers[m_iState.y] & (1 << m_iState.x)) == 0); //shift the bit in the 0 place by b times to check the right bit
		setFN(false);
		setFH(true);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::BIT_b_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = BIT_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = (m_IR >> 3) & 0b00000111; //b
		m_iState.y = readMemory(getHL()); //HL mem

		setFZ((m_iState.y & (1 << m_iState.x)) == 0);
		setFN(false);
		setFH(true);

		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::RES_b_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RES_b_r;
		break;
	case 2:
		m_iState.x = (m_IR >> 3) & 0b00000111; //b
		m_iState.y = m_IR & 0b00000111; //r

		m_registers[m_iState.y] &= ~(1 << m_iState.x);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::RES_b_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RES_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = (m_IR >> 3) & 0b00000111; //b

		writeMemory(getHL(), m_iState.x & ~(1 << m_iState.y));
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::SET_b_r()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SET_b_r;
		break;
	case 2:
		m_iState.x = (m_IR >> 3) & 0b00000111; //b
		m_iState.y = m_IR & 0b00000111; //r

		m_registers[m_iState.y] |= (1 << m_iState.x);
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::SET_b_HL()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = SET_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = readMemory(getHL()); //HL mem
		break;
	case 4:
		m_iState.y = (m_IR >> 3) & 0b00000111; //b

		writeMemory(getHL(), m_iState.x | (1 << m_iState.y));
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::JP_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = JP_nn;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		break;
	case 4:
		m_PC = m_iState.xx;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::JP_HL()
{
	m_PC = getHL();
	cyclesCounter = 0;
}

void CPU::JP_cc_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = JP_cc_nn;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx |= readMemory(m_PC++) << 8; //nn
		m_iState.x = (m_IR >> 3) & 0b00000011; //cc

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NotZero: m_iState.y = !getFZ(); break;
		case Zero: m_iState.y = getFZ(); break;
		case NotCarry: m_iState.y = !getFC(); break;
		case Carry: m_iState.y = getFC(); break;
		}

		if(m_iState.y) m_PC = m_iState.xx;
		else
		{
			cyclesCounter = 0;
			m_currentInstr = nullptr;
		}
		break;
	case 4:
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::JR_e()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = JR_e;
		break;
	case 2:
		m_iState.e = static_cast<int8>(readMemory(m_PC++)); //e
		break;
	case 3:
		m_PC += m_iState.e;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
	}
}

void CPU::JR_cc_e()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = JR_cc_e;
		break;
	case 2:
		m_iState.x = (m_IR >> 3) & 0b00000011; //cc
		m_iState.e = static_cast<int8>(readMemory(m_PC++)); //e

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NotZero: m_iState.y = !getFZ(); break;
		case Zero: m_iState.y = getFZ(); break;
		case NotCarry: m_iState.y = !getFC(); break;
		case Carry: m_iState.y = getFC(); break;
		}

		if(m_iState.y) m_PC += m_iState.e;
		else 
		{
			cyclesCounter = 0;
			m_currentInstr = nullptr;
		}
		break;
	case 3:
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::CALL_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = CALL_nn;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.xx = readMemory(m_PC++); //nn
		break;
	case 4:
		break;
	case 5:
		writeMemory(--m_SP, getMSB(m_PC));
		break;
	case 6:
		writeMemory(--m_SP, getLSB(m_PC));
		m_PC = m_iState.xx;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::CALL_cc_nn()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = CALL_cc_nn;
		break;
	case 2:
		m_iState.xx = readMemory(m_PC++);
		break;
	case 3:
		m_iState.x = (m_IR >> 3) & 0b00000011; //cc
		m_iState.xx = readMemory(m_PC++); //nn

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NotZero: m_iState.y = !getFZ(); break;
		case Zero: m_iState.y = getFZ(); break;
		case NotCarry: m_iState.y = !getFC(); break;
		case Carry: m_iState.y = getFC(); break;
		}

		if(!m_iState.y)
		{
			cyclesCounter = 0;
			m_currentInstr = nullptr;
		}
		break;
	case 4:
		break;
	case 5:
		writeMemory(--m_SP, getMSB(m_PC));
		break;
	case 6:
		writeMemory(--m_SP, getLSB(m_PC));
		m_PC = m_iState.xx;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RET()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RET;
		break;
	case 2:
		m_iState.x = readMemory(m_SP++); 
		break;
	case 3:
		m_iState.y = readMemory(m_SP++);
		break;
	case 4:
		m_PC = (m_iState.y << 8) | m_iState.x;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RET_cc()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RET_cc;
		break;
	case 2:
		m_iState.x = (m_IR >> 3) & 0b00000011; //cc

		switch(m_iState.x)
		{
		case NotZero: m_iState.y = !getFZ(); break;
		case Zero: m_iState.y = getFZ(); break;
		case NotCarry: m_iState.y = !getFC(); break;
		case Carry: m_iState.y = getFC(); break;
		}

		if(!m_iState.y)
		{
			cyclesCounter = 0;
			m_currentInstr = nullptr;
		}
		break;
	case 3:
		m_iState.x = readMemory(m_SP++);
		break;
	case 4:
		m_iState.y = readMemory(m_SP++);
		break;
	case 5:
		m_PC = (m_iState.y << 8) | m_iState.x;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RETI()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RETI;
		break;
	case 2:
		m_iState.x = readMemory(m_SP++);
		break;
	case 3:
		m_iState.y = readMemory(m_SP++);
		break;
	case 4:
		m_PC = (m_iState.y << 8) | m_iState.x;
		m_ime = true;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::RST_n()
{
	switch(cyclesCounter)
	{
	case 1:
		m_currentInstr = RST_n;
		break;
	case 2:
		break;
	case 3:
		writeMemory(--m_SP, getMSB(m_PC));
		break;
	case 4:
		writeMemory(--m_SP, getLSB(m_PC));
		m_iState.x = (m_IR >> 3) & 0b00000111; //n
		
		m_PC = (0x00 << 8) | m_iState.x;
		cyclesCounter = 0;
		m_currentInstr = nullptr;
		break;
	}
}

void CPU::HALT()
{
	m_isHalted = true;
	if(!m_ime && (readMemory(hardwareReg::IF) & readMemory(hardwareReg::IE)) != 0) ++m_PC; //HALT bug(dont know if its correct)

	cyclesCounter = 0;
}

void CPU::STOP()
{
	//TODO AOI
	cyclesCounter = 0;
}

void CPU::DI()
{
	m_ime = false;
	m_imeEnableRequested = false;
	m_imeEnableAfterNextInstruction = false;
	cyclesCounter = 0;
}

void CPU::EI()
{
	m_imeEnableRequested = true;
	cyclesCounter = 0;
}

void CPU::NOP()
{
	cyclesCounter = 0; //nop
}