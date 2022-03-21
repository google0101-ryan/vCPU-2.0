#include <CPU.hpp>
#include <fstream>

void CPU::mi()
{
    flags.IF = false;
    printf("[CPU%d]: MI\n", id);
}

void CPU::m_r_imm()
{
    uint8_t index = Read8(pc++);
    uint32_t imm = Read32(pc);
    pc += 4;

    regs[index] = imm;

    printf("[CPU%d]: M r%d, 0x%08X\n", id, index, imm);
}

void CPU::mtsr()
{
    sr[regs[0]] = regs[1];
    printf("[CPU%d]: MTSR sr%d, 0x%08X\n", id, regs[0], regs[1]);
}

void CPU::j_reg()
{
    uint8_t index = Read8(pc++);
    pc = regs[index];

    printf("[CPU%d]: J r%d\n", id, index);
}

void CPU::umi()
{
    flags.IF = true;
    if (Read32(sr[2]))
    {
        printf("[CPU%d]: Invalid SR1T: INT1 entry non-zero\n", id);
        exit(1);
    }
    printf("[CPU%d]: UMI\n", id);
}

void CPU::m_mem_r()
{
    uint32_t addr = Read32(pc);
    pc += 4;

    uint8_t index = Read8(pc++);
    Write32(addr, regs[index]);
    printf("[CPU%d]: M 0x%08X, r%d\n", id, addr, index);
}

void CPU::int_imm()
{
    uint8_t int_num = Read8(pc++);

    uint32_t offset = sr[2] + (int_num * 5);
    uint32_t entry = Read32(offset);
    uint8_t int_flags = Read8(offset + 4);

    if (!(int_flags & 1))
    {
        printf("[CPU%d]: Called interrupt entry that isn't present (entry: %d)\n", id, int_num);
        exit(1);
    }
    Push32(pc);

    flags.IF = false;
    pc = entry;
    printf("[CPU%d]: Jumped to 0x%08X to handle INT %d\n", id, entry, int_num);
}

void CPU::iret()
{
    pc = Pop32();
    flags.IF = true;
    printf("[CPU%d]: IRET (0x%08X)\n", id, pc);
}

void CPU::m_r_mem()
{
    uint8_t index = Read8(pc++);
    uint32_t addr = Read32(pc);
    pc += 4;
    uint32_t data = Read32(addr);
    regs[index] = data;
    printf("[CPU%d]: M r%d, [0x%08X] (0x%08X)\n", id, index, addr, data);
}

void CPU::m_r_mem8()
{
    uint8_t index = Read8(pc++);
    uint32_t addr = Read32(pc);
    pc += 4;
    uint32_t data = (uint32_t)Read8(addr);
    regs[index] = data;
    printf("[CPU%d]: M r%d, [0x%08X] (0x%08X)\n", id, index, addr, data);
}

void CPU::cmp_r_imm()
{
    uint8_t index = Read8(pc++);
    uint32_t imm = Read32(pc);
    pc += 4;
    flags.ZF = (regs[index] == imm);
    printf("[CPU%d]: CMP r%d, 0x%08X\n", id, index, imm);
}

void CPU::je()
{
    uint32_t addr = Read32(pc);
    pc += 4;
    if (flags.ZF)
    {
        pc = addr;
        printf("[CPU%d]: JE 0x%08X [TAKEN]\n", id, pc);
    }
    else
        printf("[CPU%d]: JE 0x%08X [IGNORED]\n", id, addr);
}

void CPU::pop_r()
{
    uint8_t index = Read8(pc++);
    regs[index] = Pop32();
    printf("[CPU%d]: POP r%d\n", id, index);
}

void CPU::push_r()
{
    uint8_t index = Read8(pc++);
    Push32(regs[index]);
    printf("[CPU%d]: PUSH r%d\n", id, index);
}

void CPU::j()
{
    uint32_t addr = Read32(pc);
    pc += 4;
    pc = addr;
    printf("[CPU%d]: J 0x%08X\n", id, addr);
}