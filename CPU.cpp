#include <CPU.hpp>
#include <prog.hpp>

static uint8_t curId = 0;

CPU::CPU()
{
    id = curId++;
    if (id != 0)
        halted = true;
    pc = 0;
    std::memset(regs, 0, sizeof(regs));
    std::memset(sr, 0, sizeof(sr));
    std::memcpy(ram, data, sizeof(data));
    flags.reg = 0;
    flags.IF = true;
    sr[0] = 0;

    FlushPipeline();

    lookup[0x06] = std::bind(&CPU::m_r_imm, this);
    lookup[0x08] = std::bind(&CPU::mi, this);
    lookup[0x09] = std::bind(&CPU::umi, this);
    lookup[0x0F] = std::bind(&CPU::mtsr, this);
    lookup[0x11] = std::bind(&CPU::m_mem_r, this);
    lookup[0x12] = std::bind(&CPU::m_r_mem, this);
    lookup[0x13] = std::bind(&CPU::int_imm, this);
    lookup[0x14] = std::bind(&CPU::m_r_mem8, this);
    lookup[0x16] = std::bind(&CPU::push_r, this);
    lookup[0x18] = std::bind(&CPU::pop_r, this);
    lookup[0x20] = std::bind(&CPU::cmp_r_imm, this);
    lookup[0x24] = std::bind(&CPU::j, this);
    lookup[0x25] = std::bind(&CPU::je, this);
    lookup[0x29] = std::bind(&CPU::iret, this);
    lookup[0x30] = std::bind(&CPU::j_reg, this);
}

void CPU::TranslateAddr(uint32_t& addr, bool w)
{
    if (sr[0] & 1)
    {
        uint32_t pd_addr = sr[1];
        uint32_t index = (addr & 0xFFC00000) >> 22;
        uint32_t offset = addr & 0x3FFFFF;
        uint32_t page_addr = ReadRaw32(pd_addr + (index * 4));
        
        if (!(page_addr & 1))
        {
            printf("[CPU%d]: Accessed non-present page 0x%X (0x%08X)!\n", id, index, addr);
            exit(1);
        }
        if (w && !(page_addr & 4))
        {
            printf("[CPU%d]: Tried to write to RO page 0x%X (0x%08X)!\n", id, index, addr);
            exit(1);
        }
        if (flags.UM && !(page_addr & 2))
        {
            printf("[CPU%d]: Accessed kernel page 0x%X from usermode (0x%08X)!\n", id, index, addr);
            exit(1);
        }
        page_addr &= 0xFFFFFFF8;
        addr = page_addr;
        addr += offset;
    }
    else
    {
        return;
    }
}

void CPU::Clock()
{
    if (halted)
        return;
    uint8_t opcode = AdvancePipeline();

    if (!lookup[opcode])
    {
        printf("[CPU%d]: Unimplemented 0x%02X\n", id, opcode);
        exit(1);
    }

    lookup[opcode]();
}