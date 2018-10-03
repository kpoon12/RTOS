// DAC.h

#ifndef _DAC_H
#define _DAC_H

#include "spi.h"
					  

// Generic Defines
#define DACDEVICE_MASK	      0x07
#define DACDATA_MASK		  0x0FFF
#define DAC_DEVICES			  0x02
#define DAC_XFER_LENGTH		  0x03

 
// DAC Channels
//
#define PA_AMP_COMMAND			((CURRENT_SERVO_DAC << 4) | CHANNEL_A)
#define PA_GAS_MFC_CMD			((CURRENT_SERVO_DAC << 4) | CHANNEL_B)
#define PA_BOOSTER_VOLT			((CURRENT_SERVO_DAC << 4) | CHANNEL_C)
#define PA_SPARE_OUT1			((CURRENT_SERVO_DAC << 4) | CHANNEL_D)
 

#define PA_CURRENT_CMD_HIGH		((ANALOG_OUTPUT_DAC << 4) | CHANNEL_A)
#define PA_CURRENT_CMD_LOW		((ANALOG_OUTPUT_DAC << 4) | CHANNEL_B)
#define PA_EXTERNAL_TVL_CMD		((ANALOG_OUTPUT_DAC << 4) | CHANNEL_C)
#define PA_EXTERNAL_WIRE_CMD	((ANALOG_OUTPUT_DAC << 4) | CHANNEL_D)

//------------------------------------//
// Input Shift Register Control Bits  //
//------------------------------------//
// Command Selection  
#define WRITE				0x00
#define READ				0x01
#define CLEAR				0x00
#define SET					0x01
#define DISABLE				0x00
#define ISENABLE			0x01

// Register Selection
#define DAC_REG		        0x00
#define OUTPUT_RANGE_REG	0x01
#define POWER_CONTROL_REG	0x02
#define CONTROL_REG			0x03		

// DAC Channel Address
#define CHANNEL_A			0x00
#define CHANNEL_B			0x01
#define CHANNEL_C			0x02
#define CHANNEL_D			0x03
#define CHANNEL_ALL			0x04

// DAC Output Range Register Selection
#define PLUS_5VOLT			0x00
#define PLUS_10VOLT			0x01
#define PLUS_11VOLT			0x02
#define PLUSMINUS_5VOLT     0x03
#define PLUSMINUS_10VOLT    0x04
#define PLUSMINUS_11VOLT    0x05

// DAC Control Register Command Options
#define NOP_CMD				0x00
#define SET_CMD				0x01
#define CLEAR_CMD			0x04
#define LOAD_CMD			0x05

// DAC Control Register Set Command Options

#define ZEROSCALE			0x00
#define MIDSCALE			0x01
 

// DAC Power Control Register Control Bits
#define POWERUP_CHANNEL		 0x01

//------------------------------------//
// Input Shift Register Control Bits  //
//------------------------------------//

typedef union REG_CTRL
{
	BYTE uRegisterControl;
	struct _stRegisterControl
	{
		BYTE uChannelAddr:3;
		BYTE uRegSelect:3;
		BYTE uZero:1;
		BYTE uRead:1;		
	} stRegisterControl;
	
} REG_CTRL;

//------------------------------------//
// Input Shift Register Data Bits  //
//------------------------------------//
typedef union tagREG_DATA
{
	WORD wInputShiftRegData;
	struct _stDacRegData
	{
		WORD wReserved:4;
		WORD wData:12;
	} stDacRegData;
	
	struct _stOutputRangeRegData
	{
		WORD wRange:3;
		WORD wReserved:13;
	} stOutputRangeRegData;
	
	struct _stCtrlRegData
	{
		WORD wSdoDisable:1;
		WORD wClearSelectMode:1;
		WORD wClampEnable:1;
		WORD wTsdEnable:1;
		WORD wReserved:12;	
	} stCtrlRegData;
	
	struct _stPowerCtrlRegData
	{
		WORD wPowerUpChA:1;
		WORD wPowerUpChB:1;
		WORD wPowerUpChC:1;
		WORD wPowerUpChD:1;
		WORD wZero1:1;
		WORD wThermaShutdown:1;
		WORD wZero2:1;
		WORD wChAOverCurr:1;
		WORD wChBOverCurr:1;
		WORD wChCOverCurr:1;
		WORD wChDOverCurr:1;
		WORD wReserved:5;
	} stPowerCtrlRegData;
} REG_DATA;

typedef struct {
	BYTE					uDevice;
	BYTE					uChannel;
	REG_CTRL	InputShiftRegCtrl;
	REG_DATA    InputShiftRegData;
} DAC_DEVICE;

void Init_DAC(void);
void WriteDacData(WORD wChannel, WORD wData);

#endif // _DAC_H

