/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2024 NXP
 * Generated linker script file for LPC55S69
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.10.0 [Build 3148] [2024-07-03] on Oct 14, 2024, 6:18:20 PM
 */

MEMORY
{
  /* Define each memory region */
  Ram1 (rwx) : ORIGIN = 0x20030000, LENGTH = 0xc000 /* 48K bytes (alias RAM) */  
  shmem (rwx) : ORIGIN = 0x2003c000, LENGTH = 0x4000 /* 16K bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_Ram1 = 0x20030000  ; /* Ram1 */  
  __base_RAM = 0x20030000 ; /* RAM */  
  __top_Ram1 = 0x20030000 + 0xc000 ; /* 48K bytes */  
  __top_RAM = 0x20030000 + 0xc000 ; /* 48K bytes */  
  __base_shmem = 0x2003c000  ; /* shmem */  
  __base_RAM2 = 0x2003c000 ; /* RAM2 */  
  __top_shmem = 0x2003c000 + 0x4000 ; /* 16K bytes */  
  __top_RAM2 = 0x2003c000 + 0x4000 ; /* 16K bytes */  
