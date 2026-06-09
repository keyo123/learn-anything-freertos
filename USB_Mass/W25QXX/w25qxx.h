#ifndef __FLASH_H
#define __FLASH_H			    

#include "spi.h"

extern uint32_t W25QXX_TYPE;
//#define  sFLASH_ID              0xEF3015   //W25X16
//#define  sFLASH_ID              0xEF4015	 //W25Q16
#define  sFLASH_ID              0XEF4018   //W25Q128
//#define  sFLASH_ID              0XEF4016    //W25Q32

#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256

/*ÃüÁî¶¨Òå-¿ªÍ·*******************************/
#define W25X_WriteEnable		      0x06 
#define W25X_WriteDisable		      0x04 
#define W25X_ReadStatusReg		    0x05 
#define W25X_WriteStatusReg		    0x01 
#define W25X_ReadData			        0x03 
#define W25X_FastReadData		      0x0B 
#define W25X_FastReadDual		      0x3B 
#define W25X_PageProgram		      0x02 
#define W25X_BlockErase			      0xD8 
#define W25X_SectorErase		      0x20 
#define W25X_ChipErase			      0xC7 
#define W25X_PowerDown			      0xB9 
#define W25X_ReleasePowerDown	    0xAB 
#define W25X_DeviceID			        0xAB 
#define W25X_ManufactDeviceID   	0x90 
#define W25X_JedecDeviceID		    0x9F

/* WIP(busy)±êÖ¾£¬FLASHÄÚ²¿ÕýÔÚÐ´Èë */
#define WIP_Flag                  0x01
#define Dummy_Byte                0xFF
/*ÃüÁî¶¨Òå-½áÎ²*******************************/

#define  		SPI_FLASH_CS_LOW()     			  HAL_GPIO_WritePin(WQ25_CS_GPIO_Port, WQ25_CS_Pin, GPIO_PIN_RESET);
#define  		SPI_FLASH_CS_HIGH()    			  HAL_GPIO_WritePin(WQ25_CS_GPIO_Port, WQ25_CS_Pin, GPIO_PIN_SET);

/*SPI½Ó¿Ú¶¨Òå-½áÎ²****************************/

/*µÈ´ý³¬Ê±Ê±¼ä*/
#define SPIT_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define SPIT_LONG_TIMEOUT         ((uint32_t)(10 * SPIT_FLAG_TIMEOUT))

/*ÐÅÏ¢Êä³ö*/
#define FLASH_DEBUG_ON         1


void SPI_FLASH_Init(void);
void SPI_FLASH_SectorErase(uint32_t SectorAddr);
void SPI_FLASH_BulkErase(void);
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t SPI_FLASH_ReadID(void);
uint32_t SPI_FLASH_ReadDeviceID(void);
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);


uint8_t SPI_FLASH_ReadByte(void);
uint8_t SPI_FLASH_SendByte(uint8_t byte);
uint16_t SPI_FLASH_SendHalfWord(uint16_t HalfWord);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);


#endif /* __SPI_FLASH_H */


	  
////W25X系列/Q系列芯片列表	   
//#define W25Q80 	0XEF13 	
//#define W25Q16 	0XEF14
//#define W25Q32 	0XEF15
//#define W25Q64 	0XEF16
//#define W25Q128	0XEF17

//extern u16 W25QXX_TYPE;					//定义W25QXX芯片型号		   

//#define	W25QXX_CS 		PBout(12)  		//W25QXX的片选信号
//#define SPI2_CS_Pin GPIO_Pin_12
//#define SPI2_CS_GPIO_Port GPIOB
//#define W25Qx_Enable() 			GPIO_ResetBits(SPI2_CS_GPIO_Port, SPI2_CS_Pin)
//#define W25Qx_Disable() 		GPIO_SetBits(SPI2_CS_GPIO_Port, SPI2_CS_Pin)
//////////////////////////////////////////////////////////////////////////////

//#define RESET_ENABLE_CMD                     0x66
//#define RESET_MEMORY_CMD                     0x99

//#define W25Q128FV_FSR_BUSY                    ((uint8_t)0x01)    /*!< busy */
//#define W25Q128FV_FSR_WREN                    ((uint8_t)0x02)    /*!< write enable */
//#define W25Q128FV_FSR_QE                      ((uint8_t)0x02)    /*!< quad enable */

//#define W25Qx_OK            ((uint8_t)0x00)
//#define W25Qx_ERROR         ((uint8_t)0x01)
//#define W25Qx_BUSY          ((uint8_t)0x02)
//#define W25Qx_TIMEOUT				((uint8_t)0x03)
////指令表
//#define W25X_WriteEnable		0x06 
//#define W25X_WriteDisable		0x04 
//#define W25X_ReadStatusReg		0x05 
//#define W25X_WriteStatusReg		0x01 
//#define W25X_ReadData			0x03 
//#define W25X_FastReadData		0x0B 
//#define W25X_FastReadDual		0x3B 
//#define W25X_PageProgram		0x02 
//#define W25X_BlockErase			0xD8 
//#define W25X_SectorErase		0x20 
//#define W25X_ChipErase			0xC7 
//#define W25X_PowerDown			0xB9 
//#define W25X_ReleasePowerDown	0xAB 
//#define W25X_DeviceID			0xAB 
//#define W25X_ManufactDeviceID	0x90 
//#define W25X_JedecDeviceID		0x9F 

//void W25QXX_Init(void);
//uint8_t W25QXX_User_Init(void);
//u16  W25QXX_ReadID(void);  	    		//读取FLASH ID
//u8	 W25QXX_ReadSR(void);        		//读取状态寄存器 
//void W25QXX_Write_SR(u8 sr);  			//写状态寄存器
//void W25QXX_Write_Enable(void);  		//写使能 
//void W25QXX_Write_Disable(void);		//写保护
//void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);
//void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead);   //读取flash
//void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);//写入flash
//void W25QXX_Erase_Chip(void);    	  	//整片擦除
//void W25QXX_Erase_Sector(u32 Dst_Addr);	//扇区擦除
//void W25QXX_Wait_Busy(void);           	//等待空闲
//void W25QXX_PowerDown(void);        	//进入掉电模式
//void W25QXX_WAKEUP(void);				//唤醒
//#endif
















