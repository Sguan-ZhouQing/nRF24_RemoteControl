/*
 * @Author: 星必尘Sguan
 * @Date: 2025-04-14 17:01:57
 * @LastEditors: 星必尘Sguan|3464647102@qq.com
 * @LastEditTime: 2025-04-14 17:27:52
 * @FilePath: \test_nRF24L01\Hardware\nRF24L01.c
 * @Description: 
 * 
 * Copyright (c) 2025 by $JUST, All Rights Reserved. 
 */
#include "nRF24L01.h"

extern SPI_HandleTypeDef hspi2;

#define TX_ADR_WIDTH    5     //5字节地址宽度
#define RX_ADR_WIDTH    5     //5字节地址宽度
#define TX_PLOAD_WIDTH  32    //32字节有效数据宽度
#define RX_PLOAD_WIDTH  32    //32字节有效数据宽度


const uint8_t TX_ADDRESS[TX_ADR_WIDTH]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
const uint8_t RX_ADDRESS[RX_ADR_WIDTH]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 


void W_SS(uint8_t BitValue)
{
    HAL_GPIO_WritePin(CSN_Port, CSN_Pin, (GPIO_PinState)BitValue);
} 

void W_CE(uint8_t BitValue)
{
    HAL_GPIO_WritePin(CE_Port, CE_Pin, (GPIO_PinState)BitValue);
} 

uint8_t R_IRQ(void)
{
    return HAL_GPIO_ReadPin(IRQ_Port, IRQ_Pin);
}

uint8_t SPI_SwapByte(uint8_t Byte)
{
    uint8_t rxData;
    HAL_SPI_TransmitReceive(&hspi2, &Byte, &rxData, 1, HAL_MAX_DELAY);
    return rxData;
}

uint8_t NRF24L01_Write_Reg(uint8_t Reg, uint8_t Value)
{
    uint8_t Status;
    
    W_SS(0);
    Status = SPI_SwapByte(Reg);
    SPI_SwapByte(Value);
    W_SS(1);
    
    return Status;
}

uint8_t NRF24L01_Read_Reg(uint8_t Reg)
{
    uint8_t Value;
    
    W_SS(0);
    SPI_SwapByte(Reg);
    Value = SPI_SwapByte(0xFF); // NOP命令用0xFF代替
    W_SS(1);
    
    return Value;
}

uint8_t NRF24L01_Read_Buf(uint8_t Reg, uint8_t *Buf, uint8_t Len)
{
    uint8_t Status;
    
    W_SS(0);
    Status = SPI_SwapByte(Reg);
    while(Len--)
    {
        *Buf++ = SPI_SwapByte(0xFF); // NOP命令用0xFF代替
    }
    W_SS(1);
    
    return Status;
}

uint8_t NRF24L01_Write_Buf(uint8_t Reg, uint8_t *Buf, uint8_t Len)
{
    uint8_t Status;
    
    W_SS(0);
    Status = SPI_SwapByte(Reg);
    while(Len--)
    {
        SPI_SwapByte(*Buf++);
    }
    W_SS(1);
    
    return Status;
}

uint8_t NRF24L01_GetRxBuf(uint8_t *Buf)
{
    uint8_t State;
    
    State = NRF24L01_Read_Reg(STATUS);
    NRF24L01_Write_Reg(nRF_WRITE_REG + STATUS, State);
    
    if(State & RX_OK)
    {
        W_CE(1);
        NRF24L01_Read_Buf(RD_RX_PLOAD, Buf, RX_PLOAD_WIDTH);
        NRF24L01_Write_Reg(FLUSH_RX, 0xFF);
        W_CE(1);
        HAL_Delay(1); // 替换原来的Delay_us(150)
        return 0;
    }
    
    return 1;
}

uint8_t NRF24L01_SendTxBuf(uint8_t *Buf)
{
    uint8_t State;
    
    W_CE(0);
    NRF24L01_Write_Buf(WR_TX_PLOAD, Buf, TX_PLOAD_WIDTH);
    W_CE(1);
    
    while(R_IRQ() == 1);
    
    State = NRF24L01_Read_Reg(STATUS);
    NRF24L01_Write_Reg(nRF_WRITE_REG + STATUS, State);
    
    if(State & MAX_TX)
    {
        NRF24L01_Write_Reg(FLUSH_TX, 0xFF);
        return MAX_TX;
    }
    if(State & TX_OK)
    {
        return TX_OK;
    }
    
    return 0xFF; // NOP用0xFF代替
}

uint8_t NRF24L01_Check(void)
{
    uint8_t check_in_buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t check_out_buf[5] = {0x00};
    
    W_SS(1);
    W_CE(0);
    
    NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, check_in_buf, 5);
    NRF24L01_Read_Buf(nRF_READ_REG + TX_ADDR, check_out_buf, 5);
    
    if((check_out_buf[0] == 0x11) && 
       (check_out_buf[1] == 0x22) && 
       (check_out_buf[2] == 0x33) && 
       (check_out_buf[3] == 0x44) && 
       (check_out_buf[4] == 0x55))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}



void NRF24L01_RT_Init(void) 
{	
	W_CE(0);		  
  	NRF24L01_Write_Reg(nRF_WRITE_REG+RX_PW_P0, RX_PLOAD_WIDTH);
	NRF24L01_Write_Reg(FLUSH_RX, NOP);									
  	NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, (uint8_t*)TX_ADDRESS, TX_ADR_WIDTH);
  	NRF24L01_Write_Buf(nRF_WRITE_REG + RX_ADDR_P0, (uint8_t*)RX_ADDRESS, RX_ADR_WIDTH);   
  	NRF24L01_Write_Reg(nRF_WRITE_REG + EN_AA, 0x01);     
  	NRF24L01_Write_Reg(nRF_WRITE_REG + EN_RXADDR, 0x01); 
  	NRF24L01_Write_Reg(nRF_WRITE_REG + SETUP_RETR, 0x1A);
  	NRF24L01_Write_Reg(nRF_WRITE_REG + RF_CH, 0);        
  	NRF24L01_Write_Reg(nRF_WRITE_REG + RF_SETUP, 0x0F);  
  	NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0F);    
	W_CE(1);									  
}

void NRF24L01_Init(void)
{
	// NRF24L01_Pin_Init();
	while(NRF24L01_Check());
	NRF24L01_RT_Init();
	
}

void NRF24L01_SendBuf(uint8_t *Buf)
{
	W_CE(0);									
	NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0E);   
	W_CE(1);
	// Delay_us(15);
	HAL_Delay(15);
	NRF24L01_SendTxBuf(Buf);						
	W_CE(0);
	NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0F);
	W_CE(1);	
}

uint8_t NRF24L01_Get_Value_Flag(void)
{
	return R_IRQ();
}
