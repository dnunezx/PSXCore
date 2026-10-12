/* POPStarter's native launch adapter restores TLB state before entering its
 * resident loader. Repeat that native entry step before standard SDK crt0. */
    .section .igr_entry,"ax",@progbits
    .set noreorder
    .globl np_igr_loader_entry
np_igr_loader_entry:
    jal 0x235680
    nop
    j __start
    nop
