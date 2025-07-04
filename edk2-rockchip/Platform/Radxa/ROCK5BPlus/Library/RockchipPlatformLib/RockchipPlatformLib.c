/** @file
*
*  Copyright (c) 2021, Rockchip Limited. All rights reserved.
*  Copyright (c) 2023-2024, Mario Bălănică <mariobalanica02@gmail.com>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/GpioLib.h>
#include <Library/RK806.h>
#include <Library/Rk3588Pcie.h>
#include <Library/PWMLib.h>
#include <Soc.h>
#include <VarStoreData.h>

static struct regulator_init_data  rk806_init_data[] = {
  /* Master PMIC */
  RK8XX_VOLTAGE_INIT (MASTER_BUCK1,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK3,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK4,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK5,  850000),
  // RK8XX_VOLTAGE_INIT(MASTER_BUCK6, 750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK7,  2000000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK8,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK10, 1800000),

  RK8XX_VOLTAGE_INIT (MASTER_NLDO1,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO2,  850000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO3,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO4,  850000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO5,  750000),

  RK8XX_VOLTAGE_INIT (MASTER_PLDO1,  1800000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO2,  1800000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO3,  1200000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO4,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO5,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO6,  1800000),

  /* No dual PMICs on this platform */
};

VOID
EFIAPI
SdmmcIoMux (
  VOID
  )
{
  /* sdmmc0 iomux (microSD socket) */
  BUS_IOC->GPIO4D_IOMUX_SEL_L  = (0xFFFFUL << 16) | (0x1111); // SDMMC_D0,SDMMC_D1,SDMMC_D2,SDMMC_D3
  BUS_IOC->GPIO4D_IOMUX_SEL_H  = (0x00FFUL << 16) | (0x0011); // SDMMC_CLK,SDMMC_CMD
  PMU1_IOC->GPIO0A_IOMUX_SEL_H = (0x000FUL << 16) | (0x0001); // SDMMC_DET
}

VOID
EFIAPI
SdhciEmmcIoMux (
  VOID
  )
{
  /* sdhci0 iomux (eMMC socket) */
  BUS_IOC->GPIO2A_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x1111); // EMMC_CMD,EMMC_CLKOUT,EMMC_DATASTROBE,EMMC_RSTN
  BUS_IOC->GPIO2D_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x1111); // EMMC_D0,EMMC_D1,EMMC_D2,EMMC_D3
  BUS_IOC->GPIO2D_IOMUX_SEL_H = (0xFFFFUL << 16) | (0x1111); // EMMC_D4,EMMC_D5,EMMC_D6,EMMC_D7
}

#define NS_CRU_BASE       0xFD7C0000
#define CRU_CLKSEL_CON59  0x03EC
#define CRU_CLKSEL_CON78  0x0438

VOID
EFIAPI
Rk806SpiIomux (
  VOID
  )
{
  /* io mux */
  // BUS_IOC->GPIO1A_IOMUX_SEL_H = (0xFFFFUL << 16) | 0x8888;
  // BUS_IOC->GPIO1B_IOMUX_SEL_L = (0x000FUL << 16) | 0x0008;
  PMU1_IOC->GPIO0A_IOMUX_SEL_H = (0x0FF0UL << 16) | 0x0110;
  PMU1_IOC->GPIO0B_IOMUX_SEL_L = (0xF0FFUL << 16) | 0x1011;
  MmioWrite32 (NS_CRU_BASE + CRU_CLKSEL_CON59, (0x00C0UL << 16) | 0x0080);
}

VOID
EFIAPI
Rk806Configure (
  VOID
  )
{
  UINTN  RegCfgIndex;

  RK806Init ();

  RK806PinSetFunction (MASTER, 1, 2); // rk806_dvs1_pwrdn

  for (RegCfgIndex = 0; RegCfgIndex < ARRAY_SIZE (rk806_init_data); RegCfgIndex++) {
    RK806RegulatorInit (rk806_init_data[RegCfgIndex]);
  }
}

VOID
EFIAPI
SetCPULittleVoltage (
  IN UINT32  Microvolts
  )
{
  struct regulator_init_data  Rk806CpuLittleSupply =
    RK8XX_VOLTAGE_INIT (MASTER_BUCK2, Microvolts);

  RK806RegulatorInit (Rk806CpuLittleSupply);
}

VOID
EFIAPI
NorFspiIomux (
  VOID
  )
{
  /* io mux */
  MmioWrite32 (
    NS_CRU_BASE + CRU_CLKSEL_CON78,
    (((0x3 << 12) | (0x3f << 6)) << 16) | (0x0 << 12) | (0x3f << 6)
    );
  #define FSPI_M1
 #if defined (FSPI_M0)
  /*FSPI M0*/
  BUS_IOC->GPIO2A_IOMUX_SEL_L = ((0xF << 0) << 16) | (2 << 0);   // FSPI_CLK_M0
  BUS_IOC->GPIO2D_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x2222);     // FSPI_D0_M0,FSPI_D1_M0,FSPI_D2_M0,FSPI_D3_M0
  BUS_IOC->GPIO2D_IOMUX_SEL_H = ((0xF << 8) << 16) | (0x2 << 8); // FSPI_CS0N_M0
 #elif defined (FSPI_M1)
  /*FSPI M1*/
  BUS_IOC->GPIO2A_IOMUX_SEL_H = (0xFF00UL << 16) | (0x3300); // FSPI_D0_M1,FSPI_D1_M1
  BUS_IOC->GPIO2B_IOMUX_SEL_L = (0xF0FFUL << 16) | (0x3033); // FSPI_D2_M1,FSPI_D3_M1,FSPI_CLK_M1
  BUS_IOC->GPIO2B_IOMUX_SEL_H = (0xF << 16) | (0x3);         // FSPI_CS0N_M1
 #else
  /*FSPI M2*/
  BUS_IOC->GPIO3A_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x5555); // [FSPI_D0_M2-FSPI_D3_M2]
  BUS_IOC->GPIO3A_IOMUX_SEL_H = (0xF0UL << 16) | (0x50);     // FSPI_CLK_M2
  BUS_IOC->GPIO3C_IOMUX_SEL_H = (0xF << 16) | (0x2);         // FSPI_CS0_M2
 #endif
}

VOID
EFIAPI
GmacIomux (
  IN UINT32  Id
  )
{
  /* No GMAC here */
}

VOID
EFIAPI
NorFspiEnableClock (
  UINT32  *CruBase
  )
{
  UINTN  BaseAddr = (UINTN)CruBase;

  MmioWrite32 (BaseAddr + 0x087C, 0x0E000000);
}

VOID
EFIAPI
I2cIomux (
  UINT32  id
  )
{
  switch (id) {
    case 0:
      GpioPinSetFunction (0, GPIO_PIN_PD1, 3); // i2c0_scl_m2
      GpioPinSetFunction (0, GPIO_PIN_PD2, 3); // i2c0_sda_m2
      break;
    case 1:
      GpioPinSetFunction (0, GPIO_PIN_PD4, 9); // i2c1_scl_m2
      GpioPinSetFunction (0, GPIO_PIN_PD5, 9); // i2c1_sda_m2
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      GpioPinSetFunction (2, GPIO_PIN_PB5, 9); // i2c4_scl_m1
      GpioPinSetFunction (2, GPIO_PIN_PB4, 9); // i2c4_sda_m1
      break;
    case 5:
      break;
    case 6:
      GpioPinSetFunction (0, GPIO_PIN_PD0, 9); // i2c6_scl_m0
      GpioPinSetFunction (0, GPIO_PIN_PC7, 9); // i2c6_sda_m0
      break;
    case 7:
      GpioPinSetFunction (1, GPIO_PIN_PD0, 9); // i2c7_scl_m0
      GpioPinSetFunction (1, GPIO_PIN_PD1, 9); // i2c7_sda_m0
      break;
    default:
      break;
  }
}

VOID
EFIAPI
UsbPortPowerEnable (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "UsbPortPowerEnable called\n"));
  /* Set GPIO1 PA1 (USB_HOST_PWREN) output high to power USB ports */
  GpioPinWrite (1, GPIO_PIN_PA1, TRUE);
  GpioPinSetDirection (1, GPIO_PIN_PA1, GPIO_PIN_OUTPUT);
  /* Set GPIO2 PB6 (USB_TYPEC_PWREN) output high to power USB ports */
  GpioPinWrite (2, GPIO_PIN_PB6, TRUE);
  GpioPinSetDirection (2, GPIO_PIN_PB6, GPIO_PIN_OUTPUT);

  // DEBUG((DEBUG_INFO, "Trying to enable blue led\n"));
  // GpioPinWrite (0, GPIO_PIN_PB7, TRUE);
  // GpioPinSetDirection (0, GPIO_PIN_PB7, GPIO_PIN_OUTPUT);
}

VOID
EFIAPI
Usb2PhyResume (
  VOID
  )
{
  MmioWrite32 (0xfd5d0008, 0x20000000);
  MmioWrite32 (0xfd5d4008, 0x20000000);
  MmioWrite32 (0xfd5d8008, 0x20000000);
  MmioWrite32 (0xfd5dc008, 0x20000000);
  MmioWrite32 (0xfd7f0a10, 0x07000700);
  MmioWrite32 (0xfd7f0a10, 0x07000000);
}

VOID
EFIAPI
PcieIoInit (
  UINT32  Segment
  )
{
  /* Set reset and power IO to gpio output mode */
  switch (Segment) {
    case PCIE_SEGMENT_PCIE30X4:
      GpioPinSetDirection (4, GPIO_PIN_PB6, GPIO_PIN_OUTPUT);
      GpioPinSetDirection (1, GPIO_PIN_PA4, GPIO_PIN_OUTPUT);
      // PciePinmuxInit(Segment, 1);
      break;
    case PCIE_SEGMENT_PCIE30X2:
      GpioPinSetDirection (4, GPIO_PIN_PB0, GPIO_PIN_OUTPUT);
      GpioPinSetDirection (1, GPIO_PIN_PA4, GPIO_PIN_OUTPUT);
      // PciePinmuxInit(Segment, 1);
      break;
    case PCIE_SEGMENT_PCIE20L0:
      GpioPinSetDirection (4, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
      GpioPinSetDirection (1, GPIO_PIN_PD2, GPIO_PIN_OUTPUT);
      // PciePinmuxInit(Segment, 1); // PCIE30x1_0_{CLKREQN,WAKEN,PERSTN}_M1
      break;
    case PCIE_SEGMENT_PCIE20L1:
      break;
    case PCIE_SEGMENT_PCIE20L2:
      GpioPinSetDirection (3, GPIO_PIN_PB0, GPIO_PIN_OUTPUT);
      // PciePinmuxInit(Segment, 0); // PCIE20x1_2_{CLKREQN,WAKEN,PERSTN}_M0
      break;
    default:
      break;
  }
}

VOID
EFIAPI
PciePowerEn (
  UINT32   Segment,
  BOOLEAN  Enable
  )
{
  /* output high to enable power */

  switch (Segment) {
    case PCIE_SEGMENT_PCIE30X4:
    /* fall through */
    case PCIE_SEGMENT_PCIE30X2:
      GpioPinWrite (1, GPIO_PIN_PA4, Enable);
      break;
    case PCIE_SEGMENT_PCIE20L0:
      GpioPinWrite (1, GPIO_PIN_PD2, Enable);
      break;
    case PCIE_SEGMENT_PCIE20L1:
      break;
    case PCIE_SEGMENT_PCIE20L2:
      break;
    default:
      break;
  }
}

VOID
EFIAPI
PciePeReset (
  UINT32   Segment,
  BOOLEAN  Enable
  )
{
  switch (Segment) {
    case PCIE_SEGMENT_PCIE30X4:
      GpioPinWrite (4, GPIO_PIN_PB6, !Enable);
      break;
    case PCIE_SEGMENT_PCIE30X2:
      GpioPinWrite (4, GPIO_PIN_PB0, !Enable);
      break;
    case PCIE_SEGMENT_PCIE20L0: // m.2 a+e key
      GpioPinWrite (4, GPIO_PIN_PA5, !Enable);
      break;
    case PCIE_SEGMENT_PCIE20L1:
      break;
    case PCIE_SEGMENT_PCIE20L2: // rtl8125b
      GpioPinWrite (3, GPIO_PIN_PB0, !Enable);
      break;
    default:
      break;
  }
}

VOID
EFIAPI
HdmiTxIomux (
  IN UINT32  Id
  )
{
  switch (Id) {
    case 0:
      GpioPinSetFunction (4, GPIO_PIN_PC1, 5); // hdmim0_tx0_cec
      GpioPinSetPull (4, GPIO_PIN_PC1, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (1, GPIO_PIN_PA5, 5); // hdmim0_tx0_hpd
      GpioPinSetPull (1, GPIO_PIN_PA5, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (4, GPIO_PIN_PB7, 5); // hdmim0_tx0_scl
      GpioPinSetPull (4, GPIO_PIN_PB7, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (4, GPIO_PIN_PC0, 5); // hdmim0_tx0_sda
      GpioPinSetPull (4, GPIO_PIN_PC0, GPIO_PIN_PULL_NONE);
      break;
    case 1:
      GpioPinSetFunction (2, GPIO_PIN_PC4, 4); // hdmim0_tx1_cec
      GpioPinSetPull (2, GPIO_PIN_PC4, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (1, GPIO_PIN_PA6, 5); // hdmim0_tx1_hpd
      GpioPinSetPull (1, GPIO_PIN_PA6, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (3, GPIO_PIN_PC6, 5); // hdmim1_tx1_scl
      GpioPinSetPull (3, GPIO_PIN_PC6, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (3, GPIO_PIN_PC5, 5); // hdmim1_tx1_sda
      GpioPinSetPull (3, GPIO_PIN_PC5, GPIO_PIN_PULL_NONE);
      break;
  }
}

PWM_DATA  pwm_data = {
  .ControllerID = PWM_CONTROLLER0,
  .ChannelID    = PWM_CHANNEL1,
  .PeriodNs     = 4000000,
  .DutyNs       = 4000000,
  .Polarity     = FALSE,
}; // PWM0_CH1

VOID
EFIAPI
PwmFanIoSetup (
  VOID
  )
{
  GpioPinSetFunction (0, GPIO_PIN_PC0, 0x3); // PWM1_M0
  RkPwmSetConfig (&pwm_data);
  RkPwmEnable (&pwm_data);
}

VOID
EFIAPI
PwmFanSetSpeed (
  IN UINT32  Percentage
  )
{
  pwm_data.DutyNs = pwm_data.PeriodNs * Percentage / 100;
  RkPwmSetConfig (&pwm_data);
}

VOID
EFIAPI
PlatformInitLeds (
  VOID
  )
{
  /* Status indicator */
  GpioPinWrite (0, GPIO_PIN_PB7, FALSE);
  GpioPinSetDirection (0, GPIO_PIN_PB7, GPIO_PIN_OUTPUT);
}

VOID
EFIAPI
PlatformSetStatusLed (
  IN BOOLEAN  Enable
  )
{
  GpioPinWrite (0, GPIO_PIN_PB7, Enable);
}

VOID
EFIAPI
PlatformPcieWiFiEnable (
  IN BOOLEAN  Enable
  )
{
  // WiFi - enable
  GpioPinWrite (0, GPIO_PIN_PC4, Enable);
  GpioPinSetDirection (0, GPIO_PIN_PC4, GPIO_PIN_OUTPUT);
  GpioPinWrite (4, GPIO_PIN_PA2, Enable);
  GpioPinSetDirection (4, GPIO_PIN_PA2, GPIO_PIN_OUTPUT);

  // bluetooth - enable
  GpioPinWrite (3, GPIO_PIN_PD5, Enable);
  GpioPinSetDirection (3, GPIO_PIN_PD5, GPIO_PIN_OUTPUT);
}

CONST EFI_GUID *
EFIAPI
PlatformGetDtbFileGuid (
  IN UINT32  CompatMode
  )
{
  STATIC CONST EFI_GUID  VendorDtbFileGuid = {
    // DeviceTree/Vendor.inf
    0xd58b4028, 0x43d8, 0x4e97, { 0x87, 0xd4, 0x4e, 0x37, 0x16, 0x13, 0x65, 0x80 }
  };
  STATIC CONST EFI_GUID  MainlineDtbFileGuid = {
    // DeviceTree/Mainline.inf
    0x117855c9, 0xfa71, 0x43d8, { 0x89, 0x37, 0x37, 0xd8, 0x1b, 0x19, 0xcd, 0x02 }
  };

  switch (CompatMode) {
    case FDT_COMPAT_MODE_VENDOR:
      return &VendorDtbFileGuid;
    case FDT_COMPAT_MODE_MAINLINE:
      return &MainlineDtbFileGuid;
  }

  return NULL;
}

VOID
EFIAPI
PlatformEarlyInit (
  VOID
  )
{
  // Configure various things specific to this platform
  PlatformPcieWiFiEnable (TRUE);
  GpioPinSetFunction (1, GPIO_PIN_PD5, 0); // jdet
}
