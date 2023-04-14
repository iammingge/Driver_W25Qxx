/**
 * Copyright (c) 2022 iammingge
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file    W25Qxx.h
 * @brief   W25Qxx family driver source file
 * @author  iammingge
 *
 *      DATE             NAME                      DESCRIPTION
 *
 *   11-27-2022        iammingge                Initial Version 1.0
 *   04-14-2023        iammingge      			1. modify the function W25Qxx_ReadStatus "2^ret" -> "1<<ret"
 *											    2. align code 4 bytes
 *											    3. support 4 address mode
 *
**/

#define W25QXX_GLOBALS
#include "W25Qxx.h"

/* ----------------------------------------------------------------------------------------------------------------------
   |                                                     NOR FLASH                                                      |
   ----------------------------------------------------------------------------------------------------------------------
   |  Model  | Block |   Sector  |      Page      |             Byte             |            Bit           | Addr Mode |
   | W25Q02  | 4096  | 4096 * 16 | 4096 * 16 * 16 | 4096 * 16 * 16 * 256 (256MB) | 4096 * 16 * 16 * 256 * 8 |    3/4    |
   | W25Q01  | 2048  | 2048 * 16 | 2048 * 16 * 16 | 2048 * 16 * 16 * 256 (128MB) | 2048 * 16 * 16 * 256 * 8 |    3/4    |
   | W25Q512 | 1024  | 1024 * 16 | 1024 * 16 * 16 | 1024 * 16 * 16 * 256  (64MB) | 1024 * 16 * 16 * 256 * 8 |    3/4    |
   | W25Q256 |  512  |  512 * 16 |  512 * 16 * 16 |  512 * 16 * 16 * 256  (32MB) |  512 * 16 * 16 * 256 * 8 |    3/4    |
   | W25Q128 |  256  |  256 * 16 |  256 * 16 * 16 |  256 * 16 * 16 * 256  (16MB) |  256 * 16 * 16 * 256 * 8 |     3     |
   | W25Q64  |  128  |  128 * 16 |  128 * 16 * 16 |  128 * 16 * 16 * 256   (8MB) |  128 * 16 * 16 * 256 * 8 |     3     |
   | W25Q32  |   64  |   64 * 16 |   64 * 16 * 16 |   64 * 16 * 16 * 256   (4MB) |   64 * 16 * 16 * 256 * 8 |     3     |
   | W25Q16  |   32  |   32 * 16 |   32 * 16 * 16 |   32 * 16 * 16 * 256   (2MB) |   32 * 16 * 16 * 256 * 8 |     3     |
   | W25Q80  |   16  |   16 * 16 |   16 * 16 * 16 |   16 * 16 * 16 * 256   (1MB) |   16 * 16 * 16 * 256 * 8 |     3     |
   | W25Q40  |    8  |    8 * 16 |    8 * 16 * 16 |    8 * 16 * 16 * 256 (512KB) |    8 * 16 * 16 * 256 * 8 |     3     |
   | W25Q20  |    4  |    4 * 16 |    4 * 16 * 16 |    4 * 16 * 16 * 256 (256KB) |    4 * 16 * 16 * 256 * 8 |     3     |
   | W25X10  |    2  |    2 * 16 |    2 * 16 * 16 |    2 * 16 * 16 * 256 (128KB) |    2 * 16 * 16 * 256 * 8 |     3     |
   | W25X05  |    1  |    1 * 16 |    1 * 16 * 16 |    1 * 16 * 16 * 256  (64KB) |    1 * 16 * 16 * 256 * 8 |     3     |
   ----------------------------------------------------------------------------------------------------------------------
**/
/* Tool Function */
#define rbit(val, x)     (((val) & (1<<(x)))>>(x))							/* Read  1 bit */
#define wbit(val, x, a)  (val = ((val) & ~(1<<(x))) | ((a)<<(x)))			/* Write 1 bit */
#define noruint(val)     (val = !!(val))									/* Normalize data while keeping the logical value unchanged */
static uint8_t BcdToByte(uint16_t num)										/* Calculate BCD(0 - 597) convert 1Byte(0 - 255) example : 20(0x14) ---> 0x20 */
{
	uint8_t d1, d2, d3;

	if (num > 597) return 0x00;

	d1 = (uint16_t)num & 0x000F;
	d2 = ((uint16_t)num & 0x00F0) >> 4;
	d3 = ((uint16_t)num & 0x0F00) >> 8;
	d1 = d1 + d2 * 10 + d3 * 100;

	return d1;
}
/* W25Qxx Cache */
static uint8_t W25QXX_CACHE[W25Qxx_SECTORSIZE];
/* W25Qxx Info List */
static W25Qxx_INFO_t W25QInfoList[] = {
	/* Type    | ProgramPage | EraseSector | EraseBlock64 | EraseBlock32 | EraseChip */
	{  W25X05,   1,            300,          1000,          800,           1000       }, //W25X05CL     0.8
	{  W25X10,   1,            300,          1000,          800,           1000       }, //W25X10CL     0.8
	{  W25Q20,   1,            300,          1000,          800,           2000       }, //W25Q20CL     0.8
	{  W25Q40,   1,            300,          1000,          800,           4000       }, //W25Q40CL     0.8
	{  W25Q80,   4,            500,          2000,          1500,          8000       }, //W25Q80DV     4
	{  W25Q16,   3,            400,          2000,          1600,          25000      }, //W25Q16JV     3
	{  W25Q32,   3,            400,          2000,          1600,          50000      }, //W25Q32JV     3
	{  W25Q64,   3,            400,          2000,          1600,          100000     }, //W25Q64JV     3
	{  W25Q128,  3,            400,          2000,          1600,          200000     }, //W25Q128JV    3
	{  W25Q256,  3,            400,          2000,          1600,          400000     }, //W25Q256JV    3
	{  W25Q512,  4,            400,          2000,          1600,          1000000    }, //W25Q512JV    3.5
	{  W25Q01,   4,            400,          2000,          1600,          1000000    }, //W25Q01JV_DTR 3.5
	{  W25Q02,   4,            400,          2000,          1600,          1000000    }, //W25Q02JV_DTR 3.5 
};
/*---------------------------------------------------------------------------------------------------------------------*/
/*                            The following functions does not include error check judgment  						   */
/*---------------------------------------------------------------------------------------------------------------------*/
/* W25Qxx Read Manufacturer/JEDEC/Unique ID */
uint16_t W25Qxx_ID_Manufacturer(void)																						/* Read Manufacturer ID */
{
	uint8_t i = 1;
	uint16_t ID = 0;
	uint16_t IDByte = 0;

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set power enable */
	W25Qxx.port.spi_rw(W25Q_CMD_MANUFACTURER);

	/* read ID */
	W25Qxx.port.spi_rw(W25Q_DUMMY);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
	W25Qxx.port.spi_rw(0x00);
	do
	{
		IDByte = W25Qxx.port.spi_rw(W25Q_DUMMY);
		ID |= IDByte << (i * 8);
	} while (i--);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tRES2 max = 1.8us */
	W25Qxx.port.spi_delayms(1);

	return ID;
}
uint32_t W25Qxx_ID_JEDEC(void)																					 			/* Read JEDEC  ID */
{
	uint8_t  i = 2;
	uint32_t ID = 0;
	uint32_t IDByte = 0;

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read JEDEC ID */
	W25Qxx.port.spi_rw(W25Q_CMD_JEDECID);
	do
	{
		IDByte = W25Qxx.port.spi_rw(W25Q_DUMMY);
		ID |= IDByte << (i * 8);
	} while (i--);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	return ID;
}
uint64_t W25Qxx_ID_Unique(void)																								/* Read Unique ID */
{
	uint8_t  i = 7;
	uint64_t ID = 0;
	uint64_t IDByte = 0;

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read Unique ID */
	W25Qxx.port.spi_rw(W25Q_CMD_UNIQUEID);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_DUMMY);
#endif
	do
	{
		IDByte = W25Qxx.port.spi_rw(W25Q_DUMMY);
		ID |= IDByte << (i * 8);
	} while (i--);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	return ID;
}
/* W25Qxx Individual Control Instruction */
void W25Qxx_Reset(void)																									    /* Software reset */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* reset device */
	W25Qxx.port.spi_rw(W25Q_CMD_ENRESET);
	W25Qxx.port.spi_rw(W25Q_CMD_RESETDEV);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tRST (30us) */
	W25Qxx.port.spi_delayms(1);
}
void W25Qxx_PowerEnable(void)   																							/* Power Enable */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set power enable */
	W25Qxx.port.spi_rw(W25Q_CMD_POWEREN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tRES1 max = 3us */
	W25Qxx.port.spi_delayms(1);
}
void W25Qxx_PowerDisable(void)  																							/* Power Disable */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set power disable */
	W25Qxx.port.spi_rw(W25Q_CMD_POWERDEN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tDP max = 3us */
	W25Qxx.port.spi_delayms(1);
}
void W25Qxx_VolatileSR_WriteEnable(void)																					/* Write Enable for Volatile Status Register */
{
	/* This gives more flexibility to change the system configuration and memory protection schemes quickly without
	 * waiting for the typical non-volatile bit write cycles or affecting the endurance of the Status Register non-volatile bits
	**/

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* Determine if volatile SR is enable */
	W25Qxx.port.spi_rw(W25Q_CMD_VOLATILESREN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();
}
void W25Qxx_WriteEnable(void)   																							/* Write Enable */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set write enable */
	W25Qxx.port.spi_rw(W25Q_CMD_WEN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* read back WEL Bit */
	W25Qxx_ReadStatusRegister(1);
}
void W25Qxx_WriteDisable(void)   																						    /* Write Disable */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set write disable */
	W25Qxx.port.spi_rw(W25Q_CMD_WDEN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* read back WEL Bit */
	W25Qxx_ReadStatusRegister(1);
}
void W25Qxx_4ByteMode(void)																									/* Set 4 bytes address mode */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set 4 byte address mode */
	W25Qxx.port.spi_rw(W25Q_CMD_4ByteAddrEN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* read back ADS Bit */
	W25Qxx_ReadStatusRegister(3);
}
void W25Qxx_3ByteMode(void)																									/* Set 3 bytes address mode */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set 3 byte address mode */
	W25Qxx.port.spi_rw(W25Q_CMD_4ByteAddrDEN);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* read back ADS Bit */
	W25Qxx_ReadStatusRegister(3);
	
	/* set the extended address register to 0x00 */
	W25Qxx_WriteExtendedRegister(0x00);
}
void W25Qxx_Suspend(void)																							 		/* Erase/Program suspend (SUS = 0 & BUSY = 1) */
{
	/* Erase Suspend instruction use scope       : 1. Erase operation (20h, 52h, D8h, 44h)             (¡Ì)
	 * 											   2. Erase operation (C7h, 60h)                       (x)
	 * Commands Supported During Erase Suspend   : 1. Write Status Register instruction (01h)          (x)
	 *                                             2. Erase instruction (20h, 52h, D8h, C7h, 60h, 44h) (x)
	 *                                             3. Read instruction (03h, 0Bh, 5Ah, 48h)            (¡Ì)
	 * Program Suspend instruction use scope     : 1. Page Program operation (02h, 42h)                (¡Ì)
	 *                                             2. Quad Page Program operation (32h)                (¡Ì)
	 * Commands Supported During Program Suspend : 1. Write Status Register instruction (01h)          (x)
	 *                                             2. Program instructions (02h, 32h, 42h)             (x)
	 *                                             3. Read instruction (03h, 0Bh, 5Ah, 48h)            (¡Ì)
<<<<<<< HEAD
	**/

	/* CS enable */
=======
	 **/

	 /* CS enable */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	W25Qxx.port.spi_cs_L();

	/* erase data */
	W25Qxx.port.spi_rw(W25Q_CMD_EWSUSPEND);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tSUS max = 20us */
	W25Qxx.port.spi_delayms(1);

	/* read back Busy Bit */
	W25Qxx_ReadStatusRegister(1);

	/* read back Suspend Bit */
	W25Qxx_ReadStatusRegister(2);
}
void W25Qxx_Resume(void)																							 	 	/* Erase/Program resume  (SUS = 1) */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
	W25Qxx.port.spi_rw(W25Q_CMD_EWRESUME);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* tSUS max = 20us */
	W25Qxx.port.spi_delayms(1);

	/* read back Busy Bit */
	W25Qxx_ReadStatusRegister(1);

	/* read back Suspend Bit */
	W25Qxx_ReadStatusRegister(2);
}
/* W25Qxx Read sector/block lock of current address status */
uint8_t W25Qxx_ReadLock(uint32_t ByteAddr)												   		 					        /* Read current Sector/Block Lock Status */
{
	uint8_t ret = 0;

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (ByteAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((ByteAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read block lock status */
	W25Qxx.port.spi_rw(W25Q_CMD_RBLOCKLOCK);

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
#endif
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	ret = W25Qxx.port.spi_rw(W25Q_DUMMY);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	return ret;
}
/* W25Qxx Read/Write ExtendedRegister 
 * 1. The Extended Address Register is only effective when the device is in the 3-Byte Address Mode.
 * 2. When the device operates in the 4-Byte Address Mode (ADS=1), any command with address input of 
 *    A31-A24 will replace the Extended Address Register values. 
 * 3. It is recommended to check and update the Extended Address Register if necessary when the device 
 *    is switched from 4-Byte to 3-Byte Address Mode.
 * 4. Upon power up or the execution of a Software/Hardware Reset, the Extended Address Register bit 
 *    values will be cleared to 0.
**/
void W25Qxx_ReadExtendedRegister(void)																						/* Read  Extended Address Register */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write extended address register */
	W25Qxx.port.spi_rw(W25Q_CMD_REXTREG);
	W25Qxx.ExtendedRegister = W25Qxx.port.spi_rw(W25Q_DUMMY);

	/* CS disable */
	W25Qxx.port.spi_cs_H();
}
void W25Qxx_WriteExtendedRegister(uint8_t ExtendedAddr)																		/* Write Extended Address Register */
{
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write extended address register */
	W25Qxx.ExtendedRegister = ExtendedAddr;
	W25Qxx.port.spi_rw(W25Q_CMD_WEXTREG);
	W25Qxx.port.spi_rw(ExtendedAddr);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();
}
/* W25Qxx Read/Write StatusRegister */
void W25Qxx_ReadStatusRegister(uint8_t Select_SR_1_2_3)																		/* Read  Status Register1/2/3 */
{
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read status register */
	switch (Select_SR_1_2_3)
	{
		case 1: W25Qxx.port.spi_rw(W25Q_CMD_RSREG1); W25Qxx.StatusRegister1 = W25Qxx.port.spi_rw(W25Q_DUMMY); break;
		case 2: W25Qxx.port.spi_rw(W25Q_CMD_RSREG2); W25Qxx.StatusRegister2 = W25Qxx.port.spi_rw(W25Q_DUMMY); break;
		case 3: W25Qxx.port.spi_rw(W25Q_CMD_RSREG3); W25Qxx.StatusRegister3 = W25Qxx.port.spi_rw(W25Q_DUMMY); break;
		default: break;
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();
}
void W25Qxx_WriteStatusRegister(uint8_t Select_SR_1_2_3, uint8_t Data)									 					/* Write Status Register1/2/3 */
{
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write status register */
	switch (Select_SR_1_2_3)
	{
		case 1: W25Qxx.StatusRegister1 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG1);  W25Qxx.port.spi_rw(Data); break;
		case 2: W25Qxx.StatusRegister2 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG2);  W25Qxx.port.spi_rw(Data); break;
		case 3: W25Qxx.StatusRegister3 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG3);  W25Qxx.port.spi_rw(Data); break;
		default: break;
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* tW max = 15ms */
	W25Qxx.port.spi_delayms(15);
}
void W25Qxx_VolatileSR_WriteStatusRegister(uint8_t Select_SR_1_2_3, uint8_t Data)											/* Volatile write Status Register1/2/3 */
{
	/* This gives more flexibility to change the system configuration and memory protection schemes quickly without
	 * waiting for the typical non-volatile bit write cycles or affecting the endurance of the Status Register non-volatile bits
	**/

	/* volatile write enable */
	W25Qxx_VolatileSR_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write status register */
	switch (Select_SR_1_2_3)
	{
		case 1: W25Qxx.StatusRegister1 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG1);  W25Qxx.port.spi_rw(Data); break;
		case 2: W25Qxx.StatusRegister2 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG2);  W25Qxx.port.spi_rw(Data); break;
		case 3: W25Qxx.StatusRegister3 = Data; W25Qxx.port.spi_rw(W25Q_CMD_WSREG3);  W25Qxx.port.spi_rw(Data); break;
		default: break;
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();
}
uint8_t W25Qxx_RBit_WEL(void)																								/* Read WEL  Bit for StatusRegister1 */
{
	uint8_t ret = 0;

	/* read StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);

	/* read WEL bit */
	ret = rbit(W25Qxx.StatusRegister1, 1);

	return ret;
}
uint8_t W25Qxx_RBit_BUSY(void)																							    /* Read BUSY Bit for StatusRegister1 */
{
	uint8_t ret = 0;

	/* read StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);

	/* read BUSY bit */
	ret = rbit(W25Qxx.StatusRegister1, 0);

	return ret;
}
uint8_t W25Qxx_RBit_SUS(void)																								/* Read SUS  Bit for StatusRegister2 */
{
	uint8_t ret = 0;

	/* read StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);

	/* read SUS bit */
	ret = rbit(W25Qxx.StatusRegister2, 7);

	return ret;
}
uint8_t W25Qxx_RBit_ADS(void)																								/* Read ADS  Bit for StatusRegister3 */
{
	uint8_t ret = 0;

	/* read StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);

	/* read ADS bit */
	ret = rbit(W25Qxx.StatusRegister3, 0);

	return ret;
}
void W25Qxx_WBit_SRP(W25Qxx_SRM srm, uint8_t bit)																			/* Write SRP Bit for StatusRegister1 */
{
	/* read StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);

	/* write SRP bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister1, 7, bit);

	/* write StatusRegister1 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}
	else
	{
		W25Qxx_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}

	/* read back StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);
}
void W25Qxx_WBit_TB(W25Qxx_SRM srm, uint8_t bit) 																			/* Write TB  Bit for StatusRegister1 */
{
	/* read StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);

	/* write TB bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister1, 6, bit);

	/* write StatusRegister1 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}
	else
	{
		W25Qxx_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}

	/* read back StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);
}
void W25Qxx_WBit_CMP(W25Qxx_SRM srm, uint8_t bit)																			/* Write CMP Bit for StatusRegister2 */
{
	/* read StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);

	/* write CMP bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister2, 6, bit);

	/* write StatusRegister2 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}
	else
	{
		W25Qxx_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}

	/* read back StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);
}
void W25Qxx_WBit_QE(W25Qxx_SRM srm, uint8_t bit) 																			/* Write QE  Bit for StatusRegister2 */
{
	/* read StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);

	/* write QE bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister2, 1, bit);

	/* write StatusRegister2 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}
	else
	{
		W25Qxx_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}

	/* read back StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);
}
void W25Qxx_WBit_SRL(W25Qxx_SRM srm, uint8_t bit)																			/* Write SRL Bit for StatusRegister2 */
{
	/* read StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);

	/* write CMP bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister2, 0, bit);

	/* write StatusRegister2 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}
	else
	{
		W25Qxx_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}

	/* read back StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);
}
void W25Qxx_WBit_WPS(W25Qxx_SRM srm, uint8_t bit)																			/* Write WPS Bit for StatusRegister3 */
{
	/* read StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);

	/* write WPS bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister3, 2, bit);

	/* write StatusRegister3 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(3, W25Qxx.StatusRegister3);
	}
	else
	{
		W25Qxx_WriteStatusRegister(3, W25Qxx.StatusRegister3);
	}

	/* read back StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);
}
void W25Qxx_WBit_DRV(W25Qxx_SRM srm, uint8_t bit)																			/* Write DRV Bit for StatusRegister3 */
{
	/* read StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);

	/* write DRV bit (0110 0000 -> 0x60) */
	bit &= 0x03;
	W25Qxx.StatusRegister3 &= ~0x60;
	W25Qxx.StatusRegister3 |= (bit << 5);

	/* write StatusRegister3 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(3, W25Qxx.StatusRegister3);
	}
	else
	{
		W25Qxx_WriteStatusRegister(3, W25Qxx.StatusRegister3);
	}

	/* read back StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);
}
void W25Qxx_WBit_BP(W25Qxx_SRM srm, uint8_t bit) 																			/* Write BP  Bit for StatusRegister1 */
{
	/* read StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);

	/* write BP bit (0011 1100 -> 0x3C) */
	W25Qxx.StatusRegister1 &= ~0x3C;
	W25Qxx.StatusRegister1 |= (bit << 2);

	/* write StatusRegister1 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}
	else
	{
		W25Qxx_WriteStatusRegister(1, W25Qxx.StatusRegister1);
	}

	/* read back StatusRegister1 */
	W25Qxx_ReadStatusRegister(1);
}
void W25Qxx_WBit_LB(W25Qxx_SRM srm, uint8_t bit) 																			/* Write LB  Bit for StatusRegister2 */
{
	/* read StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);

	/* write LB bit (0011 1000 -> 0x38)*/
	bit &= 0x07;
	W25Qxx.StatusRegister2 &= ~0x38;
	W25Qxx.StatusRegister2 |= (bit << 3);

	/* write StatusRegister2 */
	if (srm == W25Qxx_VOLATILE)
	{
		W25Qxx_VolatileSR_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}
	else
	{
		W25Qxx_WriteStatusRegister(2, W25Qxx.StatusRegister2);
	}

	/* read back StatusRegister2 */
	W25Qxx_ReadStatusRegister(2);
}
void W25Qxx_WBit_ADP(uint8_t bit)                																			/* Write ADP Bit for StatusRegister3 */
{
	/* read StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);

	/* write ADP bit */
	noruint(bit);
	wbit(W25Qxx.StatusRegister3, 1, bit);

	/* write StatusRegister3 */
	W25Qxx_WriteStatusRegister(3, W25Qxx.StatusRegister3);

	/* read back StatusRegister3 */
	W25Qxx_ReadStatusRegister(3);
}
uint8_t W25Qxx_ReadStatus(void)																								/* Read current chip running status */
{
	uint8_t ret = 0;

	ret |= W25Qxx_RBit_BUSY();
	ret |= W25Qxx_RBit_SUS() << 1;

	return (1 << ret);
}
/*---------------------------------------------------------------------------------------------------------------------*/
/*                                The following functions include error check judgment      					       */
/*---------------------------------------------------------------------------------------------------------------------*/
/* W25Qxx Determine Status */
void W25Qxx_isStatus(uint8_t Select_Status, uint32_t timeout, W25Qxx_ERR *err)												/* Determine current running status */
{
	uint32_t time = timeout;
	W25Qxx_STATUS curstatus = W25Qxx_STATUS_IDLE;

	do
	{
		/* Read current chip running status */
		curstatus = (W25Qxx_STATUS)W25Qxx_ReadStatus();

		if (curstatus & Select_Status)
		{
			/* current status correct */
			*err = W25Qxx_ERR_NONE;
			return;
		}

		if (time-- == 0) break;

		W25Qxx.port.spi_delayms(1);

	} while (time);

	/* current status err */
	*err = W25Qxx_ERR_STATUS;
}
/* W25Qxx Sector/Blcok Lock protect for " WPS = 1 "
 * WPS = 0 : The Device will only utilize CMP, TB, BP[3:0] bits to protect specific areas of the array.
 * WPS = 1 : The Device will utilize the Individual Block Locks for write protection.
 * Example : W25Q256FVEIQ
 * 		     Sector(Block 0)   : 0-15
 *           Block 1-510
 * 		     Sector(Block 511) : 0-15
 *           W25Q128JVSIQ
 * 		     Sector(Block 0)   : 0-15
 *           Block 1-254
 * 		     Sector(Block 255) : 0-15
**/
void W25Qxx_Global_UnLock(W25Qxx_ERR *err)																					/* Global Sector/Block Unlock */
{
	/* Determine if WPS bit Mode is 1 */
	if (rbit(W25Qxx.StatusRegister3, 2) == 0x00)
	{
		/* WPS = 0 */
		*err = W25Qxx_ERR_WPSMODE;
		return;
	}

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set all block unlocked */
	W25Qxx.port.spi_rw(W25Q_CMD_WALLBLOCKUNLOCK);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Global_Locked(W25Qxx_ERR *err)																					/* Global Sector/Block Locked */
{
	/* Determine if WPS bit Mode is 1 */
	if (rbit(W25Qxx.StatusRegister3, 2) == 0x00)
	{
		/* WPS = 0 */
		*err = W25Qxx_ERR_WPSMODE;
		return;
	}

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* set all block locked */
	W25Qxx.port.spi_rw(W25Q_CMD_WALLBLOCKLOCK);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Individual_UnLock(uint32_t ByteAddr, W25Qxx_ERR *err)															/* Individual Sector/Block Unlock */
{
	/* Determine if WPS bit Mode is 1 */
	if (rbit(W25Qxx.StatusRegister3, 2) == 0x00)
	{
		/* WPS = 0 */
		*err = W25Qxx_ERR_WPSMODE;
		return;
	}

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (ByteAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((ByteAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read block lock status */
	W25Qxx.port.spi_rw(W25Q_CMD_WSIGBLOCKUNLOCK);

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
#endif
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Individual_Locked(uint32_t ByteAddr, W25Qxx_ERR *err)															/* Individual Sector/Block Locked */
{
	/* Determine if WPS bit Mode is 1 */
	if (rbit(W25Qxx.StatusRegister3, 2) == 0x00)
	{
		/* WPS = 0 */
		*err = W25Qxx_ERR_WPSMODE;
		return;
	}

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (ByteAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((ByteAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* read block lock status */
	W25Qxx.port.spi_rw(W25Q_CMD_WSIGBLOCKLOCK);

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
#endif
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	*err = W25Qxx_ERR_NONE;
}
/* Main Storage Read/Erase/Program
 *
 * Security    Storage Read/Erase/Program
 *                            ( ByteAddr )     ( SectorAddr )
 * Security #1 Address : 0x00001000 - 0x000010FF ---> 1
 * Security #2 Address : 0x00002000 - 0x000020FF ---> 2
 * Security #3 Address : 0x00003000 - 0x000030FF ---> 3
 *
 * SFDP Storage Read
 * SFDP Address : 0x00000000 - 0x000000FF
**/
void W25Qxx_Erase_Chip(W25Qxx_ERR *err)                                                              						/* Erase all chip */
{
	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
	W25Qxx.port.spi_rw(W25Q_CMD_ECHIP);

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE, W25Qxx.info.EraseMaxTimeChip, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Erase_Block64(uint32_t Block64Addr, W25Qxx_ERR *err)                                   							/* Erase block of 64k */
{
	/* Determine if Block 64 Addrress Bound */
	if (Block64Addr >= W25Qxx.numBlock)
	{
		*err = W25Qxx_ERR_BLOCK64ADDRBOUND;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* calculate sector address */
	Block64Addr *= W25Qxx.sizeBlock;

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (Block64Addr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((Block64Addr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_E64KBLOCK);
	W25Qxx.port.spi_rw((uint8_t)((Block64Addr) >> 24));
	W25Qxx.port.spi_rw((uint8_t)((Block64Addr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((Block64Addr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)Block64Addr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_E64KBLOCK);
	W25Qxx.port.spi_rw((uint8_t)((Block64Addr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((Block64Addr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)Block64Addr);
#endif
	
	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.EraseMaxTimeBlock64, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Erase_Block32(uint32_t Block32Addr, W25Qxx_ERR *err)                                   							/* Erase block of 32k */
{
	/* Determine if Block 32 Addrress Bound */
	if (Block32Addr >= W25Qxx.numBlock * 2)
	{
		*err = W25Qxx_ERR_BLOCK32ADDRBOUND;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* calculate sector address */
	Block32Addr *= (W25Qxx.sizeBlock >> 1);		/* Block32Addr *= (W25Qxx.sizeBlock / 2); */

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (Block32Addr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((Block32Addr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_E32KBLOCK);
	W25Qxx.port.spi_rw((uint8_t)((Block32Addr) >> 24));
	W25Qxx.port.spi_rw((uint8_t)((Block32Addr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((Block32Addr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)Block32Addr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_E32KBLOCK);
	W25Qxx.port.spi_rw((uint8_t)((Block32Addr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((Block32Addr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)Block32Addr);
#endif
	
	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.EraseMaxTimeBlock32, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Erase_Sector(uint32_t SectorAddr, W25Qxx_ERR *err)                                   							/* Erase sector of 4k (Notes : 150ms) */
{
	/* Determine if Sector Addrress Bound */
	if (SectorAddr >= W25Qxx.numSector)
	{
		*err = W25Qxx_ERR_SECTORADDRBOUND;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* calculate sector address */
	SectorAddr *= W25Qxx.sizeSector;

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (SectorAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((SectorAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_ESECTOR);
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 24));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)SectorAddr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_ESECTOR);
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)SectorAddr);
#endif
	
	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.EraseMaxTimeSector, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Erase_Security(uint32_t SectorAddr, W25Qxx_ERR *err)                                               				/* Erase security page of 256Byte (Notes : 150ms) */
{
	/* Determine if Sector Addrress Bound */
	if (SectorAddr > 3 || SectorAddr == 0)
	{
		*err = W25Qxx_ERR_PAGEADDRBOUND;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_ESECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)(SectorAddr << 4));
	W25Qxx.port.spi_rw(0x00);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_ESECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)(SectorAddr << 4));
	W25Qxx.port.spi_rw(0x00);
#endif

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.EraseMaxTimeSector, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Read(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead, W25Qxx_ERR *err)   							/* Read */
{
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToRead == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Determine if the address > W25Qxx.sizeChip */
	if (ByteAddr + NumByteToRead > W25Qxx.sizeChip)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (ByteAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((ByteAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write address */
#if W25QXX_FASTREAD
	#if W25QXX_4BADDR
		W25Qxx.port.spi_rw(W25Q_CMD_FASTREAD);
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
		W25Qxx.port.spi_rw((uint8_t)ByteAddr);
		W25Qxx.port.spi_rw(W25Q_DUMMY);
	#else
		W25Qxx.port.spi_rw(W25Q_CMD_FASTREAD);
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
		W25Qxx.port.spi_rw((uint8_t)ByteAddr);
		W25Qxx.port.spi_rw(W25Q_DUMMY);
	#endif
#else
	#if W25QXX_4BADDR
		W25Qxx.port.spi_rw(W25Q_CMD_READ);
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
		W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	#else
		W25Qxx.port.spi_rw(W25Q_CMD_READ);
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
		W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
		W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	#endif
#endif

	/* read data */
	for (i = 0; i < NumByteToRead; i++)
	{
		pBuffer[i] = W25Qxx.port.spi_rw(W25Q_DUMMY);
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Read_Security(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead, W25Qxx_ERR *err)              		/* Read security */
{
	uint32_t numPage = 0;
	uint32_t startAddr = 0;
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToRead == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Determine if the address > startAddr + W25Qxx_PAGESIZE */
	numPage = ByteAddr >> W25Qxx_SECTORPOWER;
	startAddr = numPage * 0x00001000;
	if (numPage > 3 || numPage == 0 || ByteAddr + NumByteToRead > startAddr + W25Qxx_PAGESIZE)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_RSECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_RSECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	W25Qxx.port.spi_rw(W25Q_DUMMY);
#endif
	
	/* read data */
	for (i = 0; i < NumByteToRead; i++)
	{
		pBuffer[i] = W25Qxx.port.spi_rw(W25Q_DUMMY);
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Read_SFDP(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead, W25Qxx_ERR *err)      					/* Read SFDP */
{
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToRead == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Determine if address is correct */
	if (ByteAddr + NumByteToRead > W25Qxx_PAGESIZE)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write address */
	W25Qxx.port.spi_rw(W25Q_CMD_RSFDP);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
	W25Qxx.port.spi_rw(0x00);
	
	/* read data */
	for (i = 0; i < NumByteToRead; i++)
	{
		pBuffer[i] = W25Qxx.port.spi_rw(W25Q_DUMMY);
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_DIR_Program_Page(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err)  				/* No check Direct program Page   (0-256), Notes : no beyond page address */
{
	/* No check Direct Page write
<<<<<<< HEAD
	 * 1. Write data of the specified length at the specified address,
	 *    but ensure that the data is in the same page.
	 * 2. You must ensure that all data within the written address range is 0xFF,
	 *    otherwise the data written at a location other than 0xFF will fail.
=======
	   * 1. Write data of the specified length at the specified address,
	   *    but ensure that the data is in the same page.
	   * 2. You must ensure that all data within the written address range is 0xFF,
	   *    otherwise the data written at a location other than 0xFF will fail.
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	**/
	uint16_t i = 0;
	uint16_t remPage = 0;

	/* Determine if the number is 0 */
	if (NumByteToWrite == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Determine if the address > remainPage
<<<<<<< HEAD
	 * (Notes : remainPage maxsize = 256) */
=======
		 * (Notes : remainPage maxsize = 256) */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	remPage = W25Qxx_PAGESIZE - (ByteAddr & (W25Qxx_PAGESIZE - 1));		/* remPage = W25Qxx_PAGESIZE - ByteAddr % W25Qxx_PAGESIZE; */
	if (NumByteToWrite > remPage)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (ByteAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((ByteAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_WPAGE);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 24));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_WPAGE);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
#endif
	
	/* write data */
	for (i = 0; i < NumByteToWrite; i++)
	{
		W25Qxx.port.spi_rw(pBuffer[i]);
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.ProgrMaxTimePage, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_DIR_Program(uint8_t *pBuffer, uint32_t ByteAddr, uint32_t NumByteToWrite, W25Qxx_ERR *err)				 		/* No check Direct program */
{
	/* No check Direct write
	 * 1. With automatic page change function.
	 * 2. You must ensure that all data within the written address range is 0xFF,
	 *    otherwise the data written at a location other than 0xFF will fail.
	**/
	uint16_t remPage = 0;

	/* Determine if the number is 0 */
	if (NumByteToWrite == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* First page remain bytes */
	remPage = W25Qxx_PAGESIZE - (ByteAddr & (W25Qxx_PAGESIZE - 1));				/* remPage = W25Qxx_PAGESIZE - ByteAddr % W25Qxx_PAGESIZE; */
	if (NumByteToWrite <= remPage) remPage = NumByteToWrite;

	while (1)
	{
		/*---------------------------------- Check Data Area ------------------------------------*/

		// No Check Data

		/*------------------------------------ Write data ---------------------------------------*/

		W25Qxx_DIR_Program_Page(pBuffer, ByteAddr, remPage, err);
		if (*err != W25Qxx_ERR_NONE) return;

		/*------------------------------- Update next parameters --------------------------------*/

		/* Determine if writing is completed */
		if (NumByteToWrite == remPage) break;
		else
		{
			pBuffer += remPage;
			ByteAddr += remPage;
			NumByteToWrite -= remPage;
			remPage = (NumByteToWrite > W25Qxx_PAGESIZE) ? W25Qxx_PAGESIZE : NumByteToWrite;
		}
	}

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_DIR_Program_Security(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err)  		 	/* No check Direct program Security (0-256), Notes : no beyond page address */
{
	/* Security Area : No check Direct Page write
<<<<<<< HEAD
	 * 1. Write data of the specified length at the specified address,
	 *    but ensure that the data is in the same page.
	 * 2. You must ensure that all data within the written address range is 0xFF,
	 *    otherwise the data written at a location other than 0xFF will fail.
=======
	   * 1. Write data of the specified length at the specified address,
	   *    but ensure that the data is in the same page.
	   * 2. You must ensure that all data within the written address range is 0xFF,
	   *    otherwise the data written at a location other than 0xFF will fail.
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	**/
	uint16_t numPage = 0;
	uint32_t startAddr = 0;
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToWrite == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Determine if the address > startAddr + W25Qxx_PAGESIZE */
	numPage = ByteAddr >> W25Qxx_SECTORPOWER;
	startAddr = numPage * 0x00001000;
	if (numPage > 3 || numPage == 0 || ByteAddr + NumByteToWrite > startAddr + W25Qxx_PAGESIZE)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* write address */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_WSECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_WSECREG);
	W25Qxx.port.spi_rw(0x00);
	W25Qxx.port.spi_rw((uint8_t)((ByteAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)ByteAddr);
#endif
	
	/* write data */
	for (i = 0; i < NumByteToWrite; i++)
	{
		W25Qxx.port.spi_rw(pBuffer[i]);
	}

	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	/* wait for Erase or write end */
	W25Qxx_isStatus(W25Qxx_STATUS_IDLE | W25Qxx_STATUS_SUSPEND, W25Qxx.info.ProgrMaxTimePage, err);
	if (*err != W25Qxx_ERR_NONE) return;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Program(uint8_t *pBuffer, uint32_t ByteAddr, uint32_t NumByteToWrite, W25Qxx_ERR *err)   						/* Check program */
{
	/* Check program
	 * Built in data erasure operation !!!
	 * pBuffer        : Data storage area
	 * WriteAddr      : Write address (24bit)
	 * NumByteToWrite : Number of writes (max : sizeChip)
	**/
	uint32_t numSec = 0;
	uint16_t offSec = 0;
	uint16_t remSec = 0;
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToWrite == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* First Sector remain bytes */
<<<<<<< HEAD
	numSec = ByteAddr >> W25Qxx_SECTORPOWER; 		/* calculate sector number address ( numSec = ByteAddr / W25Qxx_SECTORSIZE; ) */
=======
	numSec = ByteAddr >> W25Qxx_SECTORPOWER; 			/* calculate sector number address ( numSec = ByteAddr / W25Qxx_SECTORSIZE; ) */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	offSec = ByteAddr & (W25Qxx_SECTORSIZE - 1); 	/* calculate sector offset address ( offSec = ByteAddr % W25Qxx_SECTORSIZE; ) */
	remSec = W25Qxx_SECTORSIZE - offSec;
	if (NumByteToWrite <= remSec) remSec = NumByteToWrite;

	while (1)
	{
		/*---------------------------------- Check Data Area ------------------------------------*/

		/* read current sector data to buffer area */
		W25Qxx_Read(W25QXX_CACHE, numSec * W25Qxx_SECTORSIZE, W25Qxx_SECTORSIZE, err);
		if (*err != W25Qxx_ERR_NONE) return;

		/* Check whether the current sector data is 0xFF */
		for (i = 0; i < remSec; i++)
		{
			if (W25QXX_CACHE[offSec + i] != 0xFF) break;
		}

		/*------------------------------------ Write data ---------------------------------------*/

		if (i < remSec)					/* need to be erased */
		{
			/* erase current sector */
			W25Qxx_Erase_Sector(numSec, err);
			if (*err != W25Qxx_ERR_NONE) return;

			/* copy data to buffer area */
			for (i = 0; i < remSec; i++)
			{
				W25QXX_CACHE[offSec + i] = pBuffer[i];
			}

			/* Write the entire sector */
			W25Qxx_DIR_Program(W25QXX_CACHE, numSec * W25Qxx_SECTORSIZE, W25Qxx_SECTORSIZE, err);
			if (*err != W25Qxx_ERR_NONE) return;
		}
<<<<<<< HEAD
		else							/* no need to be erased */
=======
		else										/* no need to be erased */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
		{
			/* Directly write the remaining section of the sector */
			W25Qxx_DIR_Program(pBuffer, ByteAddr, remSec, err);
			if (*err != W25Qxx_ERR_NONE) return;
		}

<<<<<<< HEAD
		numSec++;						/* updata sector address */
		offSec = 0;						/* reset  sector offset  */
=======
		numSec++;								/* updata sector address */
		offSec = 0;							/* reset  sector offset  */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b

		/*------------------------------- Update next parameters --------------------------------*/

		/* Determine if writing is completed */
		if (NumByteToWrite == remSec) break;
		else
		{
			pBuffer += remSec;
			ByteAddr += remSec;
			NumByteToWrite -= remSec;
			remSec = (NumByteToWrite > W25Qxx_SECTORSIZE) ? W25Qxx_SECTORSIZE : NumByteToWrite;
		}
	}

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_Program_Security(uint8_t *pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err)		   		 	/* Check program Security (0-256 at a time) */
{
	/* Check Security program
	 * Built in data erasure operation !!!
	 * pBuffer        : Data storage area
	 * WriteAddr      : Write address (24bit)
	 * NumByteToWrite : Number of writes (max : 256)
	**/
	uint8_t  numPage = 0;
	uint16_t offPage = 0;
	uint16_t remPage = 0;
	uint16_t i = 0;

	/* Determine if the number is 0 */
	if (NumByteToWrite == 0x00)
	{
		*err = W25Qxx_ERR_INVALID;
		return;
	}

	/* First page remain bytes */
	numPage = ByteAddr >> W25Qxx_SECTORPOWER; 			/* calculate page number address ( numPage = ByteAddr / W25Qxx_SECTORPOWER; ) */
	offPage = ByteAddr & (W25Qxx_PAGESIZE - 1); 		/* calculate page offset address ( offPage = ByteAddr % W25Qxx_PAGESIZE; ) */
	remPage = W25Qxx_PAGESIZE - offPage;

	if (numPage > 3 || numPage == 0)
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

	if (NumByteToWrite <= remPage) remPage = NumByteToWrite;
	else
	{
		*err = W25Qxx_ERR_BYTEADDRBOUND;
		return;
	}

	/*---------------------------------- Check Data Area ------------------------------------*/

	/* read current page data to buffer area */
	W25Qxx_Read_Security(W25QXX_CACHE, numPage * 0x00001000, W25Qxx_PAGESIZE, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Check whether the current page data is 0xFF */
	for (i = 0; i < remPage; i++)
	{
		if (W25QXX_CACHE[offPage + i] != 0xFF) break;
	}

	/*------------------------------------ Write data ---------------------------------------*/

	if (i < remPage)			/* need to be erased */
	{
		/* erase current page */
		W25Qxx_Erase_Security(numPage, err);
		if (*err != W25Qxx_ERR_NONE) return;

		/* copy data to buffer area */
		for (i = 0; i < remPage; i++)
		{
			W25QXX_CACHE[offPage + i] = pBuffer[i];
		}

		/* Ensure that the written data is in the same page, write the entire page */
		W25Qxx_DIR_Program_Security(W25QXX_CACHE, numPage * 0x00001000, W25Qxx_PAGESIZE, err);
		if (*err != W25Qxx_ERR_NONE) return;
	}
<<<<<<< HEAD
	else						/* no need to be erased */
=======
	else									/* no need to be erased */
>>>>>>> 2c58ac6170ef450bca98b8863018248f6d0a441b
	{
		/* Ensure that the written data is in the same page, directly write the remaining section of the page */
		W25Qxx_DIR_Program_Security(pBuffer, ByteAddr, remPage, err);
		if (*err != W25Qxx_ERR_NONE) return;
	}

	/*------------------------------- Update next parameters --------------------------------*/

	// No Check Data

	*err = W25Qxx_ERR_NONE;
}
/* W25Qxx config */
void W25Qxx_QueryChip(W25Qxx_ERR *err)																						/* Retrieve chip model and configuration information */
{
	uint8_t numlist = 0;
	uint8_t i = 0;

	/* search for chip model */
	numlist = sizeof(W25QInfoList) / sizeof(W25Qxx_INFO_t);
	for (i = 0; i < numlist; i++)
	{
		if (W25Qxx.IDJEDEC == (W25QInfoList[i].type & W25Qxx.IDJEDEC))
		{
			W25Qxx.info = W25QInfoList[i];
			break;
		}
	}
	if (i == numlist)
	{
		*err = W25Qxx_ERR_CHIPNOTFOUND;
		return;
	}

	/* calculate Block/Sector/Page num */
	W25Qxx.numBlock = 1 << (BcdToByte(W25Qxx.IDJEDEC & 0x000000FF) - 10);
	W25Qxx.numSector = W25Qxx.numBlock * 16;
	W25Qxx.numPage = W25Qxx.numSector * 16;

	/* calculate Block/Sector/Page size */
	W25Qxx.sizePage = 0x100;
	W25Qxx.sizeSector = W25Qxx.sizePage * 16;
	W25Qxx.sizeBlock = W25Qxx.sizeSector * 16;
	W25Qxx.sizeChip = W25Qxx.sizeSector * W25Qxx.numSector;

	*err = W25Qxx_ERR_NONE;
}
void W25Qxx_config(W25Qxx_ERR *err)																							/* config W25Qxx Chip */
{
	/* Determine the validity of the port */
	if (W25Qxx.port.spi_rw == NULL
		| W25Qxx.port.spi_cs_H == NULL
		| W25Qxx.port.spi_cs_L == NULL
		| W25Qxx.port.spi_delayms == NULL)
	{
		*err = W25Qxx_ERR_HARDWARE;
		return;
	}

	/* reset device */
	W25Qxx_Reset();

#if W25QXX_SUPPORT_SFDP

	/* Support SFDP */

#else
	/* Read Unique ID */
	W25Qxx.IDUnique = W25Qxx_ID_Unique();

	/* Read Manufacturer ID */
	W25Qxx.IDManufacturer = W25Qxx_ID_Manufacturer();

	/* Read JEDEC ID */
	W25Qxx.IDJEDEC = W25Qxx_ID_JEDEC();

	/* Query W25Qxx Type */
	W25Qxx_QueryChip(err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* Read Register */
	W25Qxx_ReadStatusRegister(1);
	W25Qxx_ReadStatusRegister(2);
	W25Qxx_ReadStatusRegister(3);

	/* Set address mode  */
	if (W25Qxx.numBlock >= 512)								/* Block >= 512 have 4 address */
	{
#if W25QXX_4BADDR
		W25Qxx_4ByteMode();
#else
		W25Qxx_3ByteMode();
#endif
	}
	else
	{
		W25Qxx_3ByteMode();
	}

#endif

}
/* W25Qxx Suspend/Resume Test */
void W25Qxx_SusResum_Erase_Sector(uint32_t SectorAddr, W25Qxx_ERR *err)
{
	/* Determine if Sector Addrress Bound */
	if (SectorAddr >= W25Qxx.numSector)
	{
		*err = W25Qxx_ERR_SECTORADDRBOUND;
		return;
	}

	/* Determine if it is busy */
	W25Qxx_isStatus(W25Qxx_STATUS_BUSY, 0, err);
	if (*err != W25Qxx_ERR_NONE) return;

	/* calculate sector address */
	SectorAddr *= W25Qxx.sizeSector;

#if W25QXX_4BADDR == 0
	/* Address > 0xFFFFFF */
	if (SectorAddr > 0xFFFFFF)
	{
		W25Qxx_WriteExtendedRegister((uint8_t)((SectorAddr) >> 24));
	}
	else
	{
		W25Qxx_WriteExtendedRegister(0x00);
	}
#endif
	
	/* write enable */
	W25Qxx_WriteEnable();

	/* CS enable */
	W25Qxx.port.spi_cs_L();

	/* erase data */
#if W25QXX_4BADDR
	W25Qxx.port.spi_rw(W25Q_CMD_ESECTOR);
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 24));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)SectorAddr);
#else
	W25Qxx.port.spi_rw(W25Q_CMD_ESECTOR);
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 16));
	W25Qxx.port.spi_rw((uint8_t)((SectorAddr) >> 8));
	W25Qxx.port.spi_rw((uint8_t)SectorAddr);
#endif
	
	/* CS disable */
	W25Qxx.port.spi_cs_H();

	/* write disable */
	W25Qxx_WriteDisable();

	*err = W25Qxx_ERR_NONE;
}

