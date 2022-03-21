#ruledef
{
    m r{index}, {value} => 0x06 @ le(index`8) @ le(value`32)
    mi => 0x08
    mtsr => 0x0F
    j r{index} => 0x30 @ index`8
    m {addr}, r{index} => 0x11 @ le(addr`32) @ index`8
    invt => 0x32
    umi => 0x09
    int {int_num} => 0x13 @ int_num`8
    pop r{index} => 0x18 @ index`8
    push {imm} => 0x17 @ le(imm`32)
    push r{index} => 0x16 @ index`8
    iret => 0x29
    hlt => 0x31
    cmp r{index}, {imm} => 0x20 @ index`8 @ le(imm`32)
    j {addr} => 0x24 @ le(addr`32)
    m r{index}, [{addr}] => 0x12 @ index`8 @ le(addr`32)
    je {addr} => 0x25 @ le(addr`32)
    m8 r{index}, [{addr}] => 0x14 @ index`8 @ le(addr`32)
}

PAGE_ADDR = 0xC0000000 >> 22

start:
	mi
	m r0, 1
	m r1, page_tables
	mtsr
	m r0, 0
	m r1, 1
	mtsr
	m r0, higher_half+0xC0000000
	j r0
higher_half:
	m r31, stack_top+0xC0000000
	m r30, stack_bottom+0xC0000000
    m r2, 0
    m page_tables, r2
    m r0, 2
    m r1, int_table+0xC0000000
    mtsr
    m r2, task_2+0xC0000000
    m mp_struc.entry_2+0xC0000000, r2
    umi
task_1:
    m r0, 0xDEADBABA
    int 1
    j task_1+0xC0000000
task_2:
    m r0, 0xDEADCAFE
    int 1
    j task_2+0xC0000000

hlt

mp_switch:
#d32 0
	
mp_handler:
    m8 r0, [mp_switch+0xC0000000]
    cmp r0, 1
    je use_entry_2+0xC0000000
use_entry_1:
    pop r0
    m mp_struc.entry_1+0xC0000000, r0
    m r0, [mp_struc.entry_2+0xC0000000]
    push r0
    m r2, 1
    m mp_switch+0xC0000000, r2
    j end_handler+0xC0000000
use_entry_2:
    pop r0
    m mp_struc.entry_2+0xC0000000, r0
    m r0, [mp_struc.entry_1+0xC0000000]
    push r0
    m r2, 0
    m mp_switch+0xC0000000, r2
    j end_handler+0xC0000000
end_handler:
    iret

mp_struc:
.entry_1:
#d32 0
.entry_2:
#d32 0
	
page_tables:
#d8 5
#d8 0
#d8 0
#d8 0
#addr page_tables+(PAGE_ADDR*4)
#d8 5
#d8 0
#d8 0
#d8 0

int_loc = mp_handler + 0xC0000000
int_table:
#d32 0
#d8 0
#d32 int_loc[7:0] @ int_loc[15:8] @ int_loc[23:16] @ int_loc[31:24]
#d8 1

stack_bottom:
#addr stack_bottom+0x500
stack_top:
#d8 0