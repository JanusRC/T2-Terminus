//******************************************************************************
//
//  TERMINUS2 Boot Loader - Keil - Built with STM32F405ZG
//
//  Clive Turvey - Connor Winfield Corp (NavSync and Janus RC Groups)
//     cturvey@conwin.com  cturvey@navsync.com  cturvey@gmail.com
//
//******************************************************************************
//
// Modified stm32f4xx.h
//
// #ifndef HSE_VALUE
//  #define HSE_VALUE            ((uint32_t)16000000) /*!< Value of the External oscillator in Hz */
// #endif
//
// Modified system_stm32f4xx.c
//
//  *        HSE Frequency(Hz)                      | 16000000
//  *        PLL_M                                  | 16
//  #define PLL_M      16
//
// Ported to STM32F4xx_DSP_StdPeriph_Lib_V1.1.0
//  startup_stm32f40x.s
//  system_stm32F4xx.c
//   fixed for 16 MHz HSE
//   added failover to HSI
//  Compiler command line -DHSE_VALUE=16000000
//
// Ported to STM32F4xx_DSP_StdPeriph_Lib_V1.2.0
//  Compiler command line -DSTM32F40_41xxx deprecated -DSTM32F4XX
//******************************************************************************
//
// Boot Loader resides at 0x08000000..0x08003FFF (16 KB)
// Application resides at 0x08020000..0x0803FFFF (128 KB)
//
// Supports
//   Polled operation on USART3
//   Flashing Application
//
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "stm32f4xx.h"

//******************************************************************************

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define FALSE 0
#define TRUE (!FALSE)

//******************************************************************************

volatile int SystemTick             = 0; // 1 ms ticker

volatile int CurrentLED;

extern void SystemReset(void);

void Reboot_Application(void);

void NVIC_Configuration(void);
void GPIO_Configuration(void);

void EnableTiming(void);
void TimingDelay(unsigned int tick);

void ClockDiagnostic(void);

int ApplicationFirmwareCheck(void);

WORD CRC16(WORD Crc, DWORD Size, BYTE *Buffer);
DWORD CRC32(DWORD Crc, DWORD Size, BYTE *Buffer);

FLASH_Status EraseFlash(DWORD Addr);
FLASH_Status WriteFlash(DWORD Addr, DWORD Size, BYTE *Buffer);

void BootLoaderApplication(void);
void BootLoaderConfiguration(void);

unsigned int FiveMicro;

#define APPLICATION_BASE 0x08020000

#define MODEM_BAUD 115200

//#define REV1 // PB2409R0
#define REV2 // PB2409R1

#define FLOW_NONE   0
#define FLOW_CTS_HW 1
#define FLOW_RTS_HW 2
#define FLOW_CTS_SW 4
#define FLOW_RTS_SW 8

#define FLOW3 (FLOW_CTS_HW | FLOW_RTS_SW) // RS232
#define FLOW6 (FLOW_CTS_HW | FLOW_RTS_SW) // MODEM

#define CLK_DIAG

#define POWER_ON_FAST 1250 // CC864
#define POWER_ON_SLOW 5000 // HE910 5 Seconds

//****************************************************************************

typedef struct _BOOTLOADER
{
  // Hardware Parameters

  char MEID[16];

  // Device Parameters

  char  Identifier[24];

  // Functional

  int BaudRate;
  int PowerPulse;
  int RS232Flow;

  char InitialAT[32];

  // Management

  int Updated; // Fields updated, commit to EEPROM periodic

} BOOTLOADER;

BOOTLOADER BootLoader[1];

//****************************************************************************

BOOTLOADER *BootLoader_Open(BOOTLOADER *btldr);
void BootLoader_Close(BOOTLOADER *btldr);
void BootLoader_WriteConfig(BOOTLOADER *btldr);
int BootLoader_ReadConfig(BOOTLOADER *btldr);

//****************************************************************************

BOOTLOADER *BootLoader_Open(BOOTLOADER *btldr)
{
  // Initialize BootLoader State

  memset(btldr, 0, sizeof(BOOTLOADER));

  // Load EEPROM Settings into BootLoader Context

  if (BootLoader_ReadConfig(btldr) == 0) // If none, then write in defaults
    BootLoader_WriteConfig(btldr);

  return(btldr);
}

//****************************************************************************

void BootLoader_Close(BOOTLOADER *btldr)
{
}

//****************************************************************************

typedef struct _EEPROM_BOOTLOADER_VER1 {
  DWORD Signature;                // Signature Word, non 0xFFFFFFFF
  DWORD Version;                  // Handle version changes from the outset

  // Hardware Parameters

  char MEID[16];

  // Device Parameters

  char  Identifier[24];

  // Functional

  int BaudRate;
  int PowerPulse;
  int RS232Flow;

  char InitialAT[32];

  DWORD Crc32;                    // CRC Checksum - Should be on a 4-byte boundary
} EEPROM_BOOTLOADER_VER1;

#define BTLDREESIG 0x45455442 // BTEE

//****************************************************************************

#define FLASH_CFG 0x08008000 // 16KB

//#define DBG_WRITECFG // Don't enable here, it makes assumptions about debug output functions

void BootLoader_WriteConfig(BOOTLOADER *btldr)
{
  EEPROM_BOOTLOADER_VER1 btldree[1];

#ifdef DBG_WRITECFG
  puts("BootLoader_WriteConfig");
#endif

  // Write only current version

  memset(btldree, 0x00, sizeof(EEPROM_BOOTLOADER_VER1));

  // Signature

  btldree->Signature = BTLDREESIG;
  btldree->Version = 1;

  // Hardware

  strcpy(btldree->MEID,               btldr->MEID);   // Modem Serial

  // Device

  strcpy(btldree->Identifier,         btldr->Identifier);

  // Functional

  btldree->BaudRate   = btldr->BaudRate;
  btldree->PowerPulse = btldr->PowerPulse;
  btldree->RS232Flow  = btldr->RS232Flow;

  strcpy(btldree->InitialAT,          btldr->InitialAT);

  // Signing

  btldree->Crc32 = CRC32(0xFFFFFFFF, sizeof(EEPROM_BOOTLOADER_VER1) - sizeof(DWORD), (void *)btldree);

  if (memcmp((void *)FLASH_CFG, (void *)btldree, sizeof(EEPROM_BOOTLOADER_VER1)) != 0) // Actually Different?
  {
    EraseFlash(FLASH_CFG); /* Erase the configuration area */

#ifdef DBG_WRITECFG
    DumpData(sizeof(EEPROM_BOOTLOADER_VER1), (void *)FLASH_CFG);
    putchar('\n');
#endif

    WriteFlash(FLASH_CFG, sizeof(EEPROM_BOOTLOADER_VER1), (void *)btldree);

#ifdef DBG_WRITECFG
    DumpData(sizeof(EEPROM_BOOTLOADER_VER1), (void *)btldree);
    putchar('\n');
    DumpData(sizeof(EEPROM_BOOTLOADER_VER1), (void *)FLASH_CFG);
    putchar('\n');
#endif
  }

  btldr->Updated = 0; // Matches EEPROM
}

//****************************************************************************

//#define DBG_READCFG // Don't enable here, it makes assumptions about debug output functions

int BootLoader_ReadConfig(BOOTLOADER *btldr)
{
  EEPROM_BOOTLOADER_VER1 *btldree = (EEPROM_BOOTLOADER_VER1 *)FLASH_CFG;

#ifdef DBG_READCFG
  puts("BootLoader_ReadConfig");

  DumpData(sizeof(EEPROM_BOOTLOADER_VER1), (void *)btldree);
  putchar('\n');
#endif

  // Read ALL version(s)

  if ((btldree->Signature == BTLDREESIG) && (btldree->Version == 1)) // Identify
  {
    if (btldree->Crc32 == CRC32(0xFFFFFFFF, sizeof(EEPROM_BOOTLOADER_VER1) - sizeof(DWORD), (void *)btldree) ) // Validate
    {
      // Hardware

      strcpy(btldr->MEID,                 btldree->MEID);   // Modem Serial

      // Device

      strcpy(btldr->Identifier,           btldree->Identifier);

      // Functional

      btldr->BaudRate   = btldree->BaudRate;
      btldr->PowerPulse = btldree->PowerPulse;
      btldr->RS232Flow  = btldree->RS232Flow;

      strcpy(btldr->InitialAT,            btldree->InitialAT);


      // Management

      btldr->Updated = 0; // Valid from EEPROM

      return(1);
    }
#ifdef DBG_READCFG
    else
      puts("Corrupt");
#endif
  }
#ifdef DBG_READCFG
  else
    puts("Invalid");
#endif

  // Load Device Defaults

  strcpy(btldr->Identifier, "123456");

  btldr->BaudRate = 115200;
  btldr->PowerPulse = POWER_ON_FAST;
  btldr->RS232Flow = 0;  // Off by default FLOW3;

  strcpy(btldr->InitialAT, "");

  btldr->Updated = 1; // All invalid in EEPROM

  return(0);
}

//******************************************************************************
#ifdef CLK_DIAG
//******************************************************************************

#define CAPCOUNT 3

//******************************************************************************
// Configures TIM5 to measure the LSI oscillator frequency.
//  returns LSI Frequency

uint32_t GetLSIFrequency(void)
{
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  uint32_t CaptureNumber = 0, PeriodValue = 0;
  uint16_t tmpCC4[CAPCOUNT];

  /* TIM5 configuration *******************************************************/
  /* Enable TIM5 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

  /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
  TIM_RemapConfig(TIM5, TIM5_LSI);

  /* Configure TIM5 presclaer */
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);

  /* Enable TIM5 counter */
  TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
  TIM5->SR = 0;

  CaptureNumber = 0;

  /* Wait until the TIM5 get 2 LSI edges, without physical interrupts */
  while(CaptureNumber != CAPCOUNT)
  {
    if (TIM_GetFlagStatus(TIM5, TIM_FLAG_CC4) != RESET)
    {
      /* Get the Input Capture value */
      if (CaptureNumber < CAPCOUNT)
        tmpCC4[CaptureNumber++] = TIM_GetCapture4(TIM5);

      /* Clear CC4 pending bit */
      TIM_ClearFlag(TIM5, TIM_FLAG_CC4);

      if (CaptureNumber >= CAPCOUNT)
      {
        /* Compute the period length */
        PeriodValue = (uint32_t)(tmpCC4[CAPCOUNT - 1] - tmpCC4[CAPCOUNT - 2]);
      }
    }
  }

  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_DeInit(TIM5);

  /* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  {
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return((RCC_ClockFreq.PCLK1_Frequency * 8) / PeriodValue);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return((2 * RCC_ClockFreq.PCLK1_Frequency * 8) / PeriodValue);
  }
}

//******************************************************************************
// Configures TIM5 to measure the LSE oscillator frequency.
//  returns LSE Frequency

uint32_t GetLSEFrequency(void)
{
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  uint32_t CaptureNumber = 0, PeriodValue = 0;
  uint16_t tmpCC4[CAPCOUNT];

  /* TIM5 configuration *******************************************************/
  /* Enable TIM5 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

  /* Connect internally the TIM5_CH4 Input Capture to the LSE clock output */
  TIM_RemapConfig(TIM5, TIM5_LSE);

  /* Configure TIM5 presclaer */
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);

  /* Enable TIM5 counter */
  TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
  TIM5->SR = 0;

  CaptureNumber = 0;

  /* Wait until the TIM5 get 2 LSE edges, without physical interrupts */
  while(CaptureNumber != CAPCOUNT)
  {
    if (TIM_GetFlagStatus(TIM5, TIM_FLAG_CC4) != RESET)
    {
      /* Get the Input Capture value */
      if (CaptureNumber < CAPCOUNT)
        tmpCC4[CaptureNumber++] = TIM_GetCapture4(TIM5);

      /* Clear CC4 pending bit */
      TIM_ClearFlag(TIM5, TIM_FLAG_CC4);

      if (CaptureNumber >= CAPCOUNT)
      {
        /* Compute the period length */
        PeriodValue = (uint32_t)(tmpCC4[CAPCOUNT - 1] - tmpCC4[CAPCOUNT - 2]);
      }
    }
  }

  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_DeInit(TIM5);

  /* Compute the LSE frequency, depending on TIM5 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  {
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return((RCC_ClockFreq.PCLK1_Frequency * 8) / PeriodValue);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return((2 * RCC_ClockFreq.PCLK1_Frequency * 8) / PeriodValue);
  }
}

//******************************************************************************

void ClockMCO(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOs clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure MCO (PA8) */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

#if 1
  /* Output LSE clock (32.768KHz) on MCO pin (PA8) */
  RCC_MCO1Config(RCC_MCO1Source_LSE, RCC_MCO1Div_1);
#endif
	
#if 0	
	/* Output PLL/2 clock (84MHz) on MCO pin (PA8) */
  RCC_MCO1Config(RCC_MCO1Source_PLLCLK, RCC_MCO1Div_2);
#endif
}

//******************************************************************************

//#define CPUSPEED 168000000
#define CPUSPEED (SystemCoreClock) // As developed by PLL settings
#define LSE_USABLE
#if 0
#define LSE_MIN 32767 // Should be on the money
#define LSE_MAX 32769
#endif
#if 1 // 1% +/-327.68
#define LSE_MIN (32768-328)
#define LSE_MAX (32768+328)
#endif
#if 0 // 5% +/-1638.4
#define LSE_MIN (32768-1638)
#define LSE_MAX (32768+1638)
#endif

void ClockDiagnostic(void)
{
  RCC_ClocksTypeDef RCC_ClockFreq;
  volatile int i;
#ifdef LSE_USABLE
  uint32_t lsifreq = 0, lsefreq = 0;
#else
  uint32_t lsifreq = 0;
#endif
  int OutputDiagnostic = 0;

  ClockMCO(); /* Output LSE to test point */

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); /* Enable PWR clock */

  PWR_BackupAccessCmd(ENABLE); /* Allow access to BKP Domain */

  RCC_LSICmd(ENABLE); /* Enable LSI */

  RCC_LSEConfig(RCC_LSE_ON); /* Enable LSE */

  RCC_GetClocksFreq(&RCC_ClockFreq);

  if (RCC_ClockFreq.SYSCLK_Frequency != CPUSPEED) // Valid PLL rate
    OutputDiagnostic++;

  if (RCC_GetSYSCLKSource() != 8) // PLL Source, probably duplicative
    OutputDiagnostic++;

  // LSI startup time 15us typical,  40us max
  // LSE startup time  2s not untypical - Crystal not working with F405 silicon

  i = 0;

#ifdef LSE_USABLE
  while((i++ < 20000) && (!(RCC->BDCR & RCC_BDCR_LSERDY) || !(RCC->CSR & RCC_CSR_LSIRDY))) /* Wait till LSI/LSE is ready */
    TimingDelay(RCC_ClockFreq.SYSCLK_Frequency / 10000);
#else
  while((i++ < 20000) && !(RCC->CSR & RCC_CSR_LSIRDY)) /* Wait till LSI is ready */
    TimingDelay(RCC_ClockFreq.SYSCLK_Frequency / 10000);
#endif

  if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != RESET)
    lsifreq = GetLSIFrequency();
  else
    OutputDiagnostic++;

  if ((lsifreq < 17000) || (lsifreq > 47000)) //  17 KHz min, 32 KHz typ, 47 KHz max, ie pretty hideous
    OutputDiagnostic++;

#ifdef LSE_USABLE
  if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET)
    lsefreq = GetLSEFrequency();
  else
    OutputDiagnostic++;

  if ((lsefreq < LSE_MIN) || (lsefreq > LSE_MAX)) 
    OutputDiagnostic++;
#endif

//  if (OutputDiagnostic)
  {
    puts("ClockDiagnostic");

    printf("SYS:%d H:%d, P1:%d, P2:%d\r\n",
              RCC_ClockFreq.SYSCLK_Frequency,
              RCC_ClockFreq.HCLK_Frequency,   // AHB
              RCC_ClockFreq.PCLK1_Frequency,  // APB1
              RCC_ClockFreq.PCLK2_Frequency); // APB2

#ifdef LSE_USABLE
    printf("LSI:%d, LSE:%d\n", lsifreq, lsefreq);
#else
    printf("LSI:%d\n", lsifreq);
#endif

    switch(RCC_GetSYSCLKSource())
    {
      case 0 : puts("HSI Source"); break;
      case 4 : puts("HSE Source"); break;
      case 8 : puts("PLL Source"); break;
    }

    if (RCC_ClockFreq.SYSCLK_Frequency != CPUSPEED) // Valid PLL rate
      puts("WARNING: Not running at expected speed");

    if (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET)
      puts("WARNING: HSI Failed to start");

    if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
      puts("WARNING: HSE Failed to start");

    if (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
      puts("WARNING: PLL Failed to lock");

//  if (RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) == RESET) // Not set up
//    puts("PLL I2S Not Ready");

    if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
      puts("WARNING: LSI Failed to start");

    if ((lsifreq < 17000) || (lsifreq > 47000)) //  17 KHz min, 32 KHz typ, 47 KHz max, ie pretty hideous
      puts("WARNING: PLL, HSE or LSI out of spec");

#ifdef LSE_USABLE
    if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
      puts("WARNING: LSE Failed to start");

    if ((lsefreq < LSE_MIN) || (lsefreq > LSE_MAX)) // Should be on the money, suprised if not 32768
      puts("WARNING: PLL, HSE or LSE out of spec");
#endif

    putchar('\n');
  }

}

//******************************************************************************
#endif // CLK_DIAG
//******************************************************************************

#define DBG_FLASH

FLASH_Status EraseFlash(DWORD Addr)
{
  FLASH_Status status = FLASH_COMPLETE;

  FLASH_Unlock();

  /* Clear pending flags (if any), observed post Keil/J-Link flashing */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR| FLASH_FLAG_PGSERR);

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation();

// VoltageRange_4 means BOOT0/VPP = 7-9V, upto 1 Hour

// 128K takes approx 1 second to erase

  if (status == FLASH_COMPLETE)
  {
    if (Addr == 0x08020000)
      status = FLASH_EraseSector(FLASH_Sector_5, VoltageRange_3);   // 128 KB
    else if (Addr == 0x08040000)
      status = FLASH_EraseSector(FLASH_Sector_6, VoltageRange_3);
    else if (Addr == 0x08060000)
      status = FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
    else if (Addr == 0x08080000)
      status = FLASH_EraseSector(FLASH_Sector_8, VoltageRange_3);
    else if (Addr == 0x080A0000)
      status = FLASH_EraseSector(FLASH_Sector_9, VoltageRange_3);
    else if (Addr == 0x080C0000)
      status = FLASH_EraseSector(FLASH_Sector_10, VoltageRange_3);
    else if (Addr == 0x080E0000)
      status = FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);  // 128 KB
#if 0 // Protect against self destruction
    else if (Addr == 0x08000000)
      status = FLASH_EraseSector(FLASH_Sector_0, VoltageRange_3);   // 16 KB
#endif
#if 1
    else if (Addr == 0x08004000)
      status = FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);   // 16 KB
    else if (Addr == 0x08008000)
      status = FLASH_EraseSector(FLASH_Sector_2, VoltageRange_3);   // 16 KB
    else if (Addr == 0x0800C000)
      status = FLASH_EraseSector(FLASH_Sector_3, VoltageRange_3);   // 16 KB
    else if (Addr == 0x08010000)
      status = FLASH_EraseSector(FLASH_Sector_4, VoltageRange_3);   // 64 KB
#endif
#if 0 // 2MB Chip
    else if (Addr == 0x08100000)
      status = FLASH_EraseSector(FLASH_Sector_12, VoltageRange_3);  // 16 KB
    else if (Addr == 0x08104000)
      status = FLASH_EraseSector(FLASH_Sector_13, VoltageRange_3);  // 16 KB
    else if (Addr == 0x08108000)
      status = FLASH_EraseSector(FLASH_Sector_14, VoltageRange_3);  // 16 KB
    else if (Addr == 0x0810C000)
      status = FLASH_EraseSector(FLASH_Sector_15, VoltageRange_3);  // 16 KB
    else if (Addr == 0x08110000)
      status = FLASH_EraseSector(FLASH_Sector_16, VoltageRange_3);  // 64 KB
    else if (Addr == 0x08120000)
      status = FLASH_EraseSector(FLASH_Sector_17, VoltageRange_3);  // 128 KB
    else if (Addr == 0x08140000)
      status = FLASH_EraseSector(FLASH_Sector_18, VoltageRange_3);
    else if (Addr == 0x08160000)
      status = FLASH_EraseSector(FLASH_Sector_19, VoltageRange_3);
    else if (Addr == 0x08180000)
      status = FLASH_EraseSector(FLASH_Sector_20, VoltageRange_3);
    else if (Addr == 0x081A0000)
      status = FLASH_EraseSector(FLASH_Sector_21, VoltageRange_3);
    else if (Addr == 0x081C0000)
      status = FLASH_EraseSector(FLASH_Sector_22, VoltageRange_3);
    else if (Addr == 0x081E0000)
      status = FLASH_EraseSector(FLASH_Sector_23, VoltageRange_3);  // 128 KB
#endif
  }

  FLASH_Lock();

  return(status);
}

//******************************************************************************

FLASH_Status WriteFlash(DWORD Addr, DWORD Size, BYTE *Buffer)
{
  FLASH_Status status = FLASH_ERROR_PROGRAM;
  DWORD i;

  if (*(__IO uint32_t*)Addr != 0xFFFFFFFF)
  {
#ifdef DBG_FLASH
    printf("Flash not blank at %08X\n", Addr);
#endif
    return(status);
  }

  FLASH_Unlock();

  /* Clear pending flags (if any), observed post Keil/J-Link flashing */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR| FLASH_FLAG_PGSERR);

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation();

  if (status == FLASH_COMPLETE)
  {
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_WORD;
    FLASH->CR |= FLASH_CR_PG;

    for(i=Addr; i<(Addr + Size); i += 4)
    {
      uint32_t Data;

      Data = *((uint32_t *)&Buffer[i - Addr]);

      if (Data != 0xFFFFFFFF) // Non-Blank
      {
        *(__IO uint32_t*)i = Data;

        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOperation();

        if (status != FLASH_COMPLETE)
        {
#ifdef DBG_FLASH
          printf("FLASH Write Fail at %08X\n", i);
#endif
          break;
        }
      }
    }

    FLASH->CR &= (~FLASH_CR_PG);
  }

  FLASH_Lock();

  return(status);
}

//****************************************************************************

extern void * __Vectors;

u8 SysTickPriority;

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  u8 Priority;

  NVIC_SetVectorTable((u32)(&__Vectors), 0x0); // Smart Base Location

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;

  Priority = 0;

  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the USART6 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  SysTickPriority = Priority++;
}

//******************************************************************************

// From http://forums.arm.com/index.php?showtopic=13949

volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register
volatile unsigned int *SCB_DEMCR    = (volatile unsigned int *)0xE000EDFC; //address of the register

//******************************************************************************

void EnableTiming(void)
{
  static int enabled = 0;

  if (!enabled)
  {
    RCC_ClocksTypeDef RCC_Clocks;

    RCC_GetClocksFreq(&RCC_Clocks);

    FiveMicro = RCC_Clocks.HCLK_Frequency / (1000000 / 5); // Clock ticks for ~5us

    *SCB_DEMCR |= 0x01000000;
    *DWT_CYCCNT = 0; // reset the counter
    *DWT_CONTROL |= 1 ; // enable the counter

    enabled = 1;
  }
}

//******************************************************************************

void TimingDelay(unsigned int tick)
{
  unsigned int start, current;

  start = *DWT_CYCCNT;

  do
  {
    current = *DWT_CYCCNT;
  } while((current - start) < tick);
}

//******************************************************************************

// Generic CRC-CCITT 16 bit implementation for buffer/stream validation
//  This is a minimal code space version, a fast parallel table routine
//  could be used if time/throughput is critical

WORD CRC16(WORD Crc, DWORD Size, BYTE *Buffer)
{
  while(Size--)
  {
#if 0
    int i;

    Crc = Crc ^ ((WORD)*Buffer++ << 8);

    for(i=0; i<8; i++)
      if (Crc & 0x8000)
        Crc = (Crc << 1) ^ 0x1021;
      else
        Crc = (Crc << 1);
#else
    static const WORD CrcTable[] = {
      0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
      0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF };

    Crc = Crc ^ ((WORD)*Buffer++ << 8);

    Crc = (Crc << 4) ^ CrcTable[Crc >> 12];

    Crc = (Crc << 4) ^ CrcTable[Crc >> 12];
#endif
  }

  return(Crc);
}

//******************************************************************************

// Generic CRC 32 bit implementation for buffer/stream validation

DWORD CRC32(DWORD Crc, DWORD Size, BYTE *Buffer)
{
  while(Size--)
  {
#if 0
  int i;

    Crc = Crc ^ (DWORD)*Buffer++;

    for(i=0; i<8; i++)
      if (Crc & 1)
        Crc = (Crc >> 1) ^ 0xEDB88320;
      else
        Crc = (Crc >> 1);
#else
    static const DWORD CrcTable[] = {
      0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
      0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C };

    Crc = Crc ^ (DWORD)*Buffer++;

    Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F];

    Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F];
#endif
  }

  return(Crc);
}

//******************************************************************************

int ApplicationFirmwareCheck(void)
{
  if (*((unsigned long *)APPLICATION_BASE) == 0xFFFFFFFF) // Application erased
  {
#ifndef SILENT
    puts("Application Missing"); // Did say Application Erased, too alarming
#endif

    return(0);
  }

  if (*((unsigned long *)(APPLICATION_BASE + 0x188)) == 0xDEADBEEF) // Unchecked Debug Application
  {
#ifndef SILENT
    puts("Unchecked Debug Application");
#endif

    return(1);
  }

  if (CRC32(0xFFFFFFFF, 0x20000, (void *)APPLICATION_BASE) != 0)
  {
#ifndef SILENT
    puts("Application Corrupted");
#endif

    return(0);
  }

  return(1);
}

//******************************************************************************

// Check for Three ESC characters in 1 Second to break to boot loader

int BreakKeyCheck(void)
{
  unsigned int start, current;
  int esc = 0;

  start = *DWT_CYCCNT;

  do
  {
    if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
    {
      if (USART_ReceiveData(USART3) == 0x1B) // ESCAPE Hit
      {
        esc++;

        if (esc >= 3) // Three ESC observed
          break;
      }
    }

    current = *DWT_CYCCNT;
  } while((current - start) < SystemCoreClock); // 1 Second

  return(esc >= 3 ? 1 : 0);
}

//******************************************************************************

// Win32 style implementation of time delay, or yield type functionality
//  Timer increments on timer tick interrupt, so processor halted with
//  Thumb2 WFI instruction so this won't grind in a loop unnecessarily

int Sleep(int Delay)
{
  int Start, Current;

  Start = SystemTick;

  do
  {
    __WFI();                  // Wait for interrupt

    Current = SystemTick;
  }
  while((Current - Start) < Delay);

  return(Current - Start); // Estimate of time past
}

//******************************************************************************
//
// LED Abstraction
//
//******************************************************************************

void WriteLED(int i)
{
  if (i & 1) // RED
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_12);   // PE.12 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_12); // PE.12 Low
  }

  if (i & 2) // YELLOW
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_13);   // PE.13 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_13); // PE.13 Low
  }

  if (i & 4) // GREEN
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_14);   // PE.14 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_14); // PE.14 Low
  }
}

//****************************************************************************
//
// FIFO Library
//
//****************************************************************************

#define FIFO_SIZE 1024
//#define FIFO_SIZE 2048

//****************************************************************************

// Simple FIFO buffering example, designed to hold over a second of data
//  at 38400 baud (ie around 3840 bps)

//****************************************************************************

typedef struct _FIFO {
  DWORD FifoHead;
  DWORD FifoTail;
  DWORD FifoSize;
  BYTE FifoBuffer[FIFO_SIZE];
} FIFO;

FIFO *Fifo;

//****************************************************************************

FIFO *Fifo_Open(FIFO *Fifo)
{
  memset(Fifo, 0, sizeof(FIFO));

  Fifo->FifoHead = 0;
  Fifo->FifoTail = 0;
  Fifo->FifoSize = FIFO_SIZE;

  return(Fifo);
}

//****************************************************************************

void Fifo_Close(FIFO *Fifo)
{
  // free(Fifo); // Dynamic implementation
}

//****************************************************************************

DWORD Fifo_Used(FIFO *Fifo)
{
  DWORD Used;

   if (Fifo->FifoHead >= Fifo->FifoTail)
     Used = (Fifo->FifoHead - Fifo->FifoTail);
   else
     Used = ((Fifo->FifoHead + Fifo->FifoSize) - Fifo->FifoTail);

  return(Used);
}

//****************************************************************************

DWORD Fifo_Free(FIFO *Fifo)
{
  return((Fifo->FifoSize - 1) - Fifo_Used(Fifo));
}

//****************************************************************************

void Fifo_Flush(FIFO *Fifo)
{
  Fifo->FifoHead = Fifo->FifoTail;
}

//****************************************************************************

// Insert stream of data into FIFO, assumes stream is smaller than FIFO
//  will fail if insufficient space which will only occur if the worker
//  process is not extracting and processing the data in a timely fashion

BOOL Fifo_Insert(FIFO *Fifo, DWORD Size, BYTE *Buffer)
{
   if (Size > Fifo_Free(Fifo))
      return(FALSE);

   while(Size)
   {
      DWORD Length;

      Length = min(Size, (Fifo->FifoSize - Fifo->FifoHead));

      memcpy(&Fifo->FifoBuffer[Fifo->FifoHead], Buffer, Length);

      Fifo->FifoHead += Length;

      if (Fifo->FifoHead == Fifo->FifoSize)
         Fifo->FifoHead = 0;

      Size -= Length;
      Buffer += Length;
   }

   return(TRUE);
}

//****************************************************************************

int Fifo_Extract(FIFO *Fifo, DWORD Size, BYTE *Buffer)
{
   if (Size > Fifo_Used(Fifo))
      return(FALSE);

   while(Size)
   {
      DWORD Length;

      Length = min(Size, (Fifo->FifoSize - Fifo->FifoTail));

      memcpy(Buffer, &Fifo->FifoBuffer[Fifo->FifoTail], Length);

      Fifo->FifoTail += Length;

      if (Fifo->FifoTail == Fifo->FifoSize)
         Fifo->FifoTail = 0;

      Size -= Length;
      Buffer += Length;
   }

  return(TRUE);
}

//******************************************************************************

void Fifo_Forward(FIFO *FifoA, FIFO *FifoB)
{
  DWORD Used;
  DWORD Free;
  DWORD Size;
  BYTE Buffer[32]; // Could make this bigger

  // Half Duplex, currently not totally interrupt safe, could a) check
  //  the Tx buffer space more often, or b) always limit the apparent free space

  Used = Fifo_Used(FifoA);
  Free = Fifo_Free(FifoB);

  Size = min(Used, Free);

  while(Size) // Either run out of source, or the destination fills
  {
    Size = min(Size, sizeof(Buffer));

    Fifo_Extract(FifoA, Size, Buffer);

    Fifo_Insert(FifoB, Size, Buffer);

    Used -= Size;
    Free -= Size;

    Size = min(Used, Free);
  }
}

//****************************************************************************

char Fifo_PeekChar(FIFO *Fifo)
{
  if (!Fifo_Used(Fifo)) // No Data available
    return(0);

  return(Fifo->FifoBuffer[Fifo->FifoTail]);
}

//****************************************************************************

char Fifo_GetChar(FIFO *Fifo)
{
  char c;

  if (!Fifo_Used(Fifo)) // No data available
    return(0);

  c = Fifo->FifoBuffer[Fifo->FifoTail++];

  if (Fifo->FifoTail == Fifo->FifoSize)
    Fifo->FifoTail = 0;

  return(c);
}

//****************************************************************************

char Fifo_PutChar(FIFO *Fifo, char c)
{
  if (!Fifo_Free(Fifo)) // No space available
    return(0);

  Fifo->FifoBuffer[Fifo->FifoHead++] = c;

  if (Fifo->FifoHead == Fifo->FifoSize)
    Fifo->FifoHead = 0;

  return(c);
}

//******************************************************************************
//
// STM32 Uart Library
//
//******************************************************************************

typedef struct _GPIO {
  GPIO_TypeDef* Bank;
  uint32_t Pin;
  uint32_t PinSource;
} GPIO;

//******************************************************************************

typedef struct _UART {

  USART_TypeDef *Port;  // STM32 Hardware for interface

  // Management and diagnostics

  int Flow;
  int Transmit;
  int Throttle;
  int Errors;
  int Overflow;
  int RxCount;
  int Busy;
  int TxCount;

  // FIFO Ring buffers

  FIFO Tx[1];
  FIFO Rx[1];

  // Modem state per interface

  int Uart_ModemOnTick;

  int ModemOn;
  int ModemConnected;
  int NetworkRegistrationState; // Not, Failed, Local, Roam
  int NetworkActivationState;

  // Interface pins, watch that Uart_Open() clears this region

  GPIO TX_GPIO;
  GPIO RX_GPIO;
  GPIO CTS_GPIO;
  GPIO RTS_GPIO;
  GPIO DSR_GPIO;
  GPIO DTR_GPIO;
  GPIO DCD_GPIO;
  GPIO RING_GPIO;

  GPIO PWRMON_GPIO;
  GPIO ONOFF_GPIO;
  GPIO RESET_GPIO;
  GPIO GSMLED_GPIO;

} UART;

//******************************************************************************

static UART Uart3[1]; // RS232 Debug
static UART Uart6[1]; // Modem

static UART *UartDebug = Uart3;
static UART *UartModem = Uart6;

//******************************************************************************

UART *Uart_Open(UART *Uart, USART_TypeDef *Port);
void Uart_Close(UART *Uart);
void Uart_EmptyRx(UART *Uart);
void Uart_FillTx(UART *Uart, int Periodic);
void Uart_Enable(UART *Uart);
void Uart_Disable(UART *Uart);
void Uart_Throttle(UART *Uart);
void Uart_ForwardHalfDuplex(UART *UartA, UART *UartB);
void Uart_ForwardFullDuplex(UART *UartA, UART *UartB);
void Uart_SendBuffer(UART *Uart, DWORD Size, BYTE *Buffer);
void Uart_SendMessage(UART *Uart, char *Buffer);
void Uart_Flush(UART *Uart);

u8 Uart_ModemReadCTS(UART *Uart);
u8 Uart_ModemReadRTS(UART *Uart);

void Uart_ModemWriteRTS(UART *Uart, int i);

void USART6_Suppress(UART *Uart, int Enable);

//******************************************************************************

UART *Uart_Open(UART *Uart, USART_TypeDef *Port)
{
  memset(Uart, 0, sizeof(UART));

  Uart->Port = Port;

  Fifo_Open(Uart->Tx);
  Fifo_Open(Uart->Rx);

  return(Uart);
}

//******************************************************************************

void Uart_Close(UART *Uart)
{
  if (Uart)
  {
    Fifo_Close(Uart->Tx);
    Fifo_Close(Uart->Rx);

  // free(Uart); // dynamic implementation
  }
}

//******************************************************************************

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)

void Uart_EmptyRx(UART *Uart)
{
  USART_TypeDef *Port = Uart->Port;
  WORD status;

  status = Port->SR;

  while(status & (USART_FLAG_RXNE | USART_FLAG_ERRORS))
  {
    BYTE ch;

    ch = Port->DR;  // Read Data, or Clear Error(s)

//    if (USART_GetITStatus(Uart->Port, USART_IT_RXNE) != RESET)
//      USART_ClearITPendingBit(Uart->Port, USART_IT_RXNE); /* Clear the USART Receive interrupt */

    if (status & USART_FLAG_ERRORS)
      Uart->Errors++;
    else
    {
      Uart->RxCount++;

      if (Fifo_Insert(Uart->Rx, sizeof(ch), &ch) != sizeof(ch))
        Uart->Overflow++;
    }

    status = Port->SR;
  }

  if (USART_GetITStatus(Uart->Port, USART_IT_CTS) != RESET)
    USART_ClearITPendingBit(Uart->Port, USART_IT_CTS);  /* Clear the USART CTS interrupt */
}

//******************************************************************************

// Called by interrupt, and on periodic

void Uart_FillTx(UART *Uart, int Periodic)
{
  USART_TypeDef *Port = Uart->Port;
  WORD status;
  BYTE ch;

  status = Port->SR;

  if (status & USART_FLAG_ERRORS)
  {
    ch = Port->DR; // Clear the error

    Uart->Errors++;
  }

  if (Periodic && Uart->Transmit) // Don't try to prempt the interrupt
    return;

  if (status & USART_FLAG_TXE)
  {
    DWORD Used;

    Used = Fifo_Used(Uart->Tx);

    if (Used != 0)
    {
      Fifo_Extract(Uart->Tx, sizeof(ch), &ch);

//      Used--;

      Port->DR = ch;

      Uart->TxCount++;
    }

    if (Used == 0)
    {
      if ((Uart->Transmit == 1) || Periodic)
      {
        USART_ITConfig(Uart->Port, USART_IT_TXE, DISABLE); // Nothing to send, throttle

        Uart->Transmit = 0;
      }
    }
    else
    {
      if ((Uart->Transmit == 0) || Periodic)
      {
        USART_ITConfig(Uart->Port, USART_IT_TXE, ENABLE);

        Uart->Transmit = 1;
      }
    }
  }
}

//******************************************************************************

void Uart_Enable(UART *Uart)
{
  if (Uart->Port == USART1)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); /* Enable USART1 Clock */
  }
  else if (Uart->Port == USART2)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); /* Enable USART2 Clock */
  }
  else if (Uart->Port == USART3)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); /* Enable USART3 Clock */
  }
  else if (Uart->Port == UART4)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE); /* Enable UART4 Clock */
  }
  else if (Uart->Port == UART5)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE); /* Enable UART5 Clock */
  }
  else if (Uart->Port == USART6)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE); /* Enable USART6 Clock */
  }

  USART_Cmd(Uart->Port, ENABLE);  /* Enable the USART */

  Uart->Transmit = 1;

  USART_ITConfig(Uart->Port, USART_IT_TXE, ENABLE);

  USART_ITConfig(Uart->Port, USART_IT_RXNE, ENABLE);

  if (Uart->Flow & FLOW_CTS_HW)
    USART_ITConfig(Uart->Port, USART_IT_CTS, ENABLE); /* Interupt on CTS changing state */
}

//******************************************************************************

void Uart_Disable(UART *Uart)
{
  Uart->Transmit = 0;

  USART_ITConfig(Uart->Port, USART_IT_TXE, DISABLE);

  USART_ITConfig(Uart->Port, USART_IT_RXNE, DISABLE);

  if (Uart->Flow & FLOW_CTS_HW)
    USART_ITConfig(Uart->Port, USART_IT_CTS, DISABLE);

  USART_Cmd(Uart->Port, DISABLE);  /* Disable the USART */

  if (Uart->Port == USART1)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE); /* Disable USART1 Clock */
  }
  else if (Uart->Port == USART2)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, DISABLE); /* Disable USART2 Clock */
  }
  else if (Uart->Port == USART3)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, DISABLE); /* Disable USART3 Clock */
  }
  else if (Uart->Port == UART4)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, DISABLE); /* Disable UART4 Clock */
  }
  else if (Uart->Port == UART5)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, DISABLE); /* Disable UART5 Clock */
  }
  else if (Uart->Port == USART6)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, DISABLE); /* Disable USART6 Clock */
  }
}

//******************************************************************************

void Uart_Throttle(UART *Uart)
{
  if (Uart->Flow & FLOW_CTS_HW)
    if (USART_GetITStatus(Uart->Port, USART_IT_CTS) != RESET) // Acknowledge any CTS
      USART_ClearITPendingBit(Uart->Port, USART_IT_CTS);  /* Clear the USART CTS interrupt */

#define RTS_THRESH 32

  if (Uart->Flow & FLOW_RTS_SW) // Pin in Software
  {
    if (Uart->Busy || (Fifo_Free(Uart->Rx) < RTS_THRESH)) // Manage flow control in software
  {
      if (Uart->Throttle != 1)
    {
      Uart->Throttle = 1;

        Uart_ModemWriteRTS(Uart, Uart->Throttle);
    }
  }
  else
  {
      if ((!Uart->Busy) && (Uart->Throttle) && (Fifo_Used(Uart->Rx) < RTS_THRESH))
  //    if ((!Uart->Busy) && (Uart->Throttle) && (Fifo_Free(Uart->Rx) > RTS_THRESH))
    {
      Uart->Throttle = 0;

        Uart_ModemWriteRTS(Uart, Uart->Throttle);
      }
    }
  }
}

//******************************************************************************

void Uart_ForwardHalfDuplex(UART *UartA, UART *UartB)
{
  Fifo_Forward(UartA->Rx, UartB->Tx);
}

//******************************************************************************

void Uart_ForwardFullDuplex(UART *UartA, UART *UartB)
{
  Uart_ForwardHalfDuplex(UartA, UartB);
  Uart_ForwardHalfDuplex(UartB, UartA);
}

//****************************************************************************

void Uart_SendBuffer(UART *Uart, DWORD Size, BYTE *Buffer)
{
  DWORD Length;
  DWORD Free;

  while(Size)
  {
    Free = Fifo_Free(Uart->Tx);

    Length = min(Free, Size);

    if (Length && Fifo_Insert(Uart->Tx, Length, Buffer))
    {
      Size   -= Length;
      Buffer += Length;
    }
    else
      Sleep(1);
  }
}

//****************************************************************************

void Uart_SendMessage(UART *Uart, char *Buffer)
{
  Uart_SendBuffer(Uart, (DWORD)strlen(Buffer), (BYTE *)Buffer);
}

//****************************************************************************

void Uart_Flush(UART *Uart)
{
  while(Fifo_Used(Uart->Tx)) // Wait for FIFO to flush
    Sleep(100);
}

//****************************************************************************

void PeriodicUART(int i)
{
  if (i == (1 << (3 - 1))) // RS232
  {
    Uart_EmptyRx(Uart3);
    Uart_FillTx(Uart3, 0);

    Uart_Throttle(Uart3); // Flow Control
  }
  else if (i == (1 << (6 - 1))) // MODEM
  {
    Uart_EmptyRx(Uart6);
    Uart_FillTx(Uart6, 0);

    Uart_Throttle(Uart6); // Flow Control
  }
  else if (i == 0) // Ticker
  {
    Uart_FillTx(Uart3, 1);
    Uart_FillTx(Uart6, 1);

    Uart_Throttle(Uart3); // Flow Control
    Uart_Throttle(Uart6); // Flow Control

    if (GPIO_ReadInputDataBit(Uart6->GSMLED_GPIO.Bank, Uart6->GSMLED_GPIO.Pin))
      CurrentLED |= 2; // YELLOW
    else
      CurrentLED &= ~2;

    if (GPIO_ReadInputDataBit(Uart6->PWRMON_GPIO.Bank, Uart6->PWRMON_GPIO.Pin))
      CurrentLED |= 4; // GREEN
    else
      CurrentLED &= ~4;

    WriteLED(CurrentLED);
  }
}

//******************************************************************************
//
// Modem Hardware Stuff
//
//******************************************************************************

//#define DBG_MODEM

u8 Uart_ModemReadPwrMon(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->PWRMON_GPIO.Bank, Uart->PWRMON_GPIO.Pin)); // Modem PwrMon
}

//******************************************************************************

u8 Uart_ModemReadOnOff(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin)); // Modem OnOff
}

//******************************************************************************

void Uart_ModemWriteOnOff(UART *Uart, int i)
{
  if (i)
  {
#ifdef DBG_MODEM
    printf("Assert   OnOff (HIGH)\r\n");
#endif
    GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);    // Modem OnOff High
  }
  else
  {
#ifdef DBG_MODEM
    printf("Deassert OnOff (LOW)\r\n");
#endif
    GPIO_ResetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);  // Modem OnOff Low
  }
}

//******************************************************************************

u8 Uart_ModemReadGSMLED(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->GSMLED_GPIO.Bank, Uart->GSMLED_GPIO.Pin)); // Modem LED
}

//******************************************************************************

u8 Uart_ModemReadDSR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DSR_GPIO.Bank, Uart->DSR_GPIO.Pin)); // Modem DSR
}

//******************************************************************************

u8 Uart_ModemReadRING(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RING_GPIO.Bank, Uart->RING_GPIO.Pin)); // Modem RING
}

//******************************************************************************

u8 Uart_ModemReadDCD(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DCD_GPIO.Bank, Uart->DCD_GPIO.Pin)); // Modem DCD
}

//******************************************************************************

u8 Uart_ModemReadCTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.Pin)); // Modem CTS
}

//******************************************************************************

u8 Uart_ModemReadRTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin)); // Modem RTS
}

//******************************************************************************

void Uart_ModemWriteRTS(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin);   // RTS High
  else
    GPIO_ResetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin); // RTS Low
}

//******************************************************************************

u8 Uart_ModemReadDTR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin)); // Modem DTR
}

//******************************************************************************

void Uart_ModemWriteDTR(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin);   // DTR High
  else
    GPIO_ResetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin); // DTR Low
}

//******************************************************************************

u8 Uart_ModemReadReset(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin)); // Modem Reset
}

//******************************************************************************

//#define DBG_MODEM_PWR

void Uart_ModemWriteReset(UART *Uart, int i)
{
  if (i)
  {
#ifdef DBG_MODEM
    printf("Assert   Reset (HIGH)\r\n");
#endif
    GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Modem Reset High

for(i=0; i<10; i++)
{
  if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) == Bit_SET)
  {
#ifdef DBG_MODEM
    putchar('H');
#endif
    break;
  }
#ifdef DBG_MODEM
  else
    putchar('L');
#endif

  Sleep(100);
}
#ifdef DBG_MODEM
putchar('\n');
#endif

#ifdef DBG_MODEM
    if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) != Bit_SET)
      printf("Pin not asserting\r\n");
#endif

    for(i=0; i<10; i++)
    {
      if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) == Bit_SET)
      {
#ifdef DBG_MODEM
        putchar('H');
#endif
        break;
      }
#ifdef DBG_MODEM
      else
        putchar('L');
#endif

      Sleep(100);
    }
#ifdef DBG_MODEM
putchar('\n');
#endif
  }
  else
  {
#ifdef DBG_MODEM
    printf("Deassert Reset (LOW)\r\n");
#endif

    GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Modem Set Low

    Sleep(125);

#ifdef DBG_MODEM
    if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) != Bit_RESET)
      printf("Pin not deasserting\r\n");
#endif
  }
}

//****************************************************************************

int Uart_ModemReadPower(UART *Uart)
{
#if 1
  if (Uart_ModemReadPwrMon(Uart) == Bit_RESET)
    return(0);
#endif

  if (Uart_ModemReadReset(Uart) == Bit_RESET)
    return(0);

  return(1);
}

//****************************************************************************

// Display the modem status lines for diagnostic purposes

void Uart_DumpModemPins(UART *Uart)
{
  if (Uart_ModemReadDSR(Uart) == Bit_SET)
    printf("DSR  Hi (I),");
  else
    printf("DSR  Lo (I),");

  if (Uart_ModemReadRING(Uart) == Bit_SET)
    printf("RING Hi (I),");
  else
    printf("RING Lo (I),");

  if (Uart_ModemReadDCD(Uart) == Bit_SET)
    printf("DCD  Hi (I),");
  else
    printf("DCD  Lo (I),");

  if (Uart_ModemReadDTR(Uart) == Bit_SET)
    printf("DTR  Hi (O),");
  else
    printf("DTR  Lo (O),");

  if (Uart_ModemReadCTS(Uart) == Bit_SET)
    printf("CTS  Hi (I),");
  else
    printf("CTS  Lo (I),");

  if (Uart_ModemReadRTS(Uart) == Bit_SET)
    printf("RTS  Hi (O)\r\n");
  else
    printf("RTS  Lo (O)\r\n");


  if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
    printf("PWRMON Hi (I),");
  else
    printf("PWRMON Lo (I),");

  if (Uart_ModemReadReset(Uart) == Bit_SET)
    printf("RESET Hi (O),");
  else
    printf("RESET Lo (O),");

  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    printf("ONOFF Hi (O)\r\n");
  else
    printf("ONOFF Lo (O)\r\n");
}

//****************************************************************************

void Uart_DumpModemPinsBrief(UART *Uart, char *s)
{
  if (Uart_ModemReadDSR(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadRING(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadDCD(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadDTR(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadCTS(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadRTS(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  putchar(' ');

  if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadReset(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  if (Uart_ModemReadGSMLED(Uart) == Bit_SET)
    putchar('H');
  else
    putchar('L');

  printf("%-8s %10d\n", s, SystemTick);
}

//******************************************************************************

void Uart_DebugModemDCD(UART *Uart)
{
  if (Uart_ModemReadDCD(Uart) == Bit_SET) // Check DCD
    puts("DCD Hi");
  else
    puts("DCD Lo");
}

//******************************************************************************

void Uart_DebugModemPwrMon(UART *Uart, int mode)
{
  if (mode)
  {
    if (Uart_ModemReadReset(Uart) == Bit_SET)
      printf("Modem Reset High, ");
    else
      printf("Modem Reset Low,  ");

    if (Uart_ModemReadOnOff(Uart) == Bit_SET)
      printf("Modem OnOff High, ");
    else
      printf("Modem OnOff Low,  ");

    if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
      printf("Modem PwrMon High\r\n");
    else
      printf("Modem PwrMon Low\r\n");
  }
}

//******************************************************************************

void Uart_DebugUart_ModemOnOff(UART *Uart)
{
  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    printf("Modem OnOff High\r\n");
  else
    printf("Modem OnOff Low\r\n");
}

//******************************************************************************

void Uart_ModemReset(UART *Uart)
{
  Uart_ModemWriteReset(Uart, 0); // Modem Reset Low

  Sleep(250);         // At least 200 ms

  Uart_ModemWriteReset(Uart, 1); // Modem Reset High

  Sleep(250);
}

//******************************************************************************

//#define DEBUG
//#define DBG_MODEM
//#define DBG_BRIEF

//******************************************************************************

int Uart_ModemOn(UART *Uart, int PulseWidth)
{
  int i;
  int j = 0;

  Uart->ModemOn = 0;
  Uart->ModemConnected = 0;

#ifdef DBG_MODEM
  printf("Uart_ModemOn (%d)\r\n", PulseWidth);
#endif

  USART6_Suppress(Uart, 0);// Disable USART/IF - Remove suprious loads and voltage that might prevent modem starting

  Sleep(250);

#ifdef DBG_BRIEF
  Uart_DumpModemPinsBrief(Uart, " Initial");
#endif

  if ((Uart_ModemReadReset(Uart) == Bit_RESET) || // Modem Reset (200 ms)
      (Uart_ModemReadOnOff(Uart) == Bit_RESET))
  {
#ifdef DBG_MODEM
    printf("Modem reset released\r\n");
#endif

    Uart_ModemWriteReset(Uart, 1); // Modem Reset High
    Uart_ModemWriteOnOff(Uart, 1); // Modem OnOff High

    for(i=0; i<4; i++) // Sleep(1000)
    {
      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " Z");
#endif
    }
  }

#if 0
  if (Uart_ModemReadPower(Uart)) // Modem PwrMon
  {
#ifdef DBG_MODEM
    printf("Modem already On\r\n");
#endif
  }
  else
#endif
  {
    // Pull OnOff Low for at least one second

    Uart_ModemWriteOnOff(Uart, 0); // Modem OnOff Low

#if 1 // BLINKY
    for(i=0; i<10; i++)
    {
#ifdef DBG_MODEM_PWR
      printf("#%c  - ",'A'+i);
#endif

      Sleep(PulseWidth / 10);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);

#ifdef DEBUG
      Uart_DebugModemPwrMon(Uart, 1);
#endif

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " A");
#endif
    }
#else // BLINKY
    Sleep(PulseWidth); // At least on second
#endif

    Uart_ModemWriteOnOff(Uart, 1); // Modem OnOff High

    if (Uart_ModemReadReset(Uart) == Bit_RESET)
    {
#ifdef DBG_MODEM
      printf("Modem not present\r\n");
#endif

      return(0);
    }

    for(i=0; i<40; i++) // 10 Seconds
    {
#ifdef DBG_MODEM_PWR
      printf("#%2d - ",i);
      Uart_DebugModemPwrMon(Uart, 1);
#endif

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " B");
#endif

      if (Uart_ModemReadPower(Uart)) // Leave early if ON
        break;

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }
  }

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F1");
#endif

  if (!Uart_ModemReadPower(Uart))
  {
#ifdef DBG_MODEM
    printf("Didn't power on right\n"); // It's dead Jim!
#endif

    return(0);
  }
  else
  {
    Sleep(100);

    USART6_Suppress(Uart, 1); // Enable Modem Pins

    Sleep(100);

    for(i=0; i<12; i++) // 3 Seconds
    {
#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F2");
#endif

      if (Uart_ModemReadPwrMon(Uart) == Bit_RESET) // PWRMON drops
        break;

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }

    if (Uart_ModemReadPwrMon(Uart) == Bit_RESET) // PWRMON dropped
    {
#ifdef DBG_MODEM
      printf("Didn't power on right\n");
#endif

      return(0);
    }

    for(i=0; i<12; i++) // 3 Seconds
    {
#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F3");
#endif

      if (Uart_ModemReadDCD(Uart) == Bit_SET) // DCD Goes High when modem comes ready
        break;

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }

    // Expect RING and DCD to go High, and CTS Low here if modem on and functioning

    if (Uart_ModemReadDCD(Uart) == Bit_RESET) // DCD Didn't go High
    {
#ifdef DBG_MODEM
      printf("Didn't power on right\n");
#endif

      return(0);
    }

#ifdef DBG_MODEM
    printf("Modem is GO..\r\n"); // Thunderbirds are go...
#endif

    Uart->Uart_ModemOnTick = SystemTick;
    Uart->ModemOn = 1;

    return(1);
  }
}

//******************************************************************************

void UART_Initialization(void)
{
  // Called prior to hardware initialization

  Uart_Open(Uart3,USART3);

//#ifdef FLOW3
  Uart3->Flow = BootLoader->RS232Flow;
//#endif

  Uart_Open(Uart6,USART6);

#ifdef FLOW6
  Uart6->Flow = FLOW6;
#endif
}

//******************************************************************************
// XModem - C Turvey
//******************************************************************************

#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04

#define ACK 0x06
#define NACK 0x15

#define CAN 0x18

#define TIMEOUT_NXT 5000000

//******************************************************************************

unsigned short CrcByte(unsigned short Crc, unsigned char Byte)
{
  int i;

  Crc ^= (unsigned short)Byte << 8;

  for(i=0; i<8; i++)
    if (Crc & 0x8000)
      Crc = (Crc << 1) ^ 0x1021;
    else
      Crc = (Crc << 1);

  return(Crc);
}

//******************************************************************************

unsigned short CrcBlock(unsigned short Crc, unsigned long Size, unsigned char *Buffer)
{
  int i;

  while(Size--)
  {
    Crc ^= (unsigned short)*Buffer++ << 8;

    for(i=0; i<8; i++)
      if (Crc & 0x8000)
        Crc = (Crc << 1) ^ 0x1021;
      else
        Crc = (Crc << 1);
  }

  return(Crc);
}

//******************************************************************************

int waitforchar(long int i)
{
  while(i--)
  {
    if (Fifo_Used(UartDebug->Rx) != 0)
    {
      unsigned char ch;

      Fifo_Extract(UartDebug->Rx, 1, &ch);

      return((int)ch & 0xFF);
    }
  }

  return(-1);
}

//******************************************************************************

void flush(long int i)
{
  while(i--)
  {
    Fifo_GetChar(UartDebug->Rx);
  }
}

//******************************************************************************

void outchar(int x)
{
  while(Fifo_Free(UartDebug->Tx) == 0) Sleep(1);

  Fifo_Insert(UartDebug->Tx, 1, (void *)&x);
}

//******************************************************************************

int inchar(void)
{
  unsigned char ch;

  while(Fifo_Used(UartDebug->Rx) == 0) Sleep(1);

  Fifo_Extract(UartDebug->Tx, 1, &ch);

  return((int)ch & 0xFF);
}

//******************************************************************************

void xmodemrx(unsigned long addr)
{
  int i, j, k;
  unsigned char x, y, sum;
  unsigned char buffer[1024];
  unsigned short crc, rxcrc, usecrc, useymodemg;

  useymodemg = 0;
  usecrc = 1;

again:

  puts("Waiting for XMODEM transfer..");

//while(1);

  while(1)
  {
    if (useymodemg)
      outchar('G');
    else if (usecrc)
      outchar('C');
    else
      outchar(NACK);

// 10000 10 Hz
// 100000 1 Hz

    i = waitforchar(TIMEOUT_NXT / 2); // periodic

    if ((i == SOH) || (i == STX) || (i == EOT) || (i == 0x1B))
      break;
  }

  while((i == SOH) || (i == STX))
  {
    if (i == SOH)
    {
      k = waitforchar(TIMEOUT_NXT);
      x = (unsigned char)k;
      if (k != -1)
      {
        k = waitforchar(TIMEOUT_NXT);
        y = ~(unsigned char)k;
      }
      else
        y = ~x;

      if ((x == y) && (k != -1))
      {
        sum = 0;
        crc = 0;

        for(j=0; j<128; j++)
        {
          k = waitforchar(TIMEOUT_NXT);

          if (k == -1)
            break;

          x = (unsigned char)k;
          buffer[j] = x;
          sum += x;
          crc = CrcByte(crc, x); // Amortize cost
        }

        if (j == 128)
        {
          if (usecrc)
          {
            k = waitforchar(TIMEOUT_NXT);
            x = (unsigned char)k;
            if (k != -1)
            {
              k = waitforchar(TIMEOUT_NXT);
              y = (unsigned char)k;
            }
            else
              y = x;

//            crc = CrcBlock(0x0000, 128, buffer);

            rxcrc = ((unsigned short)x << 8) + ((unsigned short)y << 0);

            // Flush

            if ((crc == rxcrc) && (k != -1))
            {
              // Write to memory RAM/FLASH, send ACK once done

//              puts("Good 128");

              WriteFlash(addr, 128, buffer);
              addr += 128;

              if (!useymodemg)
                outchar(ACK); // acknowledge if not streaming
            }
            else
            {
//              puts("Bad 128");

              if (useymodemg)
                outchar(CAN); // Cancel transfer
              else
                outchar(NACK);
            }
          }
          else // !usecrc
          {
            x = inchar();

            // Flush

            if (sum == x)
            {
              // Write to memory RAM/FLASH, send ACK once done

//              puts("Good 128");

              WriteFlash(addr, 128, buffer);
              addr += 128;

              outchar(ACK);
            }
            else
            {
//              puts("Bad 128");

              outchar(NACK);
            }
          }
        }
        else // j != 128
        {
          puts("Timeout 128");
          printf("%3d %02X %d\n",j,x,k);

          outchar(NACK);
        }
      }
      else
      {
        // Flush

        puts("Bad Header 128");

        flush(10000);

        outchar(NACK);
      }
    }
    else if (i == STX) // XMODEM-1K
    {
      k = waitforchar(TIMEOUT_NXT);
      x = (unsigned char)k;
      if (k != -1)
      {
        k = waitforchar(TIMEOUT_NXT);
        y = ~(unsigned char)k;
      }
      else
        y = ~x;

      if ((x == y) && (k != -1))
      {
        sum = 0;
        crc = 0;

        for(j=0; j<1024; j++)
        {
          k = waitforchar(TIMEOUT_NXT);

          if (k == -1)
            break;

          x = (unsigned char)k;
          buffer[j] = x;
          sum += x;
          crc = CrcByte(crc, x); // Amortize cost
        }

        if (j == 1024)
        {
          if (usecrc)
          {
            k = waitforchar(TIMEOUT_NXT);
            x = (unsigned char)k;
            if (k != -1)
            {
              k = waitforchar(TIMEOUT_NXT);
              y = (unsigned char)k;
            }
            else
              y = x;

//            crc = CrcBlock(0x0000, 1024, buffer);

            rxcrc = ((unsigned short)x << 8) + ((unsigned short)y << 0);

            // Flush

            if ((crc == rxcrc) && (k != -1))
            {
              // Write to memory RAM/FLASH, send ACK once done

//              puts("Good 1K");

              WriteFlash(addr, 1024, buffer);
              addr += 1024;

              if (!useymodemg)
                outchar(ACK); // Acknowledge if not streaming
            }
            else
            {
              puts("Bad 1K CRC");

              flush(10000);

              if (useymodemg)
                outchar(CAN);
              else
                outchar(NACK);
            }
          }
          else // !usecrc
          {
            k = waitforchar(TIMEOUT_NXT);
            x = (unsigned char)k;

            // Flush

            if ((sum == x) && (k != -1))
            {
              // Write to memory RAM/FLASH, send ACK once done

//              puts("Good 1K");

              WriteFlash(addr, 1024, buffer);
              addr += 1024;

              outchar(ACK);
            }
            else
            {
              puts("Bad 1K Sum");

              flush(10000);

              outchar(NACK);
            }
          }
        }
        else // j != 1024
        {
          puts("Timeout 1K");
          printf("%4d %02X %d\n",j,x,k);

          if (useymodemg)
            outchar(CAN);
          else
            outchar(NACK);
        }
      }
      else
      {
        // Flush

        puts("Bad Header 1K");

        flush(10000);

        if (useymodemg)
          outchar(CAN);
        else
          outchar(NACK);
      }
    }

    i = waitforchar(TIMEOUT_NXT * 10); // periodic

    if (i == -1)
    {
      puts("Timeout Next Packet");
    }
  }

  if (i == EOT)
  {
    puts("Done");

    outchar(ACK);
  }

  if (i != 0x1B)
    goto again;
}

//******************************************************************************

void USART3_Configuration(UART *Uart)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  // Initialization of Terminus 2 (T2) RS232 port for debugging

  RCC_AHB1PeriphClockCmd(
    RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF,
    ENABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); /* Enable USART3 Clock */

  /* Configure PF3 FORCEON, PF4 FORCEOFF - RS232 */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
#ifdef REV2
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // Pulled down
#else // REV1
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#endif
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  GPIO_SetBits(GPIOF, GPIO_Pin_3 | GPIO_Pin_4);  // Both High

  /* USART RS232 */

  Uart->TX_GPIO.Bank = GPIOB; // PB10
  Uart->TX_GPIO.Pin = GPIO_Pin_10;
  Uart->TX_GPIO.PinSource = GPIO_PinSource10;

  Uart->RX_GPIO.Bank = GPIOB; // PB11
  Uart->RX_GPIO.Pin = GPIO_Pin_11;
  Uart->RX_GPIO.PinSource = GPIO_PinSource11;

  Uart->CTS_GPIO.Bank = GPIOD; // PD11
  Uart->CTS_GPIO.Pin = GPIO_Pin_11;
  Uart->CTS_GPIO.PinSource = GPIO_PinSource11;

  Uart->RTS_GPIO.Bank = GPIOD; // PD12
  Uart->RTS_GPIO.Pin = GPIO_Pin_12;
  Uart->RTS_GPIO.PinSource = GPIO_PinSource12;

  /* Define the AF function, we can still choose between AF/GPIO later */
  GPIO_PinAFConfig(Uart->TX_GPIO.Bank, Uart->TX_GPIO.PinSource, GPIO_AF_USART3); /* Connect PB10 to USART3_Tx */
  GPIO_PinAFConfig(Uart->RX_GPIO.Bank, Uart->RX_GPIO.PinSource, GPIO_AF_USART3); /* Connect PB11 to USART3_Rx */
  GPIO_PinAFConfig(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.PinSource, GPIO_AF_USART3); /* Connect PD11 to USART3_CTS */
  GPIO_PinAFConfig(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.PinSource, GPIO_AF_USART3); /* Connect PD12 to USART3_RTS */

  /* Configure USART3 Tx & Rx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Pin = Uart->TX_GPIO.Pin;
  GPIO_Init(Uart->TX_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RX_GPIO.Pin;
  GPIO_Init(Uart->RX_GPIO.Bank, &GPIO_InitStructure);

  /* Configure USART3 CTS as alternate function or input */

  if (Uart->Flow & FLOW_CTS_HW)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; // Defaults as input

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Configure USART3 RTS as alternate function or output */

  if (Uart->Flow & FLOW_RTS_HW)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; // Defaults as output

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_ResetBits(GPIOD, GPIO_InitStructure.GPIO_Pin); // Defaults to low (Request To Send = Yes)

  USART_InitStructure.USART_BaudRate = BootLoader->BaudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;

  if (Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW))
  {
    if ((Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW)) == (FLOW_RTS_HW | FLOW_CTS_HW))
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    else if (Uart->Flow & FLOW_RTS_HW)
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
    else
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
  }
  else
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART3, &USART_InitStructure); /* Configure USART */

  USART_Cmd(USART3, ENABLE);  /* Enable the USART */
}

//******************************************************************************

void USART6_Suppress(UART *Uart, int Enable)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* 910 Modem wants all USART pins in undriven state to start */

  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

  /* Configure (DSR), (RING), (DCD) as input */
  GPIO_InitStructure.GPIO_Pin = Uart->DSR_GPIO.Pin;
  GPIO_Init(Uart->DSR_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RING_GPIO.Pin;
  GPIO_Init(Uart->RING_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->DCD_GPIO.Pin;
  GPIO_Init(Uart->DCD_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure USART6 Tx & Rx as alternate function */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->TX_GPIO.Pin;
  GPIO_Init(Uart->TX_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RX_GPIO.Pin;
  GPIO_Init(Uart->RX_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (DTR) as output */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->DTR_GPIO.Pin;
  GPIO_Init(Uart->DTR_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (RTS) as output */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    if (Uart->Flow & FLOW_RTS_HW)
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    else
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->RTS_GPIO.Pin;
  GPIO_Init(Uart->RTS_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (CTS) as input */
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    if (Uart->Flow  & FLOW_CTS_HW)
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    else
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->CTS_GPIO.Pin;
  GPIO_Init(Uart->CTS_GPIO.Bank, &GPIO_InitStructure);
}

//******************************************************************************

void USART6_Configuration(UART *Uart)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  RCC_AHB1PeriphClockCmd(
    RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG,
    ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE); /* Enable USART6 Clock */

  /* USART MODEM */

  // PC.03 (RESET*)
  Uart->RESET_GPIO.Bank = GPIOC;
  Uart->RESET_GPIO.Pin = GPIO_Pin_3;
  Uart->RESET_GPIO.PinSource = GPIO_PinSource3;

  // PC.02 (ON_OFF)
  Uart->ONOFF_GPIO.Bank = GPIOC;
  Uart->ONOFF_GPIO.Pin = GPIO_Pin_2;
  Uart->ONOFF_GPIO.PinSource = GPIO_PinSource2;

#ifdef REV2 // PG.00 (PWRMON*)
  Uart->PWRMON_GPIO.Bank = GPIOG;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_0;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource0;
#else // REV1  PC.13 (PWRMON*)
  Uart->PWRMON_GPIO.Bank = GPIOC;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_13;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource13;
#endif

#ifdef REV2 // PG.01 (GSM_LED)
  Uart->GSMLED_GPIO.Bank = GPIOG;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_1;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource1;
#else // REV1  PC.14 (GSM_LED)
  Uart->GSMLED_GPIO.Bank = GPIOC;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_14;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource14;
#endif

  Uart->TX_GPIO.Bank = GPIOC; // PC6
  Uart->TX_GPIO.Pin = GPIO_Pin_6;
  Uart->TX_GPIO.PinSource = GPIO_PinSource6;

  Uart->RX_GPIO.Bank = GPIOC; // PC7
  Uart->RX_GPIO.Pin = GPIO_Pin_7;
  Uart->RX_GPIO.PinSource = GPIO_PinSource7;

  Uart->CTS_GPIO.Bank = GPIOG; // PG15
  Uart->CTS_GPIO.Pin = GPIO_Pin_15;
  Uart->CTS_GPIO.PinSource = GPIO_PinSource15;

  Uart->RTS_GPIO.Bank = GPIOG; // PG12
  Uart->RTS_GPIO.Pin = GPIO_Pin_12;
  Uart->RTS_GPIO.PinSource = GPIO_PinSource12;

  Uart->DSR_GPIO.Bank = GPIOD; // PD3
  Uart->DSR_GPIO.Pin = GPIO_Pin_3;
  Uart->DSR_GPIO.PinSource = GPIO_PinSource3;

  Uart->RING_GPIO.Bank = GPIOD; // PD7
  Uart->RING_GPIO.Pin = GPIO_Pin_7;
  Uart->RING_GPIO.PinSource = GPIO_PinSource7;

  Uart->DCD_GPIO.Bank = GPIOD; // PD8
  Uart->DCD_GPIO.Pin = GPIO_Pin_8;
  Uart->DCD_GPIO.PinSource = GPIO_PinSource8;

  Uart->DTR_GPIO.Bank = GPIOD; // PD9
  Uart->DTR_GPIO.Pin = GPIO_Pin_9;
  Uart->DTR_GPIO.PinSource = GPIO_PinSource9;

  /* Define the AF function, we can still choose between AF/GPIO later */
  GPIO_PinAFConfig(Uart->TX_GPIO.Bank, Uart->TX_GPIO.PinSource, GPIO_AF_USART6); /* Connect PC6 to USART6_Tx */
  GPIO_PinAFConfig(Uart->RX_GPIO.Bank, Uart->RX_GPIO.PinSource, GPIO_AF_USART6); /* Connect PC7 to USART6_Rx */
  GPIO_PinAFConfig(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG15 to USART6_CTS */
  GPIO_PinAFConfig(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG12 to USART6_RTS */

  USART6_Suppress(Uart, 0); // Hi-Z the modem pins

  USART_InitStructure.USART_BaudRate = BootLoader->BaudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;

  if (Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW))
  {
    if ((Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW)) == (FLOW_RTS_HW | FLOW_CTS_HW))
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    else if (Uart->Flow & FLOW_RTS_HW)
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
    else
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
  }
  else
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART6, &USART_InitStructure); /* Configure USART */

  Uart_Enable(Uart); /* Enable USART */

  // Modem Control

  /* Configure (PWRMON*) as input pulled-down */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = Uart->PWRMON_GPIO.Pin;
  GPIO_Init(Uart->PWRMON_GPIO.Bank, &GPIO_InitStructure);

  /* Configure (GSM_LED) as input pulled-down */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = Uart->GSMLED_GPIO.Pin;
  GPIO_Init(Uart->GSMLED_GPIO.Bank, &GPIO_InitStructure);

  /* Configure (ON_OFF) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin =  Uart->ONOFF_GPIO.Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->ONOFF_GPIO.Bank, &GPIO_InitStructure);

  GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin); // Default High

  /* Configure PC.01 (ENABLE_SUPPLY) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_SetBits(GPIOC, GPIO_InitStructure.GPIO_Pin); // Plug_in terminal power supply is enabled

  /* Configure (RESET*) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD; //??
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = Uart->RESET_GPIO.Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->RESET_GPIO.Bank, &GPIO_InitStructure);

  Sleep(50);
  GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Reset Low
  Sleep(250);                                                   // Hold low for at least 200 ms
  GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Reset High
  Sleep(50);

  /* Configure PC.04 (SERVICE) ??TODO */
  /* Configure PC.05 (ENABLE_VBUS) ??TODO */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_SetBits(GPIOC, GPIO_InitStructure.GPIO_Pin); // Set both High
}

//******************************************************************************

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

  /* Configure PE.12 (RED_LED), PE.13 (YLW_LED), PE.14 (GRN_LED) */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  CurrentLED = 0;
  WriteLED(CurrentLED); /* All LEDs Off */

  /* Ensure that the Modem module is powered down over a reset */

  /* Configure PC.01 (ENABLE_SUPPLY) and PC.03 (RESET*) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_ResetBits(GPIOC, GPIO_InitStructure.GPIO_Pin); // Plug_in terminal power supply is disabled
}

//****************************************************************************

static const char Hex[] = "0123456789ABCDEF";

//****************************************************************************

// Generic but efficient hexidecimal memory dumping routines

char *DumpLine(DWORD Addr, DWORD Size, BYTE *Buffer)
{
  DWORD i;
  char *p;
  static char OutLine[80];

  p = OutLine;

  *p++ = Hex[((Addr >> 28) & 0x0F)];
  *p++ = Hex[((Addr >> 24) & 0x0F)];
  *p++ = Hex[((Addr >> 20) & 0x0F)];
  *p++ = Hex[((Addr >> 16) & 0x0F)];
  *p++ = Hex[((Addr >> 12) & 0x0F)];
  *p++ = Hex[((Addr >>  8) & 0x0F)];
  *p++ = Hex[((Addr >>  4) & 0x0F)];
  *p++ = Hex[((Addr >>  0) & 0x0F)];

  *p++ = ' ';
  *p++ = ':';
  *p++ = ' ';

  for(i=0; i<Size; i++)
  {
    *p++ = Hex[((Buffer[i] >> 4) & 0x0F)];
    *p++ = Hex[((Buffer[i] >> 0) & 0x0F)];

    if (i == 7)
      *p++ = '-';
    else
      *p++ = ' ';
  }

  for(i=Size; i<16; i++)
  {
    *p++ = ' ';
    *p++ = ' ';
    *p++ = ' ';
  }

  for(i=0; i<Size; i++)
  {
    if ((Buffer[i] >= 32) && (Buffer[i] <= 126))
      *p++ = Buffer[i];
    else
      *p++ = '.';
  }

  *p++ = '\r';
  *p++ = '\n';

  *p++ = 0;

  return(OutLine);
}

//****************************************************************************

void DumpData(DWORD Size, BYTE *Buffer)
{
  DWORD Addr;
  DWORD Length;

  Addr = 0;

  while(Size)
  {
    Length = min(16, Size);

    Uart_SendMessage(UartDebug, DumpLine(Addr, Length, Buffer));

    Buffer += Length;

    Addr += Length;
    Size -= Length;
  }
}

//****************************************************************************

void DumpMemory(DWORD Addr, DWORD Size, BYTE *Buffer)
{
  DWORD Length;

  while(Size)
  {
    Length = min(16, Size);

    Uart_SendMessage(UartDebug, DumpLine(Addr, Length, Buffer));

    Buffer += Length;

    Addr += Length;
    Size -= Length;
  }
}

//******************************************************************************

void BootLoaderConfiguration(void)
{
  BootLoader_Open(BootLoader);
}

//******************************************************************************

int BaudTable[] = { 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600,
    115200, 230400, 460800, 921600, 0 };

int BaudSelect(int Current)
{
  char x;
  int i;
  char string[32];

  Uart_SendMessage(UartDebug, "Baud Rate Selection\r\n");

  i = 0;

  while(BaudTable[i])
  {
    sprintf(string,"  %c %c %6d\r\n",
      'A'+i,
      (Current == BaudTable[i]) ? '*' : ' ',
      BaudTable[i]);

    Uart_SendMessage(UartDebug, string);

    i++;
  }

  Uart_SendMessage(UartDebug, "Baud> ");

  while(1)
  {
    x = Fifo_GetChar(UartDebug->Rx); // Key strokes on UART/Debug Console

    if ((x > 'A') && (x < ('A'+i)))
      Current = BaudTable[x - 'A'];
    else if ((x > 'a') && (x < ('a'+i)))
      Current = BaudTable[x - 'a'];

    if (x)
    {
      sprintf(string, "%c\r\n",x);
      Uart_SendMessage(UartDebug, string);
      break;
    }
  }

  sprintf(string, "Selected %d\r\n", Current);

  Uart_SendMessage(UartDebug, string);

  return(Current);
}

//******************************************************************************

int FlowSelect(int Current)
{
  char x;
  char string[32];

  Uart_SendMessage(UartDebug, "Flow Control Selection (RS232)\r\n");

  sprintf(string, "Current  %s\r\n", Current ? "ON" : "OFF");
  Uart_SendMessage(UartDebug, string);

  Uart_SendMessage(UartDebug, "Change Y/N? ");

  while(1)
  {
    x = Fifo_GetChar(UartDebug->Rx); // Key strokes on UART/Debug Console

    if ((x == 'Y') || (x == 'y'))
    {
      if (Current)
        Current = 0; // OFF
      else
        Current = FLOW3; // ON
    }

    if (x)
    {
      sprintf(string, "%c\r\n",x);
      Uart_SendMessage(UartDebug, string);
      break;
    }
  }

  sprintf(string, "Selected %s\r\n", Current ? "ON" : "OFF");
  Uart_SendMessage(UartDebug, string);

  return(Current);
}

//******************************************************************************

#define MAXLINE 80

char *GetLine(UART *Uart)
{
  static char InputLine[MAXLINE+1];
  int i;

  i = 0;

  while(1)
  {
    char x;

    x = Fifo_GetChar(Uart->Rx); // Key strokes on UART/Debug Console

    if ((x == '\n') || (x == '\r'))
    {
//      printf("[%02X]",x);

      if (x == '\r') // 0x0D (RETURN/ENTER), we will eat 0x0A
      {
        Fifo_Insert(Uart->Tx, 2, "\r\n"); // CR/LF

        break;
      }
    }
    else if ((x == 0x08) || (x == 0x7F))
    {
      if (i)
      {
        i--;

        Fifo_Insert(Uart->Tx, 3, "\010 \010"); // Backspace/Erase
      }
    }
    else if (x)
    {
      if (i < MAXLINE)
        InputLine[i++] = x;

      Fifo_Insert(Uart->Tx, 1, (BYTE *)&x); // Echo to console
    }

    __WFI(); // Lower power than grinding
  }

  InputLine[i] = 0;

  return(InputLine);
}

//******************************************************************************

void BootLoaderMenu(void)
{
  Uart_SendMessage(UartDebug,
    "Boot Loader\r\n"
    "E - Erase Application Flash\r\n"
    "X - XModem Download\r\n"
    "D - Dump Memory\r\n"
    "B - Select Baud Rate\r\n"
    "Z - Select Flow Control\r\n"
    "I - Set Initial AT command\r\n"
#ifdef CLK_DIAG
    "C - Clock Diagnostic\r\n"
#endif
    "F - Fire DfuSe\r\n"
    "M - Modem Console\r\n");
}

//******************************************************************************

void BootLoaderConsole(void)
{
  RCC_ClocksTypeDef RCC_ClockFreq;
  DWORD Addr = APPLICATION_BASE;
  char x;
  char string[32];

  BootLoaderMenu();

  while(1)
  {
    Uart_SendMessage(UartDebug, "BL>"); // Output prompt

    while(!Fifo_Used(UartDebug->Rx)) { __WFI(); } // Wait for a character

    x = Fifo_GetChar(UartDebug->Rx); // Key strokes on UART/Debug Console

    if ((x >= 0x20) && (x <= 0x7E))
    {
      sprintf(string,"%c", x); // Output character
      Uart_SendMessage(UartDebug, string);
    }

    Uart_SendMessage(UartDebug, "\r\n");

    if (x == '?')
      BootLoaderMenu();

    if ((x == 'E') || (x == 'e'))
    {
      Uart_SendMessage(UartDebug, "Erasing..\r\n");
      EraseFlash(APPLICATION_BASE);
      Uart_SendMessage(UartDebug, "Erase Complete\r\n");
      Addr = APPLICATION_BASE;
    }

    if ((x == 'X') || (x == 'x'))
    {
      xmodemrx(APPLICATION_BASE);
      Addr = APPLICATION_BASE;
    }

    if ((x == 'D') || (x == 'd'))
    {
      DumpMemory(Addr, 0x100, (void *)Addr);
      Addr += 0x100;
      if (Addr == (APPLICATION_BASE + 0x20000))
        Addr = APPLICATION_BASE;
    }

#ifdef CLK_DIAG
    if ((x == 'C') || (x == 'c'))
    {
      Uart_Flush(UartDebug);
      while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); // Wait for Transmission to complete
      ClockDiagnostic();
    }
#endif

    if ((x == 'F') || (x == 'f')) // DFuse
    {
      Uart_SendMessage(UartDebug, "Going DfuSe...\r\n");
      Uart_Flush(UartDebug);
      while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); // Wait for Transmission to complete
      *((DWORD *)0x2001FFFC) = 0xBEEFBEEF;
      SystemReset();
    }

    if ((x == 'I') || (x == 'i')) // Initial AT command to send to modem at power up
    {
      char *s;
      int i;

      Uart_SendMessage(UartDebug, "Current [");
      Uart_SendMessage(UartDebug, BootLoader->InitialAT);
      Uart_SendMessage(UartDebug, "]\r\nNew     :");

      s = GetLine(UartDebug);

      i = strlen(s);

      if (i > 31) // Truncate at maximal length
        s[31] = 0;

      if (strcmp(s, BootLoader->InitialAT) != 0)
      {
        strcpy(BootLoader->InitialAT, s);
        BootLoader->Updated = 1;
        BootLoader_WriteConfig(BootLoader);

        Uart_SendMessage(UartDebug, "Updated :");
        Uart_SendMessage(UartDebug, BootLoader->InitialAT);
        Uart_SendMessage(UartDebug, "\r\n");
      }
    }

    if ((x == 'B') || (x == 'b'))
    {
      char ipr[32];
      int BaudRate;

      BaudRate = BaudSelect(BootLoader->BaudRate);

      Uart_Flush(UartDebug);
      while(USART_GetFlagStatus(UartDebug->Port, USART_FLAG_TC) == RESET); // Wait for Transmission to complete

      // Send AT+IPR= to Modem?
//      sprintf(ipr,"AT+IPR=%d\r\n",BaudRate);
      sprintf(ipr,"AT+IPR=%d;&P0;&W0\r\n",BaudRate); // Change and Write to Profile
      Uart_SendMessage(UartModem, ipr);

      Uart_Flush(UartModem);
      while(USART_GetFlagStatus(UartModem->Port, USART_FLAG_TC) == RESET); // Wait for Transmission to complete

      RCC_GetClocksFreq(&RCC_ClockFreq);
      USART3->BRR = RCC_ClockFreq.PCLK1_Frequency / BaudRate;
      USART6->BRR = RCC_ClockFreq.PCLK2_Frequency / BaudRate;

      if (BaudRate != BootLoader->BaudRate) // Update Flash/EEPROM settings when changing
      {
        BootLoader->BaudRate = BaudRate;
        BootLoader->Updated = 1;
        BootLoader_WriteConfig(BootLoader);
      }
    }

    if ((x == 'Z') || (x == 'z'))
    {
      int RS232Flow;

      RS232Flow = FlowSelect(BootLoader->RS232Flow);

      if (RS232Flow != BootLoader->RS232Flow) // Update Flash/EEPROM settings when changing
      {
        BootLoader->RS232Flow = RS232Flow;
        BootLoader->Updated = 1;
        BootLoader_WriteConfig(BootLoader);
      }
    }

    if ((x == 'M') || (x == 'm')) // Modem Forwarding
    {
      // Modem Console
      Uart_SendMessage(UartDebug, "Modem Console\r\n");

      while(1) // Forwarding Loop
      {
        Uart_ForwardFullDuplex(UartModem, UartDebug); // Modem to/from Debug

        __WFI();
      }
    }
  }
}

//******************************************************************************

void BootLoaderApplication(void)
{
  RCC_ClocksTypeDef RCC_Clocks;

  NVIC_Configuration();

  /* SysTick end of count event each 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);

  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

  NVIC_SetPriority(SysTick_IRQn, SysTickPriority);

  GPIO_Configuration(); // General Pins - Do first

  CurrentLED |= 1; // RED

  USART6_Configuration(Uart6); // Modem Pins and USART

  Uart_Enable(Uart3); // Enable the interrupt
  Uart_Enable(Uart6);

  // Press ESC to break to menu

#ifndef SILENT
  printf("Starting Modem.. Press ESC to break to console\r\n");
#endif

//  Uart_ModemReset(UartModem); // Modem Reset

  if (!Uart_ModemOn(UartModem, BootLoader->PowerPulse)) // Try programmed value first
  {
    if (Uart_ModemOn(UartModem, (BootLoader->PowerPulse == POWER_ON_FAST) ? POWER_ON_SLOW : POWER_ON_FAST)) // Switch on Failure
    {
      // Succeeded on alternate, save that for next time

      BootLoader->PowerPulse = (BootLoader->PowerPulse == POWER_ON_FAST) ? POWER_ON_SLOW : POWER_ON_FAST;
      BootLoader->Updated = 1;

      BootLoader_WriteConfig(BootLoader); // Update Flash/EEPROM settings when changing
    }

    // If two methods failed either modem not present or failed
  }

  if (!UartModem->ModemOn) // Modem hasn't started, drop to console
  {
    BootLoaderConsole();
  }

  if (Fifo_Used(UartDebug->Rx)) // Check for breakout
  {
    char x;

    x = Fifo_GetChar(UartDebug->Rx); // Key strokes on UART/Debug Console

    if (x == 0x1B) // ESC
      BootLoaderConsole();
  }

#ifndef SILENT
  // Modem Console
  Uart_SendMessage(UartDebug, "Modem Console\r\n");
#endif

  if (BootLoader->InitialAT[0])
  {
    Uart_SendMessage(UartModem, BootLoader->InitialAT); // For example ATI4 Enquire Modem Model
    Uart_SendMessage(UartModem, "\r\n");
  }

  while(1) // Forwarding Loop
  {
    Uart_ForwardFullDuplex(UartModem, UartDebug); // Modem to/from Debug

    __WFI();
  }
}

//******************************************************************************

// After Joseph Yiu's implementation

void hard_fault_handler_c(unsigned int * hardfault_args)
{
  printf ("[Hard Fault]\n");

  printf ("r0 = %08X, r1 = %08X, r2 = %08X, r3 = %08X\n",
    hardfault_args[0], hardfault_args[1], hardfault_args[2], hardfault_args[3]);
  printf ("r12= %08X, lr = %08X, pc = %08X, psr= %08X\n",
    hardfault_args[4], hardfault_args[5], hardfault_args[6], hardfault_args[7]);

  printf ("bfar=%08X, cfsr=%08X, hfsr=%08X, dfsr=%08X, afsr=%08X\n",
    *((volatile unsigned long *)(0xE000ED38)),
    *((volatile unsigned long *)(0xE000ED28)),
    *((volatile unsigned long *)(0xE000ED2C)),
    *((volatile unsigned long *)(0xE000ED30)),
    *((volatile unsigned long *)(0xE000ED3C)) );

  while(1);

  return;
}

//******************************************************************************
// Hosting of stdio functionality through USART3
//******************************************************************************

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
  static int last;

  if ((ch == (int)'\n') && (last != (int)'\r'))
  {
    last = (int)'\r';

    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

    USART_SendData(USART3, last);
  }
  else
    last = ch;

  while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

  USART_SendData(USART3, ch);

  return(ch);
}

int fgetc(FILE *f)
{
  char ch;

  while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET);

  ch = USART_ReceiveData(USART3);

  return((int)ch);
}

int ferror(FILE *f)
{
  /* Your implementation of ferror */
  return EOF;
}

void _ttywrch(int ch)
{
  static int last;

  if ((ch == (int)'\n') && (last != (int)'\r'))
  {
    last = (int)'\r';

    while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

    USART_SendData(USART3, last);
  }
  else
    last = ch;

  while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

  USART_SendData(USART3, ch);
}

void _sys_exit(int return_code)
{
label:  goto label;  /* endless loop */
}

//******************************************************************************

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while(1); /* Infinite loop */
}
#endif

//******************************************************************************

int main(void)
{
  DWORD BootState;

  /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */

  EnableTiming(); // Enable core cycle counter

  BootLoader->BaudRate = MODEM_BAUD;
  BootLoader->PowerPulse = POWER_ON_FAST;
  BootLoader->RS232Flow = FLOW3;

  BootLoaderConfiguration();  // Read Configuration burned in FLASH

  UART_Initialization(); // Need to initialize these structures really early

  USART3_Configuration(Uart3); // Setup USART for debugging on T2

#ifndef SILENT
#ifdef REV2
  printf("\n\n\nJanus Remote Communications - TERMINUS 2 R1 STM32F4 - Boot Loader\nFW: %s %s (v2.4)\n\n", __DATE__, __TIME__);
#else // REV1
  printf("\n\n\nJanus Remote Communications - TERMINUS 2 R0 STM32F4 - Boot Loader\nFW: %s %s (v2.4)\n\n", __DATE__, __TIME__);
#endif
#endif // SILENT

  BootState = *((DWORD *)0x2001FFFC);

  *((DWORD *)0x2001FFFC) = 0xAAAA5555; // Invalidate

  if (BootState == 0xCAFE0001)
    EraseFlash(APPLICATION_BASE);

  if (ApplicationFirmwareCheck() && !BreakKeyCheck())
  {
#ifndef SILENT
    printf("Application Valid, Starting\n");
#endif

    while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); /* Wait for characters to leave */

    USART_Cmd(USART3, DISABLE);  /* Disable the USART3 */

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, DISABLE); /* Disable USART3 Clock */

    Reboot_Application(); // Does not return
  }
  else
  {
#ifndef SILENT
    printf("Starting Boot Loader Application\n");
#endif

    BootLoaderApplication();
  }

  while(1); /* Infinite loop */
}

//******************************************************************************

