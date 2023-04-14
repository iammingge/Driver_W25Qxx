/**
  * @file  : example.c
  * @brief : W25Qxx config and test example
  */
#include "W25Qxx.h"
#define TEST_MODE 1			/* TEST_MODE = 1/2/3 */
uint8_t buff[500];
W25Qxx_ERR err;
static void SPI5_DelayMS(uint32_t ms)
{
	/* STM32 */
	HAL_Delay(ms);    /*>> user add <<*/
}
static uint8_t SPI5_RW(uint8_t Data)																																		 /* W25Qxx SPI Read and Write of Byte */
{
	uint8_t ret;

	/* STM32 */
	HAL_SPI_TransmitReceive(&hspi5, &Data, &ret, 1, 1000);    /*>> user add <<*/

	return ret;
}
static void SPI5_CS_H(void)																																							 /* W25Qxx SPI Read and Write of Byte */
{
	/* STM32 */
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);       /*>> user add <<*/
}
static void SPI5_CS_L(void)																																							 /* W25Qxx SPI Read and Write of Byte */
{
	/* STM32 */
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);     /*>> user add <<*/
}
/* Main */
int main(void)
{
	/* SPI hardware init */
	/*>> user add <<*/

	/* W25Qxx Init */
	W25Qxx.port.spi_delayms = SPI5_DelayMS;
	W25Qxx.port.spi_rw = SPI5_RW;
	W25Qxx.port.spi_cs_H = SPI5_CS_H;
	W25Qxx.port.spi_cs_L = SPI5_CS_L;
	W25Qxx_config(&err);
	if (err != W25Qxx_ERR_NONE) while (1);

	/* W25Qxx Erase Chip */
    W25Qxx_Erase_Chip(&err);
    if (err != W25Qxx_ERR_NONE) while(1);

	while (1)
	{

#if TEST_MODE == 1

		/* <<< Test Read and Write >>>
		**/
		uint8_t aa[24], num;

		num = sprintf((char *)aa, "Test Read and Write %03d", 123);
		W25Qxx_Program(aa, 0x00124567, num, &err);           	            /* Write */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00124567, sizeof(buff), &err);           	    /* Read  */
		if (err != W25Qxx_ERR_NONE) while (1);
		num = sprintf((char *)aa, "Test Read and Write %03d", 456);
		W25Qxx_Program(aa, 0x00124567, num, &err);           	            /* Write */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00124567, sizeof(buff), &err);           	    /* Read  */
		if (err != W25Qxx_ERR_NONE) while (1);

		/* <<< Test Block64 Erase >>>
		 * Block64 127 : 0x007F0000 --- 0x007FFFFF (ByteAddr)
		 * Block64 128 : 0x00800000 --- 0x0080FFFF (ByteAddr)
		 * Block64 255 : 0x00FF0000 --- 0x00FFFFFF (ByteAddr)
		 * Block64 256 : 0x01000000 --- 0x0100FFFF (ByteAddr)
		**/
		uint8_t bb[] = "Test Block64 Erase";

		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x007F1234), &err);     	/* Erase Block64 127 */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x007F0000, sizeof(buff), &err);             		/* Read  Block64 127          "                  " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(bb, 0x007F0000, sizeof(bb), &err);             		/* Write Block64 127                               */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x007F0000, sizeof(buff), &err);             		/* Read  Block64 127          "Test Block64 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x007FFFFF - 3, sizeof(buff), &err);           	/* Read  Block64 127 and 128  "     Block64 Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(bb, 0x007FFFFF - 3, sizeof(bb), &err);           	/* Write Block64 127 and 128                       */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x007FFFFF - 3, sizeof(buff), &err);           	/* Read  Block64 127 and 128  "Test Block64 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x01001234), &err);     	/* Erase Block64 256 */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x01000000, sizeof(buff), &err);             		/* Read  Block64 255          "                  " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(bb, 0x01000000, sizeof(bb), &err);             		/* Write Block64 255                               */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x01000000, sizeof(buff), &err);             	    /* Read  Block64 255          "Test Block64 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x0100FFFF - 3, sizeof(buff), &err);           	/* Read  Block64 255 and 256  "     Block64 Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(bb, 0x0100FFFF - 3, sizeof(bb), &err);           	/* Write Block64 255 and 256                       */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x0100FFFF - 3, sizeof(buff), &err);           	/* Read  Block64 255 and 256  "Test Block64 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);


		/* <<< Test Block32 Erase >>>
		 * Block64 0 - Block32 0  : 0x00000000 --- 0x00007FFF (ByteAddr)
		 * Block64 0 - Block32 1  : 0x00008000 --- 0x0000FFFF (ByteAddr)
		 * Block64 5 - Block32 10 : 0x00050000 --- 0x00057FFF (ByteAddr)
		 * Block64 5 - Block32 11 : 0x00058000 --- 0x0005FFFF (ByteAddr)
		**/
		uint8_t cc[] = "Test Block32 Erase";

		W25Qxx_Erase_Block32(W25Qxx_BLOCK32ADDR(0x08123), &err);            /* Erase Block32 1 */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00000000, sizeof(buff), &err);              	/* Read  Block32 0            "Test Block32 Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(cc, 0x00000000, sizeof(cc), &err);              		/* Write Block32 0                                 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00000000, sizeof(buff), &err);              	/* Read  Block32 0            "Test Block32 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00007FFF - 3, sizeof(buff), &err);              /* Read  Block32 0 and 1      "Test              " (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(cc, 0x00007FFF - 3, sizeof(cc), &err);              	/* Write Block32 0 and 1                           */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00007FFF - 3, sizeof(buff), &err);              /* Read  Block32 0 and 1      "Test Block32 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Block32(W25Qxx_BLOCK32ADDR(0x51234), &err);        	/* Erase Block32 10 */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00050000, sizeof(buff), &err);              	/* Read  Block32 10           "                  " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(cc, 0x00050000, sizeof(cc), &err);              		/* Write Block32 10                                */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00050000, sizeof(buff), &err);              	/* Read  Block32 10           "Test Block32 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00057FFF - 3, sizeof(buff), &err);              /* Read  Block32 10 and 11    "     Block32 Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(cc, 0x00057FFF - 3, sizeof(cc), &err);              	/* Write Block32 10 and 11                         */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00057FFF - 3, sizeof(buff), &err);              /* Read  Block32 10 and 11    "Test Block32 Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		/* <<< Test Sector Erase >>>
		 * Sector 0    : 0x00000000 - 0x00000FFF (ByteAddr)
		 * Sector 1    : 0x00001000 - 0x00001FFF (ByteAddr)
		 * Sector 2    : 0x00002000 - 0x00002FFF (ByteAddr)
		 * Sector 8190 : 0x01FFE000 - 0x01FFEFFF (ByteAddr)
		 * Sector 8191 : 0x01FFF000 - 0x01FFFFFF (ByteAddr)
		**/
		uint8_t dd[] = "Test Sector Erase";

		W25Qxx_Erase_Sector(W25Qxx_SECTORADDR(0x00000123), &err);
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00000000, sizeof(buff), &err);              	/* Read  Sector 0               "                 " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(dd, 0x00000000, sizeof(dd), &err);              		/* Write Sector 0                                   */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00000000, sizeof(buff), &err);              	/* Read  Sector 0               "Test Sector Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x00000FFF - 3, sizeof(buff), &err);              /* Read  Sector 0 and 1         "     Sector Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(dd, 0x00000FFF - 3, sizeof(dd), &err);              	/* Write Sector 0 and 1                             */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00000FFF - 3, sizeof(buff), &err);              /* Read  Sector 0 and 1         "Test Sector Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Sector(W25Qxx_SECTORADDR(0x01FFF123), &err);
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x01FFE000, sizeof(buff), &err);              	/* Read  Sector 8190            "Test Sector Erase" (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(dd, 0x01FFE000, sizeof(dd), &err);              		/* Write Sector 8190                                */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x01FFE000, sizeof(buff), &err);              	/* Read  Sector 8190            "Test Sector Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read(buff, 0x01FFEFFF - 3, sizeof(buff), &err);              /* Read  Sector 8190 and 8191   "Test             " (second time) */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(dd, 0x01FFEFFF - 3, sizeof(dd), &err);               /* Write Sector 8190 and 8191                       */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x01FFEFFF - 3, sizeof(buff), &err);              /* Read  Sector 8190 and 8191   "Test Sector Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		/* <<< Test Security Read/Write/Erase >>>
		 * Page 0 : 0x00001000 - 0x000010FF (ByteAddr)
		 * Page 1 : 0x00002000 - 0x000020FF (ByteAddr)
		 * Page 2 : 0x00003000 - 0x000030FF (ByteAddr)
		**/
		uint8_t ee[] = "Test Security Read/Write/Erase";

		W25Qxx_Erase_Security(W25Qxx_SECTORADDR(0x00001034), &err);
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read_Security(buff, 0x00001000, 0x100, &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program_Security(ee, 0x000010F0, sizeof(ee), &err);		    /* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Program_Security(ee, 0x00001012, sizeof(ee), &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read_Security(buff, 0x00001000, 0x100, &err);				/* Read Security Page 0			"                  Test Security Read/Write/Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Security(W25Qxx_SECTORADDR(0x00002034), &err);
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read_Security(buff, 0x00002000, 0x100, &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program_Security(ee, 0x000020F0, sizeof(ee), &err);		    /* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Program_Security(ee, 0x00002012, sizeof(ee), &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read_Security(buff, 0x00002000, 0x100, &err);				/* Read Security Page 1			"                  Test Security Read/Write/Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Security(W25Qxx_SECTORADDR(0x00003034), &err);
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Read_Security(buff, 0x00003000, 0x100, &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program_Security(ee, 0x000030F0, sizeof(ee), &err);		    /* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Program_Security(ee, 0x00003012, sizeof(ee), &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read_Security(buff, 0x00003000, 0x100, &err);				/* Read Security Page 2			"                  Test Security Read/Write/Erase" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Erase_Security(W25Qxx_SECTORADDR(0x00004034), &err);			/* Error address bound */
		if (err != W25Qxx_ERR_PAGEADDRBOUND) while (1);

		W25Qxx_Read_Security(buff, 0x00004000, 0x100, &err);				/* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Program_Security(ee, 0x000040F0, sizeof(ee), &err);		    /* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Program_Security(ee, 0x00004012, sizeof(ee), &err);			/* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);
		W25Qxx_Read_Security(buff, 0x00004000, 0x100, &err);				/* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);

#elif TEST_MODE == 2

		/* <<< Test Sector/Block Lock >>>
		 * Sector 0 : 0x00000000 - 0x00000FFF (ByteAddr)
		 * Sector 1 : 0x00001000 - 0x00001FFF (ByteAddr)
		 * Sector 2 : 0x00002000 - 0x00002FFF (ByteAddr)
		 * Block  1 : 0x00010000 - 0x0001FFFF (ByteAddr)
		 * Block  2 : 0x00020000 - 0x0002FFFF (ByteAddr)
		 * Block  3 : 0x00030000 - 0x0003FFFF (ByteAddr)
		**/
		uint8_t ff1[] = "Test  Sector/Block Lock";
		uint8_t ff2[] = "Trial Sector/Block Lock";

		W25Qxx_WBit_WPS(W25Qxx_NON_VOLATILE, 0);							/* WPS = 0 */
		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x00000000), &err);        	/* Erase Block64 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_WBit_WPS(W25Qxx_NON_VOLATILE, 1);							/* WPS = 1 */

		W25Qxx_Individual_Locked(0x00000000, &err);							/* Lock Sector 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00000000) == 0x00) while (1);					/* Read Sector 0 Lock Status */
		W25Qxx_Individual_UnLock(0x00001000, &err);							/* Lock Sector 1 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00001000) == 0x01) while (1);					/* Read Sector 1 Lock Status */

		W25Qxx_Program(ff1, 0x00000000, sizeof(ff1), &err);                 /* Write Block32 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00000000, sizeof(buff), &err);              	/* Read  Block32 0 "                       " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(ff2, 0x00010000, sizeof(ff2), &err);                 /* Write Block32 1 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00010000, sizeof(buff), &err);              	/* Read  Block32 1 "Trial Sector/Block Lock" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 0);                                /* WPS = 0 */
		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x00010000), &err);        	/* Erase Block64 1 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x00020000), &err);        	/* Erase Block64 2 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 1);                                /* WPS = 1 */

		W25Qxx_Individual_Locked(0x00010000, &err);                         /* Lock Block 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00010000) == 0x00) while (1);				    /* Read Block 0 Lock Status */
		W25Qxx_Individual_UnLock(0x00020000, &err);                         /* Lock Block 1 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00020000) == 0x01) while (1);                 /* Read Block 1 Lock Status */

		W25Qxx_Program(ff1, 0x00010000, sizeof(ff1), &err);                 /* Write Block64 1 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00010000, sizeof(buff), &err);              	/* Read  Block64 1 "                       " */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Program(ff2, 0x00020000, sizeof(ff2), &err);                 /* Write Block64 2 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00020000, sizeof(buff), &err);                  /* Read  Block64 2 "Trial Sector/Block Lock" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 0);                                /* WPS = 0 */
		W25Qxx_Erase_Block64(W25Qxx_BLOCK64ADDR(0x00030000), &err);        	/* Erase Block64 3 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 1);                                /* WPS = 1 */

		W25Qxx_Global_Locked(&err);											/* Lock All Sector/Block */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00030000) == 0x00) while (1);					/* Read Block 3 Lock Status */
		W25Qxx_Program(ff1, 0x00030000, sizeof(ff1), &err);                 /* Write Block64 3 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00030000, sizeof(buff), &err);                  /* Read  Block64 3 "                       " */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_Global_UnLock(&err);											/* UnLock All Sector/Block */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00030000) == 0x01) while (1);					/* Read Block 3 Lock Status */
		W25Qxx_Program(ff2, 0x00030000, sizeof(ff2), &err);                 /* Write Block64 3 */
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read(buff, 0x00030000, sizeof(buff), &err);              	/* Read  Block64 3 "Trial Sector/Block Lock" */
		if (err != W25Qxx_ERR_NONE) while (1);

		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 0);                                /* WPS = 0 */

		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 1);								/* WPS = 1 */
		W25Qxx_Individual_Locked(0x00000000, &err);			                /* Lock Sector 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00000000) == 0x00) while (1);					/* Read Sector 0 Lock Status */
		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 0);
		if (W25Qxx_ReadLock(0x00000000) == 0x00) while (1);					/* Read Sector 0 Lock Status */

		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 1);					    		/* WPS = 1 */
		W25Qxx_Individual_UnLock(0x00000000, &err);       					/* UnLock Sector 0 */
		if (err != W25Qxx_ERR_NONE) while (1);
		if (W25Qxx_ReadLock(0x00000000) == 0x01) while (1);					/* Read Sector 0 Lock Status */
		W25Qxx_WBit_WPS(W25Qxx_VOLATILE, 0);
		if (W25Qxx_ReadLock(0x00000000) == 0x01) while (1);					/* Read Sector 0 Lock Status */

		/* <<< Test SFDP Read >>>
		 * Page : 0x00000000 - 0x000000FF (ByteAddr)
		**/
		memset(buff, 0x00, 0x100);

		W25Qxx_Read_SFDP(buff, 0x00000000, 0x100, &err);
		if (err != W25Qxx_ERR_NONE) while (1);
		W25Qxx_Read_SFDP(buff, 0x00000000, 0x101, &err);               			  /* Error address bound */
		if (err != W25Qxx_ERR_BYTEADDRBOUND) while (1);

#elif TEST_MODE == 3

		/* <<< Test Erase/Program Suspend and Resume >>>
		 * Sector 0    : 0x00000000 - 0x00000FFF (ByteAddr)
		 * Sector 1    : 0x00001000 - 0x00001FFF (ByteAddr)
		 * Sector 2    : 0x00002000 - 0x00002FFF (ByteAddr)
		 * Sector 8190 : 0x01FFE000 - 0x01FFEFFF (ByteAddr)
		 * Sector 8191 : 0x01FFF000 - 0x01FFFFFF (ByteAddr)
		**/
		uint8_t gg[] = "Test Erase/Program Suspend and Resume";
		static bool flag = 1;

		if (flag)
		{
		    W25Qxx_Program(gg, 0x00001000, sizeof(gg), &err);
		    flag = 0;
		}

		W25Qxx_SusResum_Erase_Sector(W25Qxx_SECTORADDR(0x00000000), &err);
		
		W25Qxx_Suspend(&err);
		W25Qxx_Read(buff, 0x00001000, sizeof(buff), &err); 
		W25Qxx_Resume(&err);

			
		W25Qxx_Suspend(&err);
		W25Qxx_Read(buff, 0x00001000, sizeof(buff), &err); 
		W25Qxx_Resume(&err);

#endif

		HAL_Delay(500);
	}

}






