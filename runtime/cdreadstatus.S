/* Retire seeking after POPS has verified a sector's complete sync header.
 * XA/filtered sectors do not publish data-ready replies. Preserve all other
 * status bits, native header loads, scheduling, IRQs, queues and disc data.
 * v1 is dead at this checked boundary; the next use reloads it. */
    .section ._psxcore_cd_read_status,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_read_status,np_cd_read_status_end
np_cd_read_status:
    lbu $v1,1($s0)
    andi $v1,$v1,0xbf
    sb $v1,1($s0)
    lwl $v0,15($s2)
    j 0x20e908
    lwr $v0,12($s2)
np_cd_read_status_end:
