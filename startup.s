.syntax unified
.arm

.section .startup, "ax", %progbits
.global _entry
.type _entry, %function
_entry:
  b _start

.section .header, "a", %progbits
.ascii "PUMPKINPARAD"
.ascii "APGE"
.ascii "01"
.byte 0x96
.byte 0x00
.byte 0x00
.space 7, 0
.byte 0x00
.byte 0x00
.space 2, 0

.section .text.startup, "ax", %progbits
.global _start
.type _start, %function
_start:
  ldr sp, =0x03007F00
  ldr r0, =__bss_start__
  ldr r1, =__bss_end__
  mov r2, #0
1:
  cmp r0, r1
  strlo r2, [r0], #4
  blo 1b
  bl main
2:
  b 2b

.ltorg
