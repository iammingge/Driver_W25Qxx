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
 * @brief   W25Qxx family driver header file
 * @author  iammingge
 * 
 *      DATE             NAME                      DESCRIPTION 
 * 
 *   11-27-2022        iammingge                Initial Version 1.0
 *    
**/

#ifndef __W25QXX_H
#define __W25QXX_H

/* SPI Header File */
//#include "spi.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifdef W25QXX_GLOBALS
#define W25QXX_EXT
#else
#define W25QXX_EXT extern
#endif


/**
 * @brief W25Q and W25X Series Drive (The W25Q family is a "superset" of the W25X family)
 *             
 * 				Number of supported mounts (1)
 *
 * 				Standard SPI  (¡Ì)      3ByteAddress (¡Ì)      
 * 				Dual     SPI  (x)      4ByteAddress (x)
 * 				Quad     SPI  (x)      
 */
#define W25QXX_FASTREAD    									 0					/* 0 : No Fast Read Mode   ; 1 : Fast Read Mode */
#define W25QXX_4BADDR      									 0					/* 0 : 3 Byte Address Mode ; 1 : 4 Byte Address Mode */
#define W25QXX_SUPPORT_SFDP									 0					/* 0 : No support SFDP     ; 1 : Support SFDP */

/**
 * @brief W25Qxx CMD
 */	
#define W25Q_CMD_WEN		             				 0x06
#define W25Q_CMD_VOLATILESREN        				 0x50
#define W25Q_CMD_WDEN   		         				 0x04				 
#define W25Q_CMD_POWEREN    	       				 0xAB
#define W25Q_CMD_MANUFACTURER								 0x90
#define W25Q_CMD_JEDECID		         				 0x9F 
#define W25Q_CMD_UNIQUEID            				 0x4B				 
#define W25Q_CMD_READ			           				 0x03
#define W25Q_CMD_4BREAD              				 0x13
#define W25Q_CMD_FASTREAD            				 0x0B
#define W25Q_CMD_4BFASTREAD          				 0x0C				 
#define W25Q_CMD_WPAGE  		         				 0x02
#define W25Q_CMD_4BWPAGE             				 0x12				 
#define W25Q_CMD_ESECTOR		   	 		 				 0x20
#define W25Q_CMD_4BESECTOR		 	 		 				 0x21
#define W25Q_CMD_E32KBLOCK			 		 				 0x52
#define W25Q_CMD_E64KBLOCK			 		 				 0xD8
#define W25Q_CMD_4BE64KBLOCK		 		 				 0xDC
#define W25Q_CMD_ECHIP			     		 				 0xC7   /* or 0x60 */
#define W25Q_CMD_RSREG1		           				 0x05
#define W25Q_CMD_WSREG1              				 0x01
#define W25Q_CMD_RSREG2		           				 0x35
#define W25Q_CMD_WSREG2              				 0x31
#define W25Q_CMD_RSREG3	             				 0x15
#define W25Q_CMD_WSREG3              				 0x11
#define W25Q_CMD_REXTREG    	       				 0xC8
#define W25Q_CMD_WEXTREG 		         				 0xC5
#define W25Q_CMD_RSFDP               				 0x5A
#define W25Q_CMD_ESECREG         		 				 0x44
#define W25Q_CMD_WSECREG             				 0x42
#define W25Q_CMD_RSECREG             				 0x48
#define W25Q_CMD_WALLBLOCKLOCK    	 				 0x7E
#define W25Q_CMD_WALLBLOCKUNLOCK  	 				 0x98
#define W25Q_CMD_RBLOCKLOCK          				 0x3D
#define W25Q_CMD_WSIGBLOCKLOCK       				 0x36
#define W25Q_CMD_WSIGBLOCKUNLOCK     				 0x39	 
#define W25Q_CMD_EWSUSPEND           				 0x75
#define W25Q_CMD_EWRESUME            				 0x7A
#define W25Q_CMD_POWERDEN			       				 0xB9 	 
#define W25Q_CMD_4ByteAddrEN         				 0xB7
#define W25Q_CMD_4ByteAddrDEN        				 0xE9
#define W25Q_CMD_ENRESET             				 0x66
#define W25Q_CMD_RESETDEV            				 0x99
#define W25Q_DUMMY                   				 0xA5
				 
/**
 * @brief W25Qxx SIZE
 */				 
#define W25Qxx_PAGESIZE							 				 0x00100
#define W25Qxx_PAGEPOWER										 0x08
#define W25Qxx_SECTORSIZE						 				 0x01000
#define W25Qxx_SECTORPOWER									 0x0C
#define W25Qxx_BLOCKSIZE						 				 0x10000
#define W25Qxx_BLOCKPOWER									 	 0x10
#define W25Qxx_SECTURITYSIZE								 0x00300

/**
 * @brief W25Qxx Address Convert
 */	
#define W25Qxx_PAGEADDR(ByteAddr)					   (uint32_t)(ByteAddr >> W25Qxx_PAGEPOWER)						/* Convert Byte address to page address */
#define W25Qxx_SECTORADDR(ByteAddr) 				 (uint32_t)(ByteAddr >> W25Qxx_SECTORPOWER)					/* Convert Byte address to sector address */
#define W25Qxx_BLOCK64ADDR(ByteAddr) 				 (uint32_t)(ByteAddr >> W25Qxx_BLOCKPOWER)					/* Convert Byte address to block64 address */
#define W25Qxx_BLOCK32ADDR(ByteAddr) 				 (uint32_t)(ByteAddr >> (W25Qxx_BLOCKPOWER - 1))		/* Convert Byte address to block32 address */

/**
 * @brief W25Qxx Chip Type
 */	
typedef enum {
	UNKNOWN = 0x000000,
	W25X05  = 0xEFFF10,												/* W25X05 */
	W25X10  = 0xEFFF11,												/* W25X10 */
	W25Q20  = 0xEFFF12,												/* W25Q20 */
	W25Q40  = 0xEFFF13,												/* W25Q40 */
	W25Q80  = 0xEFFF14,												/* W25Q80 */
	W25Q16  = 0xEFFF15,												/* W25Q16 */
	W25Q32  = 0xEFFF16,												/* W25Q32 */
	W25Q64  = 0xEFFF17,												/* W25Q64 */
	W25Q128 = 0xEFFF18,												/* W25Q128 ( 0xEF4018 0xEF7018 ) */
	W25Q256 = 0xEFFF19,												/* W25Q256 */
	W25Q512 = 0xEFFF20,												/* W25Q512 */
	W25Q01  = 0xEFFF21,												/* W25Q01 */
	W25Q02  = 0xEFFF22												/* W25Q02 */
} W25Qxx_CHIP;

/**
 * @brief W25Qxx Error Information
 */	
typedef enum {
	W25Qxx_ERR_NONE             = 0x00,				/* No error */
	W25Qxx_ERR_STATUS           = 0x02,				/* Device is suspended */
	W25Qxx_ERR_LOCK             = 0x03,			  /* Device is locked */
	W25Qxx_ERR_INVALID          = 0x04,				/* Invalid value entered */
	W25Qxx_ERR_CHIPNOTFOUND     = 0x05,				/* Chip Model is not found */
	W25Qxx_ERR_BYTEADDRBOUND    = 0x06,				/* Byte address out of bounds */
	W25Qxx_ERR_PAGEADDRBOUND    = 0x07,				/* Page address out of bounds */
	W25Qxx_ERR_SECTORADDRBOUND  = 0x08,				/* Sector address out of bounds */
	W25Qxx_ERR_BLOCK64ADDRBOUND = 0x09,				/* Block64 address out of bounds */
	W25Qxx_ERR_BLOCK32ADDRBOUND = 0x0A,				/* Block32 address out of bounds */
	W25Qxx_ERR_WPSMODE          = 0x0B,				/* Write protect mode is error */
	W25Qxx_ERR_HARDWARE         = 0x0C				/* SPI/QSPI BUS is hardware error */
} W25Qxx_ERR;

/**
 * @brief W25Qxx Status Information
 */	
typedef enum {
	W25Qxx_STATUS_IDLE             = 0x01,
	W25Qxx_STATUS_BUSY             = 0x02,
	W25Qxx_STATUS_SUSPEND          = 0x04,
	W25Qxx_STATUS_BUSY_AND_SUSPEND = 0x08
} W25Qxx_STATUS;

/**
 * @brief W25Qxx StatusRegister Storage Mode
 */	
typedef enum {
	W25Qxx_VOLATILE     = 0x00,								/* StatusRegister Voliate Mode */
	W25Qxx_NON_VOLATILE = 0x01 								/* StatusRegister Non-Voliate Mode */
} W25Qxx_SRM;

/**
 * @brief W25Qxx Chip Parameter
 */	
typedef struct {
	W25Qxx_CHIP type;													/* Chip type */
	uint32_t ProgrMaxTimePage;      					/* Program       max time (ms) */
	uint32_t EraseMaxTimeSector;    					/* Erase Sector  max time (ms) */
	uint32_t EraseMaxTimeBlock64;   					/* Erase Block64 max time (ms) */
	uint32_t EraseMaxTimeBlock32;   					/* Erase Block32 max time (ms) */
	uint32_t EraseMaxTimeChip;      					/* Erase Chip    max time (ms) */
} W25Qxx_INFO_t;					
					
/**
 * @brief W25Qxx BUS Port
 */	
typedef struct {
	void (*spi_delayms)(uint32_t ms);
	void (*spi_cs_H)(void);
	void (*spi_cs_L)(void);
	uint8_t (*spi_rw)(uint8_t data);
} W25Qxx_PORT_t;

/**
 * @brief W25Qxx Chip Information
 */				
typedef struct {					
	W25Qxx_INFO_t info;												/* Chip Parameter */
	W25Qxx_PORT_t port;												/* BUS Port */
	uint16_t IDManufacturer;									/* Manufacturer ID */
	uint32_t IDJEDEC;													/* JEDEC        ID */
	uint64_t IDUnique;												/* Unique       ID */
	uint32_t numBlock;              					/* Block  number */
	uint32_t numPage;               					/* Page   number */
	uint32_t numSector;             					/* Sector number */
	uint16_t sizePage;				      					/* Page   size (Byte) */
	uint32_t sizeSector;			      					/* Sector size (Byte) */
	uint32_t sizeBlock;				      					/* Block  size (Byte) */
	uint32_t sizeChip;				      					/* Chip   size (Byte) */
	uint8_t StatusRegister1;									/* StatusRegister 1 */
	uint8_t StatusRegister2;									/* StatusRegister 2 */
	uint8_t StatusRegister3;									/* StatusRegister 3 */
	uint8_t ExtendedRegister;       					/* ExtendedRegister */
} W25Qxx_t;

/**
 * @brief W25Qxx Read Manufacturer/JEDEC/Unique ID
 */
uint16_t W25Qxx_ID_Manufacturer(void);
uint32_t W25Qxx_ID_JEDEC(void);
uint64_t W25Qxx_ID_Unique(void);

/**
 * @brief W25Qxx Individual Control Instruction
 */
void W25Qxx_Reset(void);
void W25Qxx_PowerEnable(void);
void W25Qxx_PowerDisable(void);
void W25Qxx_VolatileSR_WriteEnable(void);
void W25Qxx_WriteEnable(void);
void W25Qxx_WriteDisable(void);
void W25Qxx_4ByteMode(void);
void W25Qxx_3ByteMode(void);
void W25Qxx_Suspend(void);
void W25Qxx_Resume(void);

/**
 * @brief W25Qxx Read sector/block lock of current address status
 */
uint8_t W25Qxx_ReadLock(uint32_t ByteAddr);

/**
 * @brief W25Qxx Read/Write ExtendedRegister
 */
void W25Qxx_ReadExtendedRegister(void);
void W25Qxx_WriteExtendedRegister(uint8_t ExtendedAddr);

/**
 * @brief W25Qxx Read/Write StatusRegister
 */
void W25Qxx_ReadStatusRegister (uint8_t Select_SR_1_2_3);
void W25Qxx_WriteStatusRegister(uint8_t Select_SR_1_2_3, uint8_t Data);
void W25Qxx_VolatileSR_WriteStatusRegister(uint8_t Select_SR_1_2_3, uint8_t Data);
uint8_t W25Qxx_RBit_WEL (void);
uint8_t W25Qxx_RBit_BUSY(void);
uint8_t W25Qxx_RBit_SUS (void);
uint8_t W25Qxx_RBit_ADS (void);
void W25Qxx_WBit_SRP(W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_TB (W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_CMP(W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_QE (W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_SRL(W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_WPS(W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_DRV(W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_BP (W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_LB (W25Qxx_SRM srm, uint8_t bit);
void W25Qxx_WBit_ADP(uint8_t bit);
uint8_t W25Qxx_ReadStatus(void);

/**
 * @brief W25Qxx read current status
 */
void W25Qxx_isStatus(uint8_t Select_Status, uint32_t timeout, W25Qxx_ERR *err);

/**
 * @brief W25Qxx Sector/Blcok Lock protect for " WPS = 1 "
 */
void W25Qxx_Global_UnLock(W25Qxx_ERR *err);	
void W25Qxx_Global_Locked(W25Qxx_ERR *err);																
void W25Qxx_Individual_UnLock(uint32_t ByteAddr, W25Qxx_ERR *err);								
void W25Qxx_Individual_Locked(uint32_t ByteAddr, W25Qxx_ERR *err);

/**
 * @brief W25Qxx Main/SFDP/Security Storage read/program/erase function
 */
void W25Qxx_Erase_Chip          (W25Qxx_ERR *err);
void W25Qxx_Erase_Block64       (uint32_t Block64Addr, W25Qxx_ERR *err);
void W25Qxx_Erase_Block32       (uint32_t Block32Addr, W25Qxx_ERR *err);
void W25Qxx_Erase_Sector        (uint32_t SectorAddr,  W25Qxx_ERR *err);
void W25Qxx_Erase_Security      (uint32_t SectorAddr,  W25Qxx_ERR *err);
void W25Qxx_Read                (uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead,  W25Qxx_ERR *err);
void W25Qxx_Read_Security       (uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead,  W25Qxx_ERR *err);
void W25Qxx_Read_SFDP           (uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToRead,  W25Qxx_ERR *err);
void W25Qxx_DIR_Program_Page    (uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err);
void W25Qxx_DIR_Program         (uint8_t* pBuffer, uint32_t ByteAddr, uint32_t NumByteToWrite, W25Qxx_ERR *err);
void W25Qxx_DIR_Program_Security(uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err);
void W25Qxx_Program             (uint8_t* pBuffer, uint32_t ByteAddr, uint32_t NumByteToWrite, W25Qxx_ERR *err);
void W25Qxx_Program_Security    (uint8_t* pBuffer, uint32_t ByteAddr, uint16_t NumByteToWrite, W25Qxx_ERR *err);

/**
 * @brief W25Qxx config function
 */
W25QXX_EXT W25Qxx_t W25Qxx;
void W25Qxx_QueryChip(W25Qxx_ERR *err);
void W25Qxx_config(W25Qxx_ERR *err);
void W25Qxx_SusResum_Erase_Sector(uint32_t SectorAddr, W25Qxx_ERR *err);

#ifdef __cplusplus
}
#endif

#endif
