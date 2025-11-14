#include "core/cpu.h"
#include "hardware_registers.h"
#include "core/mmu.h"
#include <iostream>

CPU::CPU(MMU& mmu)
	: m_bus{mmu}
	, m_iState{}
	, m_currentInstr{}
	, m_cycleCounter{}
	, m_ime{}
	, m_imeEnableNextCycle{}
	, m_halted{}
	, m_haltBug{}
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
	m_halted = false;
	m_pendingInterrupts = 0;
	m_interruptIndex = 0;
	m_pc = 0x100;
	m_sp = 0xFFFE;
	m_f = 0xB0;
	m_ir = 0;
	m_registers[b] = 0x00;
	m_registers[c] = 0x13;
	m_registers[d] = 0x00;
	m_registers[e] = 0xD8;
	m_registers[h] = 0x01;
	m_registers[l] = 0x4D;
	m_registers[a] = 0x01;
}

void CPU::mCycle()
{
	++m_cycleCounter;//since instructions reset m_cycleCounter to 0 increment before the cpu cycle so its 1, then if the instruction is multi-cycle 2, 3...
	if(!m_currentInstr) handleInterrupts();
	if(m_halted) 
	{
		m_cycleCounter = 0;
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
	m_pendingInterrupts = (m_bus.read(hardwareReg::IF, MMU::Component::cpu) & m_bus.read(hardwareReg::IE, MMU::Component::cpu)) & 0b1'1111; //only bits 0-4 are used
	if(!m_pendingInterrupts) return;
	
	m_halted = false; //even if m_ime is false exit halt
	if(!m_ime) return;
	
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
		m_bus.write(--m_sp, getMsb(m_pc), MMU::Component::cpu);
		//here if the interrupt currently dispatching gets disabled it checks if 
		//there's another one to continue the dispatching with and if not it cancels the dispatching
		if(m_sp == hardwareReg::IE && !(getMsb(m_pc) & (1 << m_interruptIndex))) 
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
		m_bus.write(--m_sp, getLsb(m_pc), MMU::Component::cpu);
		break;
	case 5:
		m_bus.write(hardwareReg::IF, m_pendingInterrupts & ~(1 << m_interruptIndex), MMU::Component::cpu);
		m_pc = interruptHandlerAddress[m_interruptIndex];
		endInstruction();
		
		break;
	}
}

void CPU::fetch()
{
	m_ir = m_bus.read(m_pc++, MMU::Component::cpu);
	if(m_haltBug)
	{
		--m_pc;
		m_haltBug = false;
	}
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

uint8 CPU::getMsb(const uint16 in) const
{
	return in >> 8;
}

uint8 CPU::getLsb(const uint16 in) const
{
	return in & 0xFF;
}

uint16 CPU::getBc() const
{
	return (m_registers[b] << 8) | m_registers[c];
}

uint16 CPU::getDe() const
{
	return (m_registers[d] << 8) | m_registers[e];
}

uint16 CPU::getHl() const
{
	return (m_registers[h] << 8) | m_registers[l];
}

void CPU::setBc(const uint16 Bc)
{
	m_registers[b] = getMsb(Bc);
	m_registers[c] = getLsb(Bc);
}

void CPU::setDe(const uint16 De)
{
	m_registers[d] = getMsb(De);
	m_registers[e] = getLsb(De);
}

void CPU::setHl(const uint16 Hl)
{
	m_registers[h] = getMsb(Hl);
	m_registers[l] = getLsb(Hl);
}

bool CPU::getFz() const
{
	return static_cast<bool>(m_f & zeroFlag);
}

bool CPU::getFn() const
{
	return static_cast<bool>(m_f & negativeFlag);
}

bool CPU::getFh() const
{
	return static_cast<bool>(m_f & halfCarryFlag);
}

bool CPU::getFc() const
{
	return static_cast<bool>(m_f & carryFlag);
}

void CPU::setFz(bool z)
{
	if(z) m_f = m_f | zeroFlag;
	else  m_f = m_f & (~zeroFlag);
}

void CPU::setFn(bool n)
{
	if(n) m_f = m_f | negativeFlag;
	else  m_f = m_f & (~negativeFlag);
}

void CPU::setFh(bool h)
{
	if(h) m_f = m_f | halfCarryFlag;
	else  m_f = m_f & (~halfCarryFlag);
} 

void CPU::setFc(bool c)
{
	if(c) m_f = m_f | carryFlag;
	else  m_f = m_f & (~carryFlag);
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
		m_iState.y = m_bus.read(m_pc++, MMU::Component::cpu); //n
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
		m_registers[m_iState.x] = m_bus.read(getHl(), MMU::Component::cpu);
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
		m_bus.write(getHl(), m_registers[m_iState.x], MMU::Component::cpu);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		break;
	case 3:
		m_bus.write(getHl(), m_iState.x, MMU::Component::cpu);
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
		m_registers[a] = m_bus.read(getBc(), MMU::Component::cpu);
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
		m_registers[a] = m_bus.read(getDe(), MMU::Component::cpu);
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
		m_bus.write(getBc(), m_registers[a], MMU::Component::cpu);
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
		m_bus.write(getDe(), m_registers[a], MMU::Component::cpu);
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		break;
	case 4:
		m_registers[a] = m_bus.read(m_iState.xx, MMU::Component::cpu);
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		break;
	case 4:
		m_bus.write(m_iState.xx, m_registers[a], MMU::Component::cpu);
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
		m_registers[a] = m_bus.read(0xFF00 | m_registers[c], MMU::Component::cpu); //xx is 0xFF00 as the high byte + C as the low byte
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
		m_bus.write(0xFF00 | m_registers[c], m_registers[a], MMU::Component::cpu);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		break;
	case 3:
		m_registers[a] = m_bus.read(0xFF00 | m_iState.x, MMU::Component::cpu);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		break;
	case 3:
		m_bus.write(0xFF00 | m_iState.x, m_registers[a], MMU::Component::cpu);
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
		m_registers[a] = m_bus.read(getHl(), MMU::Component::cpu);
		if(--m_registers[l] == 0xFF) --m_registers[h]; //check for underflow
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
		
		m_bus.write(getHl(), m_registers[a], MMU::Component::cpu);
		if(--m_registers[l] == 0xFF) --m_registers[h];
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
		m_registers[a] = m_bus.read(getHl(), MMU::Component::cpu);
		if(++m_registers[l] == 0x00) ++m_registers[h]; //check for overflow
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
		m_bus.write(getHl(), m_registers[a], MMU::Component::cpu);
		if(++m_registers[l] == 0x00) ++m_registers[h];
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		switch(m_iState.x)
		{
		case bc: setBc(m_iState.xx); break;
		case de: setDe(m_iState.xx); break;
		case hl: setHl(m_iState.xx); break;
		case sp: m_sp = m_iState.xx; break;
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		break;
	case 4:
		m_bus.write(m_iState.xx, getLsb(m_sp), MMU::Component::cpu);
		break;
	case 5:
		m_bus.write(m_iState.xx + 1, getMsb(m_sp), MMU::Component::cpu);
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
		m_sp = getHl();
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
		case bc: m_bus.write(--m_sp, m_registers[b], MMU::Component::cpu); break;
		case de: m_bus.write(--m_sp, m_registers[d], MMU::Component::cpu); break;
		case hl: m_bus.write(--m_sp, m_registers[h], MMU::Component::cpu); break;
		case af: m_bus.write(--m_sp, m_registers[a], MMU::Component::cpu); break;
		}
		break;
	case 4:
		switch(m_iState.x)
		{
		case bc: m_bus.write(--m_sp, m_registers[c], MMU::Component::cpu); break;
		case de: m_bus.write(--m_sp, m_registers[e], MMU::Component::cpu); break;
		case hl: m_bus.write(--m_sp, m_registers[l], MMU::Component::cpu); break;
		case af: m_bus.write(--m_sp, m_f & 0xF0, MMU::Component::cpu); break;
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
		m_iState.xx = m_bus.read(m_sp++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_sp++, MMU::Component::cpu) << 8;
		m_iState.x = (m_ir >> 4) & 0b11; //rr
		switch(m_iState.x)
		{
		case bc: setBc(m_iState.xx); break;
		case de: setDe(m_iState.xx); break;
		case hl: setHl(m_iState.xx); break;
		case af: 
			m_registers[a] = getMsb(m_iState.xx); 
			m_f = getLsb(m_iState.xx) & 0xF0; 
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
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, MMU::Component::cpu)); //e
		break;
	case 3:
		setHl(static_cast<uint16>(m_sp + m_iState.e));

		setFz(false);
		setFn(false);
		setFh(((m_sp & 0xF) + (static_cast<uint8>(m_iState.e) & 0xF)) & 0x10);
		setFc(((m_sp & 0xFF) + static_cast<uint8>(m_iState.e)) & 0x100);

		endInstruction();
		break;
	}
}

void CPU::ADD_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[a] + m_registers[m_iState.x]; //result

	setFz((m_iState.xx & 0xFF) == 0);
	setFn(false);
	setFh(((m_registers[a] & 0xF) + (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFc(m_iState.xx & 0x100);

	m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		m_iState.xx = m_registers[a] + m_iState.x; //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(false);
		setFh(((m_registers[a] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		m_iState.xx = m_registers[a] + m_iState.x; //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(false);
		setFh(((m_registers[a] & 0xF) + (m_iState.x & 0xF)) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::ADC_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[a] + m_registers[m_iState.x] + getFc(); //result

	setFz((m_iState.xx & 0xFF) == 0);
	setFn(false);
	setFh(((m_registers[a] & 0xF) + (m_registers[m_iState.x] & 0xF) + (getFc())) & 0x10);
	setFc(m_iState.xx & 0x100);

	m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		m_iState.xx = m_registers[a] + m_iState.x + getFc(); //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(false);
		setFh(((m_registers[a] & 0xF) + (m_iState.x & 0xF) + (getFc())) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		m_iState.xx = m_registers[a] + m_iState.x + getFc(); //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(false);
		setFh(((m_registers[a] & 0xF) + (m_iState.x & 0xF) + (getFc())) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SUB_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[a] - m_registers[m_iState.x]; //result

	setFz((m_iState.xx & 0xFF) == 0);
	setFn(true);
	setFh(((m_registers[a] & 0xF) - (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFc(m_iState.xx & 0x100);

	m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		m_iState.xx = m_registers[a] - m_iState.x; //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh(((m_registers[a] & 0xF) - (m_iState.x & 0xF)) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		m_iState.xx = m_registers[a] - m_iState.x; //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh(((m_registers[a] & 0xF) - (m_iState.x & 0xF)) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::SBC_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[a] - m_registers[m_iState.x] - getFc(); //result

	setFz((m_iState.xx & 0xFF) == 0);
	setFn(true);
	setFh(((m_registers[a] & 0xF) - (m_registers[m_iState.x] & 0xF) - (getFc())) & 0x10);
	setFc(m_iState.xx & 0x100);

	m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		m_iState.xx = m_registers[a] - m_iState.x - getFc(); //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh(((m_registers[a] & 0xF) - (m_iState.x & 0xF) - getFc()) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		m_iState.xx = m_registers[a] - m_iState.x - getFc(); //result

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh(((m_registers[a] & 0xF) - (m_iState.x & 0xF) - (getFc())) & 0x10);
		setFc(m_iState.xx & 0x100);

		m_registers[a] = static_cast<uint8>(m_iState.xx);
		endInstruction();
		break;
	}
}

void CPU::CP_r()
{
	m_iState.x = m_ir & 0b111; //r
	m_iState.xx = m_registers[a] - m_registers[m_iState.x];

	setFz((m_iState.xx & 0xFF) == 0);
	setFn(true);
	setFh(((m_registers[a] & 0xF) - (m_registers[m_iState.x] & 0xF)) & 0x10);
	setFc(m_iState.xx & 0x100);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		m_iState.xx = m_registers[a] - m_iState.x;

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh((m_registers[a] & 0xF) - (m_iState.x & 0xF) & 0x10);
		setFc(m_iState.xx & 0x100);

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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n
		m_iState.xx = m_registers[a] - m_iState.x;

		setFz((m_iState.xx & 0xFF) == 0);
		setFn(true);
		setFh((m_registers[a] & 0xF) - (m_iState.x & 0xF) & 0x10);
		setFc(m_iState.xx & 0x100);

		endInstruction();
		break;
	}
}

void CPU::INC_r()
{
	m_iState.x = (m_ir >> 3) & 0b111; //r

	setFz(m_registers[m_iState.x] == 0xFF);
	setFn(false);
	setFh((m_registers[m_iState.x] & 0xF) == 0xF);

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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 3:
		m_bus.write(getHl(), m_iState.x + 1, MMU::Component::cpu);

		setFz(m_iState.x == 0xFF);
		setFn(false);
		setFh((m_iState.x & 0xF) == 0xF);
		endInstruction();
		break;
	}
}

void CPU::DEC_r()
{
	m_iState.x = (m_ir >> 3) & 0b111; //r

	setFz((m_registers[m_iState.x] - 1) == 0);
	setFn(true);
	setFh(((m_registers[m_iState.x] & 0xF) - 1) & 0x10);

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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 3:
		m_bus.write(getHl(), m_iState.x - 1, MMU::Component::cpu);

		setFz((m_iState.x - 1) == 0);
		setFn(true);
		setFh(((m_iState.x & 0xF) - 1) & 0x10);

		endInstruction();
		break;
	}
}

void CPU::AND_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[a] &= m_registers[m_iState.x];

	setFz(m_registers[a] == 0);
	setFn(false);
	setFh(true);
	setFc(false);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem

		m_registers[a] &= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(true);
		setFc(false);

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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n

		m_registers[a] &= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(true);
		setFc(false);

		endInstruction();
		break;
	}
}

void CPU::OR_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[a] |= m_registers[m_iState.x];

	setFz(m_registers[a] == 0);
	setFn(false);
	setFh(false);
	setFc(false);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem

		m_registers[a] |= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(false);
		setFc(false);

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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n

		m_registers[a] |= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(false);
		setFc(false);

		endInstruction();
		break;
	}
}

void CPU::XOR_r()
{
	m_iState.x = m_ir & 0b111; //r

	m_registers[a] ^= m_registers[m_iState.x];

	setFz(m_registers[a] == 0);
	setFn(false);
	setFh(false);
	setFc(false);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem

		m_registers[a] ^= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(false);
		setFc(false);

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
		m_iState.x = m_bus.read(m_pc++, MMU::Component::cpu); //n

		m_registers[a] ^= m_iState.x;

		setFz(m_registers[a] == 0);
		setFn(false);
		setFh(false);
		setFc(false);

		endInstruction();
		break;
	}
}

void CPU::DAA()
{
	m_iState.x = m_registers[a];

	if(!getFn())
	{
		if(getFc() || m_iState.x > 0x99) 
		{
			m_iState.x += 0x60;
			setFc(true);
		}
		if(getFh() || (m_iState.x & 0xF) > 0x09) m_iState.x += 0x06;
	}
	else
	{
		if(getFc()) m_iState.x -= 0x60;
		if(getFh()) m_iState.x -= 0x06;
	}

	m_registers[a] = m_iState.x;
	setFz(m_registers[a] == 0);
	setFh(false);
	m_cycleCounter = 0;
}

void CPU::CPL()
{
	m_registers[a] = ~m_registers[a];
	setFn(true);
	setFh(true);
	m_cycleCounter = 0;
}

void CPU::CCF()
{
	setFn(false);
	setFh(false);
	setFc(!getFc());
	m_cycleCounter = 0;
}

void CPU::SCF()
{
	setFn(false);
	setFh(false);
	setFc(true);
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
		case bc: setBc(static_cast<uint16>(getBc() + 1)); break;
		case de: setDe(static_cast<uint16>(getDe() + 1)); break;
		case hl: setHl(static_cast<uint16>(getHl() + 1)); break;
		case sp: ++m_sp; break;
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
		case bc: setBc(static_cast<uint16>(getBc() - 1)); break;
		case de: setDe(static_cast<uint16>(getDe() - 1)); break;
		case hl: setHl(static_cast<uint16>(getHl() - 1)); break;
		case sp: --m_sp; break;
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
		case bc: m_iState.xx = getBc(); break;
		case de: m_iState.xx = getDe(); break;
		case hl: m_iState.xx = getHl(); break;
		case sp: m_iState.xx = m_sp; break;
		}
	
		uint32 result{static_cast<uint32>(getHl() + m_iState.xx)}; //result

		setFn(false);
		setFh(((getHl() & 0xFFF) + (m_iState.xx & 0xFFF)) & 0x1000);
		setFc(result & 0x10000);

		setHl(static_cast<uint16>(result));

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
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, MMU::Component::cpu)); //e
		break;
	case 3:
		break;
	case 4:
		setFz(false);
		setFn(false);
		setFh((m_sp & 0xF) + (static_cast<uint8>(m_iState.e) & 0xF) & 0x10);
		setFc(((m_sp & 0xFF) + static_cast<uint8>(m_iState.e)) & 0x100);

		m_sp = static_cast<uint16>(m_sp + m_iState.e);
		endInstruction();
		break;
	}
}

void CPU::RLCA()
{
	m_iState.x = m_registers[a] >> 7; //bit out

	m_registers[a] = (m_registers[a] << 1) | m_iState.x; //shift then add the carried bit to rotate circularly

	setFz(false);
	setFn(false);
	setFh(false);
	setFc(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RRCA()
{
	m_iState.x = (m_registers[a] & 1) << 7; //bit out
	m_registers[a] = (m_registers[a] >> 1) | m_iState.x;

	setFz(false);
	setFn(false);
	setFh(false);
	setFc(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RLA()
{
	m_iState.x = m_registers[a] >> 7; //bit out
	m_registers[a] = (m_registers[a] << 1) | static_cast<uint8>(getFc());

	setFz(false);
	setFn(false);
	setFh(false);
	setFc(m_iState.x);
	m_cycleCounter = 0;
}

void CPU::RRA()
{
	m_iState.x = m_registers[a] & 1; //bit out
	m_registers[a] = (m_registers[a] >> 1) | (static_cast<uint8>(getFc()) << 7);

	setFz(false);
	setFn(false);
	setFh(false);
	setFc(m_iState.x);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | m_iState.y; //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = (m_iState.x & 1) << 7; //bit out
		m_iState.z = (m_iState.x >> 1) | m_iState.y; //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_registers[m_iState.x] = (m_registers[m_iState.x] << 1) | static_cast<uint8>(getFc());

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = (m_iState.x << 1) | static_cast<uint8>(getFc()); //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		m_registers[m_iState.x] = (m_registers[m_iState.x] >> 1) | (static_cast<uint8>(getFc()) << 7);

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = (m_iState.x >> 1) | (static_cast<uint8>(getFc()) << 7); //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x >> 7; //bit out
		m_iState.z = m_iState.x << 1; //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu);
		break;
	case 4:
		m_iState.y = m_iState.x & 1; // bit out
		m_iState.z = (m_iState.x >> 1) | (m_iState.x & 0x80); //z = result, x & 0x80 = sign bit

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(false);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = ((m_iState.x & 0xF0) >> 4) | ((m_iState.x & 0xF) << 4); //result

		m_bus.write(getHl(), m_iState.y, MMU::Component::cpu);

		setFz(m_iState.y == 0);
		setFn(false);
		setFh(false);
		setFc(false);
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

		setFz(m_registers[m_iState.x] == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = m_iState.x & 1; //bit out
		m_iState.z = m_iState.x >> 1; //result

		m_bus.write(getHl(), m_iState.z, MMU::Component::cpu);

		setFz(m_iState.z == 0);
		setFn(false);
		setFh(false);
		setFc(m_iState.y);
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

		setFz((m_registers[m_iState.y] & (1 << m_iState.x)) == 0); //shift the bit in the 0 place by b times to check the right bit
		setFn(false);
		setFh(true);
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
		m_iState.y = m_bus.read(getHl(), MMU::Component::cpu); //HL mem

		setFz((m_iState.y & (1 << m_iState.x)) == 0);
		setFn(false);
		setFh(true);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = (m_ir >> 3) & 0b111; //b

		m_bus.write(getHl(), m_iState.x & ~(1 << m_iState.y), MMU::Component::cpu);
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
		m_iState.x = m_bus.read(getHl(), MMU::Component::cpu); //HL mem
		break;
	case 4:
		m_iState.y = (m_ir >> 3) & 0b111; //b

		m_bus.write(getHl(), m_iState.x | (1 << m_iState.y), MMU::Component::cpu);
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		break;
	case 4:
		m_pc = m_iState.xx;
		endInstruction();
		break;
	}
}

void CPU::JP_HL()
{
	m_pc = getHl();
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		m_iState.x = (m_ir >> 3) & 0b11; //cc

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case notZero: m_iState.y = !getFz(); break;
		case zero: m_iState.y = getFz(); break;
		case notCarry: m_iState.y = !getFc(); break;
		case carry: m_iState.y = getFc(); break;
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
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, MMU::Component::cpu)); //e
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
		m_iState.e = static_cast<int8>(m_bus.read(m_pc++, MMU::Component::cpu)); //e

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case notZero: m_iState.y = !getFz(); break;
		case zero: m_iState.y = getFz(); break;
		case notCarry: m_iState.y = !getFc(); break;
		case carry: m_iState.y = getFc(); break;
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		break;
	case 4:
		break;
	case 5:
		m_bus.write(--m_sp, getMsb(m_pc), MMU::Component::cpu);
		break;
	case 6:
		m_bus.write(--m_sp, getLsb(m_pc), MMU::Component::cpu);
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
		m_iState.xx = m_bus.read(m_pc++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.xx |= m_bus.read(m_pc++, MMU::Component::cpu) << 8; //nn
		m_iState.x = (m_ir >> 3) & 0b11; //cc

		switch(m_iState.x) //m_iState.y is used as a bool to check later if condition is met
		{
		case notZero: m_iState.y = !getFz(); break;
		case zero: m_iState.y = getFz(); break;
		case notCarry: m_iState.y = !getFc(); break;
		case carry: m_iState.y = getFc(); break;
		}

		if(!m_iState.y) endInstruction();
		break;
	case 4:
		break;
	case 5:
		m_bus.write(--m_sp, getMsb(m_pc), MMU::Component::cpu);
		break;
	case 6:
		m_bus.write(--m_sp, getLsb(m_pc), MMU::Component::cpu);
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
		m_iState.x = m_bus.read(m_sp++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.y = m_bus.read(m_sp++, MMU::Component::cpu);
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
		case notZero: m_iState.y = !getFz(); break;
		case zero: m_iState.y = getFz(); break;
		case notCarry: m_iState.y = !getFc(); break;
		case carry: m_iState.y = getFc(); break;
		}

		if(!m_iState.y) endInstruction();
		break;
	case 3:
		m_iState.x = m_bus.read(m_sp++, MMU::Component::cpu);
		break;
	case 4:
		m_iState.y = m_bus.read(m_sp++, MMU::Component::cpu);
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
		m_iState.x = m_bus.read(m_sp++, MMU::Component::cpu);
		break;
	case 3:
		m_iState.y = m_bus.read(m_sp++, MMU::Component::cpu);
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
		m_bus.write(--m_sp, getMsb(m_pc), MMU::Component::cpu);
		break;
	case 4:
		m_bus.write(--m_sp, getLsb(m_pc), MMU::Component::cpu);
		m_pc = m_ir & 0b111000;
		endInstruction();
		break;
	}
}

void CPU::HALT()
{
	m_cycleCounter = 0;
	if(!m_ime && ((m_bus.read(hardwareReg::IF, MMU::Component::cpu) & m_bus.read(hardwareReg::IE, MMU::Component::cpu)) & 0b1'1111))
	{
		m_haltBug = true;
		return;
	}
	m_halted = true;
}

void CPU::STOP()
{
	//TODO
	std::cout << "Stop instruction not implemented\n";
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