/*
 * SPI.h
 *
 *  Created on: Aug 2, 2018
 *      Author: edwardp
 */
/* Includes ------------------------------------------------------------------*/
#include "global_includes.h"

#ifndef CORE_VXD_SPI_H_
#define CORE_VXD_SPI_H_

/* M25P FLASH SPI Interface pins  */
#define spi2_SPI                           SPI2
#define spi2_SPI_CLK                       RCC_APB1Periph_SPI2
#define spi2_SPI_CLK_INIT                  RCC_APB1PeriphClockCmd

#define spi2_SPI_SCK_PIN                   GPIO_Pin_1
#define spi2_SPI_SCK_GPIO_PORT             GPIOI
#define spi2_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOI
#define spi2_SPI_SCK_SOURCE                GPIO_PinSource1
#define spi2_SPI_SCK_AF                    GPIO_AF_SPI2

#define spi2_SPI_MISO_PIN                  GPIO_Pin_2
#define spi2_SPI_MISO_GPIO_PORT            GPIOI
#define spi2_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOI
#define spi2_SPI_MISO_SOURCE               GPIO_PinSource2
#define spi2_SPI_MISO_AF                   GPIO_AF_SPI2

#define spi2_SPI_MOSI_PIN                  GPIO_Pin_3
#define spi2_SPI_MOSI_GPIO_PORT            GPIOI
#define spi2_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOI
#define spi2_SPI_MOSI_SOURCE               GPIO_PinSource3
#define spi2_SPI_MOSI_AF                   GPIO_AF_SPI2

//#define CS_DAC1_CS_Pin				   GPIO_Pin_2		//Out PE2
#define dac1_CS_PIN                        GPIO_Pin_2
#define dac1_CS_GPIO_PORT                  GPIOE
#define dac1_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOE

//#define CS_DAC1_LOAD_Pin				   GPIO_Pin_3  	//Out PE3
#define ldac1_PIN						   GPIO_Pin_3
#define ldac1_GPIO_PORT                    GPIOE
#define ldac1_GPIO_CLK                     RCC_AHB1Periph_GPIOE

//#define CS_DAC1_CLR_Pin				   GPIO_Pin_4  	//Out PE4
#define clr1_PIN						   GPIO_Pin_4
#define clr1_GPIO_PORT                     GPIOE
#define clr1_GPIO_CLK                      RCC_AHB1Periph_GPIOE

//#define CSAD1_CS_Pin 					   GPIO_Pin_8		//Out PF8
#define adc1_CS_PIN                        GPIO_Pin_8
#define adc1_CS_GPIO_PORT                  GPIOF
#define adc1_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOF

//#define CSAD2_CS_Pin					   GPIO_Pin_9		//Out PF9
#define adc2_CS_PIN                        GPIO_Pin_9
#define adc2_CS_GPIO_PORT                  GPIOF
#define adc2_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOF

//#define CS_DAC2_CS_Pin				   GPIO_Pin_11  	//Out PF11
#define dac2_CS_PIN                        GPIO_Pin_11
#define dac2_CS_GPIO_PORT                  GPIOF
#define adc2_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOF

//#define CS_DAC2_LOAD_Pin			       GPIO_Pin_12  	//Out PF12
#define ldac2_PIN						   GPIO_Pin_12
#define ldac2_GPIO_PORT                    GPIOF
#define ldac2_GPIO_CLK                     RCC_AHB1Periph_GPIOF

//#define CS_DAC2_CLR_Pin				   GPIO_Pin_13  	//Out PF13
#define clr2_PIN						   GPIO_Pin_13
#define clr2_GPIO_PORT                     GPIOF
#define clr2_GPIO_CLK                      RCC_AHB1Periph_GPIOF

/* Exported macro ------------------------------------------------------------*/
/* Select sFLASH: Chip Select pin low */
#define ldac1_LOW()			GPIO_SetBits(ldac1_GPIO_PORT, ldac1_PIN)
#define clr1_LOW()			GPIO_SetBits(clr1_GPIO_PORT, clr1_PIN)
#define ldac2_LOW()			GPIO_SetBits(ldac2_GPIO_PORT, ldac2_PIN)
#define clr2_LOW()			GPIO_SetBits(clr2_GPIO_PORT, clr2_PIN)

/* Deselect sFLASH: Chip Select pin high */
#define ldac1_HIGH()		GPIO_ResetBits(ldac1_GPIO_PORT, ldac1_PIN)
#define clr1_HIGH()			GPIO_ResetBits(clr1_GPIO_PORT, clr1_PIN)
#define ldac2_HIGH()		GPIO_ResetBits(ldac2_GPIO_PORT, ldac2_PIN)
#define clr2_HIGH()			GPIO_ResetBits(clr2_GPIO_PORT, clr2_PIN)

#endif /* CORE_VXD_SPI_H_ */

/* Private function prototypes -----------------------------------------------*/
void Init_SPI(void);

/* Low layer functions */
uint8_t HandshakeSpiByte(uint8_t byte);
uint16_t HandshakeSpiWord(uint16_t HalfWord);
void SelectSpiDevice(uint8_t uDevice);
