ENTRY(_entry)

MEMORY
{
  ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
  EWRAM (rw): ORIGIN = 0x02000000, LENGTH = 256K
  IWRAM (rw): ORIGIN = 0x03000000, LENGTH = 32K
}

SECTIONS
{
  .startup 0x08000000 :
  {
    KEEP(*(.startup))
  } > ROM

  .header 0x080000A0 :
  {
    KEEP(*(.header))
  } > ROM

  .text 0x080000C0 :
  {
    KEEP(*(.text.startup))
    *(.text*)
    *(.rodata*)
    *(.glue_7*)
    *(.v4_bx*)
  } > ROM

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > IWRAM

  /DISCARD/ :
  {
    *(.comment*)
    *(.note*)
    *(.ARM.exidx*)
    *(.eh_frame*)
  }
}
