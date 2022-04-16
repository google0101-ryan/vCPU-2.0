#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <functional>
#include <unordered_map>

class CPU
{
private:
    uint32_t regs[32];
    uint32_t sr[5];
    uint32_t pc;
    uint8_t id;
    bool halted = false;
    
    struct CacheLine
    {
        uint32_t addr;
        uint8_t data;
        uint8_t hits;
    };

    CacheLine icache[1024];
    CacheLine dcache[1024];
    uint8_t pipeline[4];

    uint8_t AdvancePipeline()
    {
        uint8_t data = pipeline[0];
        pipeline[0] = pipeline[1];
        pipeline[1] = pipeline[2];
        pipeline[2] = pipeline[3];
        pipeline[3] = ReadInstr();
        return data;
    }

    uint32_t ReadPipeline()
    {
        uint32_t data;
        data = AdvancePipeline();
        data |= AdvancePipeline() << 8;
        data |= AdvancePipeline() << 16;
        data |= AdvancePipeline() << 24;
        return data;
    }

    void FlushPipeline()
    {
        pipeline[0] = ReadInstr();
        pipeline[1] = ReadInstr();
        pipeline[2] = ReadInstr();
        pipeline[3] = ReadInstr();
    }

    uint8_t ReadInstr()
    {
        for (auto& i : icache)
            if (i.addr == pc && i.hits != 0) // Ensure we have valid cache data, since we start at pc = 0x0
            {
                pc++;
                i.hits++;
                return i.data;
            }
        uint8_t lowest_hits = 0xFF;
        CacheLine* lowest_cache;

        for (auto& i : icache)
        {
            if (i.hits < lowest_hits)
            {
                lowest_hits = i.hits;
                lowest_cache = &i;
            }
        }

        lowest_cache->hits++;
        lowest_cache->addr = pc;
        lowest_cache->data = Read8(pc++);

        return lowest_cache->data;
    }

    void UpdateCache(uint32_t addr, uint8_t data)
    {
        for (auto& i : dcache)
            if (i.addr == addr && i.hits)
            {
                i.hits++;
                i.data = data;
            }
    }

    uint8_t ram[0xFFFF];
    union
    {
        struct
        {
            bool ZF,
            CF,
            NF,
            IF,
            UM;
            uint8_t unused : 3;
        };
        uint8_t reg;
    } flags;

    void TranslateAddr(uint32_t& addr, bool w);
    uint8_t ReadRaw8(uint32_t addr)
    {
        for (auto& i : dcache)
        {
            if (i.addr == addr && i.hits)
            {
                i.hits++;
                return i.data;
            }
        }

        uint8_t lowest_hits = 0xFF;
        CacheLine* lowest_cache;

        for (auto& i : dcache)
        {
            if (i.hits < lowest_hits)
            {
                lowest_hits = i.hits;
                lowest_cache = &i;
            }
        }
        lowest_cache->addr = addr;
        lowest_cache->hits = 1;
        if (addr < 0xFFFF)
            lowest_cache->data = ram[addr];
        else
        {
            printf("[CPU%d]: Read from unknown addr 0x%08X\n", id, addr);
            exit(1);
        }
        return lowest_cache->data;
    }
    uint32_t ReadRaw32(uint32_t addr)
    {
        uint32_t ret;
        ret = ReadRaw8(addr);
        ret |= ReadRaw8(addr+1) << 8;
        ret |= ReadRaw8(addr+2) << 16;
        ret |= ReadRaw8(addr+3) << 24;
        return ret;
    }

    void WriteRaw8(uint32_t addr, uint8_t data)
    {
        UpdateCache(addr, data); // Make sure, if we're writing to cache, to update the correct entry
        if (addr < 0xFFFF)
            ram[addr] = data;
        else
        {
            printf("[CPU%d]: Write to unknown addr 0x%08X\n", id, addr);
            exit(1);
        }
    }

    void WriteRaw32(uint32_t addr, uint32_t data)
    {
        WriteRaw8(addr, (data & 0x000000FF));
        WriteRaw8(addr+1, (data & 0x0000FF00) >> 8);
        WriteRaw8(addr+2, (data & 0x00FF0000) >> 16);
        WriteRaw8(addr+3, (data & 0xFF000000) >> 24);
    }

    uint8_t Read8(uint32_t addr)
    {TranslateAddr(addr, false); return ReadRaw8(addr);}

    uint32_t Read32(uint32_t addr)
    {TranslateAddr(addr, false); return ReadRaw32(addr);}

    void Write8(uint32_t addr, uint8_t data)
    {TranslateAddr(addr, true); WriteRaw8(addr, data);}

    void Write32(uint32_t addr, uint32_t data)
    {TranslateAddr(addr, true); WriteRaw32(addr, data);}

    void Push32(uint32_t data)
    {
        uint32_t offset = regs[31] - 4;
        regs[31] = offset;
        Write32(offset, data);
    }

    void Push8(uint8_t data)
    {
        uint32_t offset = regs[31] - 1;
        regs[31]--;
        Write8(offset, data);
    }

    uint32_t Pop32()
    {
        uint32_t offset = regs[31];
        uint32_t value = Read32(offset);
        regs[31] = offset + 4;
        return value;
    }

    uint8_t Pop8()
    {
        uint32_t offset = regs[31];
        uint8_t value = Read8(offset);
        regs[31] = offset + 1;
        return value;
    }

    typedef std::function<void()> CPUFunc;

    std::unordered_map<uint8_t, CPUFunc> lookup;

    void m_r_imm(); // 0x06
    void mi(); // 0x08
    void umi(); // 0x09
    void mtsr(); // 0x0F
    void m_mem_r(); // 0x11
    void m_r_mem(); // 0x12
    void int_imm(); // 0x13
    void m_r_mem8(); // 0x12
    void push_r(); // 0x16
    void pop_r(); // 0x18
    void cmp_r_imm(); // 0x20
    void j(); // 0x24
    void je(); // 0x25
    void iret(); // 0x29
    void j_reg(); // 0x30
public:
    CPU();
    void Clock();
};
