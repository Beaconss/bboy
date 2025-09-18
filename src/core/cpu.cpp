#include <core/cpu.h>

CPU::CPU(Bus& bus)
	: m_bus{bus}
	, m_iState{}
	, m_currentInstr{}
	, m_cycleCounter{}
	, m_ime{}
	, m_imeEnableNextCycle{}
	, m_isHalted{}
	, m_pendingInterrupts{}
	, m_interruptIndex{}
	, m_pc{}
	, m_sp{}
	, m_registers{}
	, m_f{}
	, m_ir{}
{
	reset();
}

void CPU::reset()
{
	m_iState = IState{};
	endInstruction();
	m_ime = false;
	m_imeEnableNextCycle = false;
	m_isHalted = false;
	m_pendingInterrupts = 0;
	m_interruptIndex = 0;
	m_pc = 0x100;
	m_sp = 0xFFFE;
	m_f = 0xB0;
	m_ir = 0;
	m_registers[B] = 0x00;
	m_registers[C] = 0x13;
	m_registers[D] = 0x00;
	m_registers[E] = 0xD8;
	m_registers[H] = 0x01;
	m_registers[L] = 0x4D;
	m_registers[A] = 0x01;
}

void CPU::cycle()
{
	++m_cycleCounter;//since instructions reset m_cycleCounter to 0 increment before the cpu cycle so its 1, then if the instruction is multi-cycle 2, 3...
	if(!m_currentInstr) handleInterrupts();
	if(m_isHalted) 
	{
		endInstruction();
		return;
	}

	if(m_imeEnableNextCycle)
	{
		m_ime = true;
		m_imeEnableNextCycle = false;
	}

	if(m_currentInstr) (this->*m_currentInstr)();
	else
	{
		fetch();
		execute();
	}
}

void CPU::handleInterrupts()
{
	m_pendingInterrupts = (m_bus.read(hardwareReg::IF, Bus::Component::CPU) & m_bus.read(hardwareReg::IE, Bus::Component::CPU)) & 0b1'1111; //only bits 0-4 are used
	if(m_pendingInterrupts)
	{
		m_isHalted = false; //even if m_ime is false exit halt
		if(m_ime)
		{
			for(int i = 0; i <= 4; ++i)
			{
				if(m_pendingInterrupts & (1 << i))
				{
					m_interruptIndex = i;
					m_ime = false;
					m_imeEnableNextCycle = false;
					m_currentInstr = &CPU::interruptRoutine;
					break;
				}
			}
		}
	}
}

void CPU::interruptRoutine()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::interruptRoutine;
		break;
	case 2:
		break;
	case 3:
		m_bus.write(--m_sp, getMSB(m_pc), Bus::Component::CPU);
		//here if the interrupt currently dispatching gets disabled it checks if 
		//there's another one to continue the dispatching with and if not it cancels the dispatching
		if(m_sp == hardwareReg::IE && !(getMSB(m_pc) & (1 << m_interruptIndex))) 
		{
			bool anotherInterruptPending{false};
			for(int i = 0; i <= 4; ++i)
			{
				if(m_pendingInterrupts & (1 << i) && m_interruptIndex != i)
				{
					m_interruptIndex = i;
					anotherInterruptPending = true;
					break;
				}
			}
			if(!anotherInterruptPending)
			{
				m_pc = 0;
				endInstruction();
			}
		}
		break;
	case 4:
		m_bus.write(--m_sp, getLSB(m_pc), Bus::Component::CPU);
		break;
	case 5:
		m_bus.write(hardwareReg::IF, m_pendingInterrupts & ~(1 << m_interruptIndex), Bus::Component::CPU);
		m_pc = interruptHandlerAddress[m_interruptIndex];
		endInstruction();
		
		break;
	}
}

void CPU::fetch()
{
	m_ir = m_bus.read(m_pc++, Bus::Component::CPU);
}

void CPU::execute()
{
	switch(m_ir) 
	{
		//8-bit load instructions
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

		//16-bit load instructions
	case 0x01: case 0x11: case 0x21: case 0x31:
		LD_rr_nn(); break;

	case 0x08: LD_nn_SP(); break;
	case 0xF9: LD_SP_HL(); break;

	case 0xC5: case 0xD5: case 0xE5: case 0xF5:
		PUSH_rr(); break;

	case 0xC1: case 0xD1: case 0xE1: case 0xF1:
		POP_rr(); break;

	case 0xF8: LD_HL_SP_e(); break;

		//8-bit arithmetic and logical instructions
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

		//16-bit arithmetic instructions
	case 0x03: case 0x13: case 0x23: case 0x33:
		INC_rr(); break;

	case 0x0B: case 0x1B: case 0x2B: case 0x3B:
		DEC_rr(); break;

	case 0x09: case 0x19: case 0x29: case 0x39:
		ADD_HL_rr(); break;

	case 0xE8: ADD_SP_e(); break;

		//rotate, shift and bit operations instructions
	case 0x07: RLCA(); break;
	case 0x0F: RRCA(); break;
	case 0x17: RLA(); break;
	case 0x1F: RRA(); break;

	case 0xCB:
	{
		fetch();
		switch(m_ir)
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
			 //control flow instructions
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

		//other
	case 0x76: HALT(); break;
	case 0x10: STOP(); break;
	case 0xF3: DI(); break;
	case 0xFB: EI(); break;
	case 0x00: NOP(); break;

	default:
		std::cerr << "Invalid opcode " << std::hex << static_cast<int>(m_ir) << " at PC: " << static_cast<int>(m_pc) << '\n';
		break;
	}
}

void CPU::endInstruction()
{
	m_cycleCounter = 0;
	m_currentInstr = nullptr;
}

uint8 CPU::getMSB(const uint16 in) const
{
	return in >> 8;
}

uint8 CPU::getLSB(const uint16 in) const
{
	return in & 0xFF;
}

uint16 CPU::getBC() const
{
	return (m_registers[B] << 8) | m_registers[C];
}

uint16 CPU::getDE() const
{
	return (m_registers[D] << 8) | m_registers[E];
}

uint16 CPU::getHL() const
{
	return (m_registers[H] << 8) | m_registers[L];
}

void CPU::setBC(const uint16 BC)
{
	m_registers[B] = getMSB(BC);
	m_registers[C] = getLSB(BC);
}

void CPU::setDE(const uint16 DE)
{
	m_registers[D] = getMSB(DE);
	m_registers[E] = getLSB(DE);
}

void CPU::setHL(const uint16 HL)
{
	m_registers[H] = getMSB(HL);
	m_registers[L] = getLSB(HL);
}

bool CPU::getFZ() const
{
	return static_cast<bool>(m_f & 0b1000'0000);
}

bool CPU::getFN() const
{
	return static_cast<bool>(m_f & 0b0100'0000);
}

bool CPU::getFH() const
{
	return static_cast<bool>(m_f & 0b0010'0000);
}

bool CPU::getFC() const
{
	return static_cast<bool>(m_f & 0b0001'0000);
}

void CPU::setFZ(bool z)
{
	if(z) m_f = m_f | 0b1000'0000;
	else  m_f = m_f & 0b0111'0000;
}

void CPU::setFN(bool n)
{
	if(n) m_f = m_f | 0b0100'0000;
	else  m_f = m_f & 0b1011'0000;
}

void CPU::setFH(bool h)
{
	if(h) m_f = m_f | 0b0010'0000;
	else  m_f = m_f & 0b1101'0000;
} 

void CPU::setFC(bool c)
{
	if(c) m_f = m_f | 0b0001'0000;
	else  m_f = m_f & 0b1110'0000;
}

void CPU::LD_r_r2()
{
	m_iState.x = (m_ir >> 3) & 0b111; //r
	m_iState.y = m_ir & 0b111; //r2

	m_registers[m_iState.x] = m_registers[m_iState.y];
	m_cycleCounter = 0;
}

void CPU::LD_r_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_r_n;
		m_iState.x = (m_ir >> 3) & 0b111; //r
		break;
	case 2:
		m_iState.y = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_registers[m_iState.x] = m_iState.y;
		endInstruction();
		break;
	}
}

void CPU::LD_r_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_r_HL;
		m_iState.x = (m_ir >> 3) & 0b111; //r
		break;
	case 2:
		m_registers[m_iState.x] = m_bus.read(getHL(), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_HL_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_HL_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_bus.write(getHL(), m_registers[m_iState.x], Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_HL_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_HL_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		break;
	case 3:
		m_bus.write(getHL(), m_iState.x, Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_A_BC()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_A_BC;
		break;
	case 2:
		m_registers[A] = m_bus.read(getBC(), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_A_DE()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_A_DE;
		break;
	case 2:
		m_registers[A] = m_bus.read(getDE(), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_BC_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_BC_A;
		break;
	case 2:
		m_bus.write(getBC(), m_registers[A], Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_DE_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_DE_A;
		break;
	case 2:
		m_bus.write(getDE(), m_registers[A], Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_A_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_A_nn;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		break;
	case 4:
		m_registers[A] = m_bus.read(m_iState.xx, Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_nn_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_nn_A;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		break;
	case 4:
		m_bus.write(m_iState.xx, m_registers[A], Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LDH_A_C()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LDH_A_C;
		break;
	case 2:
		m_registers[A] = m_bus.read(0xFF00 | m_registers[C], Bus::Component::CPU); //xx is 0xFF00 as the high byte + C as the low byte
		endInstruction();	
		break;
	}
}

void CPU::LDH_C_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LDH_C_A;
		break;
	case 2:
		m_bus.write(0xFF00 | m_registers[C], m_registers[A], Bus::Component::CPU);
		endInstruction();		
		break;
	}
}

void CPU::LDH_A_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LDH_A_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		break;
	case 3:
		m_registers[A] = m_bus.read(0xFF00 | m_iState.x, Bus::Component::CPU);
		endInstruction();		
		break;
	}
}

void CPU::LDH_n_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LDH_n_A;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		break;
	case 3:
		m_bus.write(0xFF00 | m_iState.x, m_registers[A], Bus::Component::CPU);
		endInstruction();		
		break;
	}
}

void CPU::LD_A_HLd()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_A_HLd;
		break;
	case 2:
		m_registers[A] = m_bus.read(getHL(), Bus::Component::CPU);
		if(--m_registers[L] == 0xFF) --m_registers[H]; //check for underflow
		endInstruction();
		break;
	}
}

void CPU::LD_HLd_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_HLd_A;
		break;
	case 2:
		
		m_bus.write(getHL(), m_registers[A], Bus::Component::CPU);
		if(--m_registers[L] == 0xFF) --m_registers[H];
		endInstruction();
		break;
	}
}

void CPU::LD_A_HLi()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_A_HLi;
		break;
	case 2:
		m_registers[A] = m_bus.read(getHL(), Bus::Component::CPU);
		if(++m_registers[L] == 0x00) ++m_registers[H]; //check for overflow
		endInstruction();
		break;
	}
}

void CPU::LD_HLi_A()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_HLi_A;
		break;
	case 2:
		m_bus.write(getHL(), m_registers[A], Bus::Component::CPU);
		if(++m_registers[L] == 0x00) ++m_registers[H];
		endInstruction();
		break;
	}
}

void CPU::LD_rr_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_rr_nn;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		switch(m_iState.x)
		{
		case BC: setBC(m_iState.xx); break;
		case DE: setDE(m_iState.xx); break;
		case HL: setHL(m_iState.xx); break;
		case SP: m_sp = m_iState.xx; break;
		}
		endInstruction();
		break;
	}
}

void CPU::LD_nn_SP()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_nn_SP;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		break;
	case 4:
		m_bus.write(m_iState.xx, getLSB(m_sp), Bus::Component::CPU);
		break;
	case 5:
		m_bus.write(m_iState.xx + 1, getMSB(m_sp), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::LD_SP_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_SP_HL;
		break;
	case 2:
		m_sp = getHL();
		endInstruction();
		break;
	}
}

void CPU::PUSH_rr()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::PUSH_rr;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		break;
	case 2:
		break;
	case 3:
		switch(m_iState.x)
		{
		case BC: m_bus.write(--m_sp, m_registers[B], Bus::Component::CPU); break;
		case DE: m_bus.write(--m_sp, m_registers[D], Bus::Component::CPU); break;
		case HL: m_bus.write(--m_sp, m_registers[H], Bus::Component::CPU); break;
		case AF: m_bus.write(--m_sp, m_registers[A], Bus::Component::CPU); break;
		}
		break;
	case 4:
		switch(m_iState.x)
		{
		case BC: m_bus.write(--m_sp, m_registers[C], Bus::Component::CPU); break;
		case DE: m_bus.write(--m_sp, m_registers[E], Bus::Component::CPU); break;
		case HL: m_bus.write(--m_sp, m_registers[L], Bus::Component::CPU); break;
		case AF: m_bus.write(--m_sp, m_f & 0xF0, Bus::Component::CPU); break;
		}
		endInstruction();
		break;
	}
}

void CPU::POP_rr()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::POP_rr;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_sp++, Bus::Component::CPU) << 8;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		switch(m_iState.x)
		{
		case BC: setBC(m_iState.xx); break;
		case DE: setDE(m_iState.xx); break;
		case HL: setHL(m_iState.xx); break;
		case AF: 
			m_registers[A] = getMSB(m_iState.xx); 
			m_f = getLSB(m_iState.xx) & 0xF0; 
			break;
		}
		endInstruction();
		break;
	}
}

void CPU::LD_HL_SP_e()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::LD_HL_SP_e;
		break;
	case 2:
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, Bus::Component::CPU)); //e
		break;
	case 3:
		setHL(static_cast<uint16>(m_sp + m_iState.e));

		setFZ(false);
		setFN(false);
		setFH(((m_sp & 0xF) + (static_cast<uint8>(m_iState.e) & 0xF)) & 0x10);
		setFC(((m_sp & 0xFF) + static_cast<uint8>(m_iState.e)) & 0x100);

		endInstruction();
		break;
	}
}

void CPU::ADD_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[A] + m_registers[m_iState.x]; //result

	setFZ((m_iState.xx & 0xFF) == 0);
	setFN(false);
	setFH(((m_registers[A] & 0xF) + (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	m_cycleCounter = 0;
}

void CPU::ADD_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADD_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		m_iState.xx = m_registers[A] + m_iState.x; //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::ADD_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADD_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_iState.xx = m_registers[A] + m_iState.x; //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::ADC_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[A] + m_registers[m_iState.x] + getFC(); //result

	setFZ((m_iState.xx & 0xFF) == 0);
	setFN(false);
	setFH(((m_registers[A] & 0xF) + (m_registers[m_iState.x] & 0xF) + (getFC())) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	m_cycleCounter = 0;
}

void CPU::ADC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADC_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		m_iState.xx = m_registers[A] + m_iState.x + getFC(); //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF) + (getFC())) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::ADC_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADC_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_iState.xx = m_registers[A] + m_iState.x + getFC(); //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(false);
		setFH(((m_registers[A] & 0xF) + (m_iState.x & 0xF) + (getFC())) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SUB_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[A] - m_registers[m_iState.x]; //result

	setFZ((m_iState.xx & 0xFF) == 0);
	setFN(true);
	setFH(((m_registers[A] & 0xF) - (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	m_cycleCounter = 0;
}

void CPU::SUB_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SUB_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		m_iState.xx = m_registers[A] - m_iState.x; //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SUB_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SUB_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_iState.xx = m_registers[A] - m_iState.x; //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF)) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SBC_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[A] - m_registers[m_iState.x] - getFC(); //result

	setFZ((m_iState.xx & 0xFF) == 0);
	setFN(true);
	setFH(((m_registers[A] & 0xF) - (m_registers[m_iState.x] & 0xF) - (getFC())) & 0x10);
	setFC(m_iState.xx & 0x100);

	m_registers[A] = static_cast<uint8>(m_iState.xx);
	m_cycleCounter = 0;
}

void CPU::SBC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SBC_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		m_iState.xx = m_registers[A] - m_iState.x - getFC(); //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF) - getFC()) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SBC_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SBC_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_iState.xx = m_registers[A] - m_iState.x - getFC(); //result

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH(((m_registers[A] & 0xF) - (m_iState.x & 0xF) - (getFC())) & 0x10);
		setFC(m_iState.xx & 0x100);

		m_registers[A] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::CP_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[A] - m_registers[m_iState.x];

	setFZ((m_iState.xx & 0xFF) == 0);
	setFN(true);
	setFH(((m_registers[A] & 0xF) - (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFC(m_iState.xx & 0x100);
	m_cycleCounter = 0;
}

void CPU::CP_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::CP_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		m_iState.xx = m_registers[A] - m_iState.x;

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) - (m_iState.x & 0xF) & 0x10);
		setFC(m_iState.xx & 0x100);

		endInstruction();
		break;
	}
}

void CPU::CP_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::CP_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n
		m_iState.xx = m_registers[A] - m_iState.x;

		setFZ((m_iState.xx & 0xFF) == 0);
		setFN(true);
		setFH((m_registers[A] & 0xF) - (m_iState.x & 0xF) & 0x10);
		setFC(m_iState.xx & 0x100);

		endInstruction();
		break;
	}
}

void CPU::INC_r()
{
	m_iState.x = (m_ir >> 3) & 0b111; //r

	setFZ(m_registers[m_iState.x] == 0xFF);
	setFN(false);
	setFH((m_registers[m_iState.x] & 0xF) == 0xF);

	++m_registers[m_iState.x];
	m_cycleCounter = 0;
}

void CPU::INC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::INC_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 3:
		m_bus.write(getHL(), m_iState.x + 1, Bus::Component::CPU);

		setFZ(m_iState.x == 0xFF);
		setFN(false);
		setFH((m_iState.x & 0xF) == 0xF);

		endInstruction();
		break;
	}
}

void CPU::DEC_r()
{
	m_iState.x = (m_ir >> 3) & 0b111; //r

	setFZ((m_registers[m_iState.x] - 1) == 0);
	setFN(true);
	setFH(((m_registers[m_iState.x] & 0xF) - 1) & 0x10);

	--m_registers[m_iState.x];
	m_cycleCounter = 0;
}

void CPU::DEC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::DEC_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 3:
		m_bus.write(getHL(), m_iState.x - 1, Bus::Component::CPU);

		setFZ((m_iState.x - 1) == 0);
		setFN(true);
		setFH(((m_iState.x & 0xF) - 1) & 0x10);

		endInstruction();
		break;
	}
}

void CPU::AND_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[A] &= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(true);
	setFC(false);
	m_cycleCounter = 0;
}

void CPU::AND_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::AND_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem

		m_registers[A] &= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(true);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::AND_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::AND_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n

		m_registers[A] &= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(true);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::OR_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[A] |= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(false);
	setFC(false);
	m_cycleCounter = 0;
}

void CPU::OR_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::OR_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem

		m_registers[A] |= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::OR_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::OR_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n

		m_registers[A] |= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::XOR_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[A] ^= m_registers[m_iState.x];

	setFZ(m_registers[A] == 0);
	setFN(false);
	setFH(false);
	setFC(false);
	m_cycleCounter = 0;
}

void CPU::XOR_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::XOR_HL;
		break;
	case 2:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem

		m_registers[A] ^= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::XOR_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::XOR_n;
		break;
	case 2:
		m_iState.x = m_bus.read(m_pc++, Bus::Component::CPU); //n

		m_registers[A] ^= m_iState.x;

		setFZ(m_registers[A] == 0);
		setFN(false);
		setFH(false);
		setFC(false);

		endInstruction();
		break;
	}
}

void CPU::DAA()
{
	m_iState.x = m_registers[A];

	if(!getFN())
	{
		if(getFC() || m_iState.x > 0x99) 
		{
			m_iState.x += 0x60;
			setFC(true);
		}
		if(getFH() || (m_iState.x & 0xF) > 0x09) m_iState.x += 0x06;
	}
	else
	{
		if(getFC()) m_iState.x -= 0x60;
		if(getFH()) m_iState.x -= 0x06;
	}

	m_registers[A] = m_iState.x;
	setFZ(m_registers[A] == 0);
	setFH(false);
	m_cycleCounter = 0;
}

void CPU::CPL()
{
	m_registers[A] = ~m_registers[A];
	setFN(true);
	setFH(true);
	m_cycleCounter = 0;
}

void CPU::CCF()
{
	setFN(false);
	setFH(false);
	setFC(!getFC());
	m_cycleCounter = 0;
}

void CPU::SCF()
{
	setFN(false);
	setFH(false);
	setFC(true);
	m_cycleCounter = 0;
}

void CPU::INC_rr()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::INC_rr;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		break;
	case 2:
		switch(m_iState.x) 
		{
		case BC: setBC(static_cast<uint16>(getBC() + 1)); break;
		case DE: setDE(static_cast<uint16>(getDE() + 1)); break;
		case HL: setHL(static_cast<uint16>(getHL() + 1)); break;
		case SP: ++m_sp; break;
		}
		endInstruction();
		break;
	}
}

void CPU::DEC_rr()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::DEC_rr;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		break;
	case 2:
		switch(m_iState.x)
		{
		case BC: setBC(static_cast<uint16>(getBC() - 1)); break;
		case DE: setDE(static_cast<uint16>(getDE() - 1)); break;
		case HL: setHL(static_cast<uint16>(getHL() - 1)); break;
		case SP: --m_sp; break;
		}
		endInstruction();
		break;
	}
}

void CPU::ADD_HL_rr()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADD_HL_rr;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		break;
	case 2:
		switch(m_iState.x) //m_iState.xx is register rr value in this switch
		{
		case BC: m_iState.xx = getBC(); break;
		case DE: m_iState.xx = getDE(); break;
		case HL: m_iState.xx = getHL(); break;
		case SP: m_iState.xx = m_sp; break;
		}
	
		uint32 result{static_cast<uint32>(getHL() + m_iState.xx)}; //result

		setFN(false);
		setFH(((getHL() & 0xFFF) + (m_iState.xx & 0xFFF)) & 0x1000);
		setFC(result & 0x10000);

		setHL(static_cast<uint16>(result));

		endInstruction();
		break;
	}
}

void CPU::ADD_SP_e()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::ADD_SP_e;
		break;
	case 2:
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, Bus::Component::CPU)); //e
		break;
	case 3:
		break;
	case 4:
		setFZ(false);
		setFN(false);
		setFH((m_sp & 0xF) + (static_cast<uint8>(m_iState.e) & 0xF) & 0x10);
		setFC(((m_sp & 0xFF) + static_cast<uint8>(m_iState.e)) & 0x100);

		m_sp = static_cast<uint16>(m_sp + m_iState.e);
		endInstruction();
		break;
	}
}

void CPU::RLCA()
{
	m_iState.x = m_registers[A] >> 7; //bit out

	m_registers[A] = (m_registers[A] << 1) | m_iState.x; //shift then add the carried bit to rotate circularly

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RRCA()
{
	m_iState.x = (m_registers[A] & 1) << 7; //bit out
	m_registers[A] = (m_registers[A] >> 1) | m_iState.x;

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RLA()
{
	m_iState.x = m_registers[A] >> 7; //bit out
	m_registers[A] = (m_registers[A] << 1) | static_cast<uint8>(getFC());

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RRA()
{
	m_iState.x = m_registers[A] & 1; //bit out
	m_registers[A] = (m_registers[A] >> 1) | (static_cast<uint8>(getFC()) << 7);

	setFZ(false);
	setFN(false);
	setFH(false);
	setFC(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RLC_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RLC_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] << 1) | m_iState.y;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RLC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RLC_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | m_iState.y; //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RRC_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RRC_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = (m_registers[m_iState.x] & 1) << 7; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | m_iState.y;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RRC_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RRC_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = (m_iState.x & 1) << 7; //bit out
		m_iState.z = (m_iState.x >> 1) | m_iState.y; //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RL_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RL_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out
		m_registers[m_iState.x] = (m_registers[m_iState.x] << 1) | static_cast<uint8>(getFC());

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RL_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RL_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | static_cast<uint8>(getFC()); //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RR_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RR_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | (static_cast<uint8>(getFC()) << 7);

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::RR_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RR_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = (m_iState.x >> 1) | (static_cast<uint8>(getFC()) << 7); //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::SLA_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SLA_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] >> 7; //bit out

		m_registers[m_iState.x] = m_registers[m_iState.x] << 1;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::SLA_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SLA_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = m_iState.x << 1; //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::SRA_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SRA_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | (m_registers[m_iState.x] & 0x80); //registers[x] & 0x80 = sign bit

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::SRA_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SRA_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU);
		break;
	case 4:
		m_iState.y = m_iState.x & 1; // bit out
		m_iState.z = (m_iState.x >> 1) | (m_iState.x & 0x80); //z = result, x & 0x80 = sign bit

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
	}
}

void CPU::SWAP_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SWAP_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r

		m_registers[m_iState.x] = ((m_registers[m_iState.x] & 0xF0) >> 4) | ((m_registers[m_iState.x] & 0x0F) << 4);

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(false);
		endInstruction();
		break;
	}
}

void CPU::SWAP_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SWAP_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = ((m_iState.x & 0xF0) >> 4) | ((m_iState.x & 0xF) << 4); //result

		m_bus.write(getHL(), m_iState.y, Bus::Component::CPU);

		setFZ(m_iState.y == 0);
		setFN(false);
		setFH(false);
		setFC(false);
		endInstruction();
		break;
	}
}

void CPU::SRL_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SRL_r;
		break;
	case 2:
		m_iState.x = m_ir & 0b111; //r
		m_iState.y = m_registers[m_iState.x] & 1; //bit out

		m_registers[m_iState.x] = m_registers[m_iState.x] >> 1;

		setFZ(m_registers[m_iState.x] == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::SRL_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SRL_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = m_iState.x >> 1; //result

		m_bus.write(getHL(), m_iState.z, Bus::Component::CPU);

		setFZ(m_iState.z == 0);
		setFN(false);
		setFH(false);
		setFC(m_iState.y);
		endInstruction();
		break;
	}
}

void CPU::BIT_b_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::BIT_b_r;
		break;
	case 2:
		m_iState.x = (m_ir >> 3) & 0b111; //b
		m_iState.y = m_ir & 0b0111; //r

		setFZ((m_registers[m_iState.y] & (1 << m_iState.x)) == 0); //shift the bit in the 0 place by b times to check the right bit
		setFN(false);
		setFH(true);
		endInstruction();
		break;
	}
}

void CPU::BIT_b_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::BIT_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = (m_ir >> 3) & 0b111; //b
		m_iState.y = m_bus.read(getHL(), Bus::Component::CPU); //HL mem

		setFZ((m_iState.y & (1 << m_iState.x)) == 0);
		setFN(false);
		setFH(true);
		endInstruction();
		break;
	}
}

void CPU::RES_b_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RES_b_r;
		break;
	case 2:
		m_iState.x = (m_ir >> 3) & 0b111; //b
		m_iState.y = m_ir & 0b111; //r

		m_registers[m_iState.y] &= ~(1 << m_iState.x);
		endInstruction();
	}
}

void CPU::RES_b_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RES_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = (m_ir >> 3) & 0b111; //b

		m_bus.write(getHL(), m_iState.x & ~(1 << m_iState.y), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::SET_b_r()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SET_b_r;
		break;
	case 2:
		m_iState.x = (m_ir >> 3) & 0b111; //b
		m_iState.y = m_ir & 0b111; //r

		m_registers[m_iState.y] |= (1 << m_iState.x);
		endInstruction();
		break;
	}
}

void CPU::SET_b_HL()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::SET_b_HL;
		break;
	case 2:
		break;
	case 3:
		m_iState.x = m_bus.read(getHL(), Bus::Component::CPU); //HL mem
		break;
	case 4:
		m_iState.y = (m_ir >> 3) & 0b111; //b

		m_bus.write(getHL(), m_iState.x | (1 << m_iState.y), Bus::Component::CPU);
		endInstruction();
		break;
	}
}

void CPU::JP_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::JP_nn;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		break;
	case 4:
		m_pc = m_iState.xx;
		endInstruction();
		break;
	}
}

void CPU::JP_HL()
{
	m_pc = getHL();
	m_cycleCounter = 0;
}

void CPU::JP_cc_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::JP_cc_nn;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		m_iState.x = (m_ir >> 3) & 0b11; //cc

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NOT_ZERO: m_iState.y = !getFZ(); break;
		case ZERO: m_iState.y = getFZ(); break;
		case NOT_CARRY: m_iState.y = !getFC(); break;
		case CARRY: m_iState.y = getFC(); break;
		}

		if(m_iState.y) m_pc = m_iState.xx;
		else endInstruction();
		break;
	case 4:
		endInstruction();
		break;
	}
}

void CPU::JR_e()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::JR_e;
		break;
	case 2:
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, Bus::Component::CPU)); //e
		break;
	case 3:
		m_pc += m_iState.e;
		endInstruction();
		break;
	}
}

void CPU::JR_cc_e()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::JR_cc_e;
		m_iState.x = (m_ir >> 3) & 0b11; //cc
		break;
	case 2:
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, Bus::Component::CPU)); //e

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NOT_ZERO: m_iState.y = !getFZ(); break;
		case ZERO: m_iState.y = getFZ(); break;
		case NOT_CARRY: m_iState.y = !getFC(); break;
		case CARRY: m_iState.y = getFC(); break;
		}

		if(m_iState.y) m_pc += m_iState.e;
		else endInstruction();
		break;
	case 3:
		endInstruction();
		break;
	}
}

void CPU::CALL_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::CALL_nn;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		break;
	case 4:
		break;
	case 5:
		m_bus.write(--m_sp, getMSB(m_pc), Bus::Component::CPU);
		break;
	case 6:
		m_bus.write(--m_sp, getLSB(m_pc), Bus::Component::CPU);
		m_pc = m_iState.xx;
		endInstruction();
		break;
	}
}

void CPU::CALL_cc_nn()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::CALL_cc_nn;
		break;
	case 2:
		m_iState.xx = m_bus.read(m_pc++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, Bus::Component::CPU) << 8; //nn
		m_iState.x = (m_ir >> 3) & 0b11; //cc

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case NOT_ZERO: m_iState.y = !getFZ(); break;
		case ZERO: m_iState.y = getFZ(); break;
		case NOT_CARRY: m_iState.y = !getFC(); break;
		case CARRY: m_iState.y = getFC(); break;
		}

		if(!m_iState.y) endInstruction();
		break;
	case 4:
		break;
	case 5:
		m_bus.write(--m_sp, getMSB(m_pc), Bus::Component::CPU);
		break;
	case 6:
		m_bus.write(--m_sp, getLSB(m_pc), Bus::Component::CPU);
		m_pc = m_iState.xx;
		endInstruction();
		break;
	}
}

void CPU::RET()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RET;
		break;
	case 2:
		m_iState.x = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.y = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 4:
		m_pc = (m_iState.y << 8) | m_iState.x;
		endInstruction();
		break;
	}
}

void CPU::RET_cc()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RET_cc;
		m_iState.x = (m_ir >> 3) & 0b11; //cc
		break;
	case 2:
		switch(m_iState.x)
		{
		case NOT_ZERO: m_iState.y = !getFZ(); break;
		case ZERO: m_iState.y = getFZ(); break;
		case NOT_CARRY: m_iState.y = !getFC(); break;
		case CARRY: m_iState.y = getFC(); break;
		}

		if(!m_iState.y) endInstruction();
		break;
	case 3:
		m_iState.x = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 4:
		m_iState.y = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 5:
		m_pc = (m_iState.y << 8) | m_iState.x;
		endInstruction();
		break;
	}
}

void CPU::RETI()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RETI;
		break;
	case 2:
		m_iState.x = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 3:
		m_iState.y = m_bus.read(m_sp++, Bus::Component::CPU);
		break;
	case 4:
		m_pc = (m_iState.y << 8) | m_iState.x;
		m_ime = true;
		endInstruction();
		break;
	}
}

void CPU::RST_n()
{
	switch(m_cycleCounter)
	{
	case 1:
		m_currentInstr = &CPU::RST_n;
		break;
	case 2:
		break;
	case 3:
		m_bus.write(--m_sp, getMSB(m_pc), Bus::Component::CPU);
		break;
	case 4:
		m_bus.write(--m_sp, getLSB(m_pc), Bus::Component::CPU);
		m_pc = m_ir & 0b111000;
		endInstruction();
		break;
	}
}

void CPU::HALT()
{
	m_isHalted = true;
	//if(!m_ime && (m_bus.read(hardwareReg::IF) & m_bus.read(hardwareReg::IE)) != 0) ++m_pc; //HALT bug(its not correct)
	m_cycleCounter = 0;
}

void CPU::STOP()
{
	//TODO
	m_cycleCounter = 0;
}

void CPU::DI()
{
	m_ime = false;
	m_imeEnableNextCycle = false;
	m_cycleCounter = 0;
}

void CPU::EI()
{
	if(!m_imeEnableNextCycle && !m_ime) m_imeEnableNextCycle = true;
	m_cycleCounter = 0;
}

void CPU::NOP()
{
	m_cycleCounter = 0; //nop
}