#include "w25qxx.h"
#include "stm32f103xe.h"
//#include "spi.h"
//#include "delay.h"
//#include "usart.h"


static __IO uint32_t  SPITimeout = SPIT_LONG_TIMEOUT;    
static uint16_t SPI_TIMEOUT_UserCallback(uint8_t errorCode);
uint32_t W25QXX_TYPE=0;
/**
  * @brief  SPI_FLASH³õÊ¼»¯
  * @param  ÎÞ
  * @retval ÎÞ
  */
void SPI_FLASH_Init(void)
{
//  SPI_InitTypeDef  SPI_InitStructure;
//  GPIO_InitTypeDef GPIO_InitStructure;
//	

//	FLASH_SPI_APBxClock_FUN ( FLASH_SPI_CLK, ENABLE );
//	

// 	FLASH_SPI_CS_APBxClock_FUN ( FLASH_SPI_CS_CLK|FLASH_SPI_SCK_CLK|
//																	FLASH_SPI_MISO_CLK|FLASH_SPI_MOSI_CLK, ENABLE );
//	

//  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_CS_PIN;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_Init(FLASH_SPI_CS_PORT, &GPIO_InitStructure);
//	

//  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_SCK_PIN;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//  GPIO_Init(FLASH_SPI_SCK_PORT, &GPIO_InitStructure);


//  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MISO_PIN;
//  GPIO_Init(FLASH_SPI_MISO_PORT, &GPIO_InitStructure);


//  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MOSI_PIN;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//  GPIO_Init(FLASH_SPI_MOSI_PORT, &GPIO_InitStructure);


//  SPI_FLASH_CS_HIGH();

//  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
//  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
//  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
//  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
//  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
//  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
//  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
//  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
//  SPI_InitStructure.SPI_CRCPolynomial = 7;
//  SPI_Init(FLASH_SPIx , &SPI_InitStructure);

//	SPI2->CR1&=0XFFC7;
//	SPI2->CR1|=0x0000;	//设置SPI2速度 
//  SPI_Cmd(FLASH_SPIx , ENABLE);
//	
//	W25QXX_TYPE=SPI_FLASH_ReadID();
}

void SPI_FLASH_SectorErase(uint32_t SectorAddr)
{

  SPI_FLASH_WriteEnable();
  SPI_FLASH_WaitForWriteEnd();

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_SectorErase);

  SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);

  SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);

  SPI_FLASH_SendByte(SectorAddr & 0xFF);

  SPI_FLASH_CS_HIGH();

  SPI_FLASH_WaitForWriteEnd();
}


void SPI_FLASH_BulkErase(void)
{

  SPI_FLASH_WriteEnable();

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_ChipErase);

  SPI_FLASH_CS_HIGH();

  SPI_FLASH_WaitForWriteEnd();
}


void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{

  SPI_FLASH_WriteEnable();

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_PageProgram);

  SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);

  SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);

  SPI_FLASH_SendByte(WriteAddr & 0xFF);

  if(NumByteToWrite > SPI_FLASH_PerWritePageSize)
  {
     NumByteToWrite = SPI_FLASH_PerWritePageSize;
//     FLASH_ERROR("SPI_FLASH_PageWrite too large!"); 
  }


  while (NumByteToWrite--)
  {

    SPI_FLASH_SendByte(*pBuffer);

    pBuffer++;
  }


  SPI_FLASH_CS_HIGH();

  SPI_FLASH_WaitForWriteEnd();
}


void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
  uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
	

  Addr = WriteAddr % SPI_FLASH_PageSize;
	

  count = SPI_FLASH_PageSize - Addr;

  NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;

  NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;
	

  if (Addr == 0)
  {
		/* NumByteToWrite < SPI_FLASH_PageSize */
    if (NumOfPage == 0) 
    {
      SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    { 
      while (NumOfPage--)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }

      SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
    }
  }

  else 
  {
		/* NumByteToWrite < SPI_FLASH_PageSize */
    if (NumOfPage == 0)
    {

      if (NumOfSingle > count) 
      {
        temp = NumOfSingle - count;

        SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
				
        WriteAddr +=  count;
        pBuffer += count;

        SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);
      }
      else 
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
      }
    }
    else 
    {

      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
      NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;
			

      SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
			

      WriteAddr +=  count;
      pBuffer += count;

      while (NumOfPage--)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;
      }

      if (NumOfSingle != 0)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      }
    }
  }
}

void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_ReadData);


  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);

  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);

  SPI_FLASH_SendByte(ReadAddr & 0xFF);
	

  while (NumByteToRead--) /* while there is data to be read */
  {

    *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);

    pBuffer++;
  }


  SPI_FLASH_CS_HIGH();
}

 /**
  * @brief  ¶ÁÈ¡FLASH ID
  * @param 	ÎÞ
  * @retval FLASH ID
  */  
uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
uint32_t SPI_FLASH_ReadID(void)
{



  SPI_FLASH_CS_LOW();


  SPI_FLASH_SendByte(W25X_JedecDeviceID);


  Temp0 = SPI_FLASH_SendByte(Dummy_Byte);


  Temp1 = SPI_FLASH_SendByte(Dummy_Byte);


  Temp2 = SPI_FLASH_SendByte(Dummy_Byte);


  SPI_FLASH_CS_HIGH();


	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;

  return Temp;
}

uint32_t SPI_FLASH_ReadDeviceID(void)
{
  uint32_t Temp = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "RDID " instruction */
  SPI_FLASH_SendByte(W25X_DeviceID);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  
  /* Read a byte from the FLASH */
  Temp = SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  return Temp;
}
/*******************************************************************************
* Function Name  : SPI_FLASH_StartReadSequence
* Description    : Initiates a read data byte (READ) sequence from the Flash.
*                  This is done by driving the /CS line low to select the device,
*                  then the READ instruction is transmitted followed by 3 bytes
*                  address. This function exit and keep the /CS line low, so the
*                  Flash still being selected. With this technique the whole
*                  content of the Flash is read with a single READ instruction.
* Input          : - ReadAddr : FLASH's internal address to read from.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr)
{
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(W25X_ReadData);

  /* Send the 24-bit address of the address to read from -----------------------*/
  /* Send ReadAddr high nibble address byte */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);
}



uint8_t SPI_FLASH_ReadByte(void)
{
  return (SPI_FLASH_SendByte(Dummy_Byte));
}


uint8_t SPI_FLASH_SendByte(uint8_t byte)
{
	uint8_t recvivebyte;
	SPITimeout = SPIT_FLAG_TIMEOUT;
	HAL_SPI_TransmitReceive(&hspi2, &byte, &recvivebyte, 1, SPITimeout);

//	Transmit_return = HAL_SPI_Transmit(&hspi2,&byte,1,SPITimeout);
//	Receive_return = HAL_SPI_Receive(&hspi2,&recvivebyte,1,SPITimeout);
	return recvivebyte;

}

//uint16_t SPI_FLASH_SendHalfWord(uint16_t HalfWord)
//{
//	  SPITimeout = SPIT_FLAG_TIMEOUT;

//  while (SPI_I2S_GetFlagStatus(FLASH_SPIx , SPI_I2S_FLAG_TXE) == RESET)
//	{
//    if((SPITimeout--) == 0) return SPI_TIMEOUT_UserCallback(2);
//   }
//	
//  SPI_I2S_SendData(FLASH_SPIx , HalfWord);

//	 SPITimeout = SPIT_FLAG_TIMEOUT;

//  while (SPI_I2S_GetFlagStatus(FLASH_SPIx , SPI_I2S_FLAG_RXNE) == RESET)
//	 {
//    if((SPITimeout--) == 0) return SPI_TIMEOUT_UserCallback(3);
//   }

//  return SPI_I2S_ReceiveData(FLASH_SPIx );
//}


void SPI_FLASH_WriteEnable(void)
{

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_WriteEnable);

  SPI_FLASH_CS_HIGH();
}

#define WIP_Flag                  0x01


void SPI_FLASH_WaitForWriteEnd(void)
{
  uint8_t FLASH_Status = 0;

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_ReadStatusReg);

  do
  {
    FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);	 
  }
  while ((FLASH_Status & WIP_Flag) == SET);  

  SPI_FLASH_CS_HIGH();
}



void SPI_Flash_PowerDown(void)   
{ 

  SPI_FLASH_CS_LOW();


  SPI_FLASH_SendByte(W25X_PowerDown);


  SPI_FLASH_CS_HIGH();
}   


void SPI_Flash_WAKEUP(void)   
{

  SPI_FLASH_CS_LOW();

  SPI_FLASH_SendByte(W25X_ReleasePowerDown);

  SPI_FLASH_CS_HIGH();
}   
   


static  uint16_t SPI_TIMEOUT_UserCallback(uint8_t errorCode)
{

//  FLASH_ERROR("SPI failed!errorCode = %d",errorCode);
  return 0;
}
   
/*********************************************END OF FILE**********************/



