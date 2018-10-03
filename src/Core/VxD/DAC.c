// DAC.c

/* Includes ------------------------------------------------------------------*/
// Local include files
#include "typedefs.h"
#include "spi.h"
#include "adc.h"
#include "Support.h"

#include "dac.h"

static DAC_DEVICE	stAnalogOutputDevice;
static DAC_DEVICE	stCurrentServoDevice;


static void SetDacOutputRangeReg(DAC_DEVICE *pDacDevice, BYTE uChannel, BYTE uVoltRange)
{	 
	pDacDevice->InputShiftRegCtrl.uRegisterControl = (WORD)(uChannel & 0x07) | (WORD)(OUTPUT_RANGE_REG << 3);
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRegSelect = OUTPUT_RANGE_REG;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRead = WRITE;

	pDacDevice->InputShiftRegData.wInputShiftRegData = 0x0000;
	pDacDevice->InputShiftRegData.stOutputRangeRegData.wRange = uVoltRange;	 
}

static void InitDacControlReg(DAC_DEVICE *pDacDevice)
{	 
	pDacDevice->InputShiftRegCtrl.uRegisterControl = (WORD)(SET_CMD & 0x07) | (WORD)(OUTPUT_RANGE_REG << 3);
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRegSelect = CONTROL_REG;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRead = WRITE;
	
	pDacDevice->InputShiftRegData.wInputShiftRegData = 0x0000;
//	pDacDevice->InputShiftRegData.stCtrlRegData.wSdoDisable = CLEAR;
	pDacDevice->InputShiftRegData.stCtrlRegData.wClearSelectMode = ZEROSCALE;
	pDacDevice->InputShiftRegData.stCtrlRegData.wClampEnable = SET;
//	pDacDevice->InputShiftRegData.stCtrlRegData.wTsdEnable = CLEAR;
}

static void InitDacPowerControlReg(DAC_DEVICE *pDacDevice, BYTE uChannel)
{	 
	pDacDevice->InputShiftRegCtrl.uRegisterControl = (WORD)(POWER_CONTROL_REG << 3);
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uChannelAddr = 0x00;  
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRegSelect = POWER_CONTROL_REG;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uZero = 0;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRead = WRITE;
	
//	pDacDevice->InputShiftRegData.wInputShiftRegData = 0x0000;

	if (CHANNEL_ALL == uChannel)
		pDacDevice->InputShiftRegData.wInputShiftRegData = 0x000F;
	else
		pDacDevice->InputShiftRegData.wInputShiftRegData = (1 << uChannel);
	 
/*
	switch(uChannel)
	{
		case CHANNEL_A:   pDacDevice->InputShiftRegData.stPowerCtrlRegData.wPowerUpChA = 1; break;
		case CHANNEL_B:	  pDacDevice->InputShiftRegData.stPowerCtrlRegData.wPowerUpChB = 1; break;
		case CHANNEL_C:   pDacDevice->InputShiftRegData.stPowerCtrlRegData.wPowerUpChC = 1; break;
		case CHANNEL_D:	  pDacDevice->InputShiftRegData.stPowerCtrlRegData.wPowerUpChD = 1; break;
		case CHANNEL_ALL: pDacDevice->InputShiftRegData.wInputShiftRegData = 0x000F;	    break;
	}
*/
}

static void InitDacDacReg(DAC_DEVICE *pDacDevice, WORD wData, BYTE uChannel)
{	 
	pDacDevice->InputShiftRegCtrl.uRegisterControl = (uChannel & 0x03); //uChannelAddr
	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRegSelect = DAC_REG;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uZero = 0;
//	pDacDevice->InputShiftRegCtrl.stRegisterControl.uRead = WRITE;

	pDacDevice->InputShiftRegData.wInputShiftRegData = 0x0000;
	pDacDevice->InputShiftRegData.stDacRegData.wData = wData;
}

static void SendDacData(DAC_DEVICE *pDacDevice)
{ 

	//SetDacLoadState(pDacDevice, __TRUE);
	for(int i =0; i < 50; i++)
	{
		if(pDacDevice->uDevice == CURRENT_SERVO_DAC)
		{
			ldac1_HIGH();
		}
		else
		{
			ldac2_HIGH();
		}
	}


	SelectSpiDevice(pDacDevice->uDevice);
	
	HandshakeSpiByte(pDacDevice->InputShiftRegCtrl.uRegisterControl);
	HandshakeSpiByte(((pDacDevice->InputShiftRegData.wInputShiftRegData & 0xFF00) >> 8));
	HandshakeSpiByte((pDacDevice->InputShiftRegData.wInputShiftRegData &  0x00FF));
	
	SelectSpiDevice(DESELECT_ALL);	
	//tDacLoadState(pDacDevice, __FALSE);

	if(pDacDevice->uDevice == CURRENT_SERVO_DAC)
	{
		ldac1_LOW();
	}
	else
	{
		ldac2_LOW();
	}
}

static void InitDacOutputRangeReg(DAC_DEVICE *pDacDevice, BYTE uChannel)
{	 
	switch(pDacDevice->uDevice)
	{
		case ANALOG_OUTPUT_DAC:
/*			switch(uChannel)
			{
				case CHANNEL_A: // PA_EXTERNAL_TVL_CMD
				case CHANNEL_C: // PA_EXTERNAL_WIRE_CMD
					SetDacOutputRangeReg(pDacDevice, uChannel,   PLUSMINUS_10VOLT);   
					break;
	
				case CHANNEL_B: // PA_TRAV_ENOUT
				case CHANNEL_D: // PA_WIRE_ENOUT
				case CHANNEL_ALL:
				default:
					SetDacOutputRangeReg(pDacDevice, uChannel,   PLUSMINUS_10VOLT);		 
					break;
			}*/
			SetDacOutputRangeReg(pDacDevice, CHANNEL_ALL, PLUSMINUS_10VOLT);
			break;	

		case CURRENT_SERVO_DAC: // PA_AMP_COMMAND
			SetDacOutputRangeReg(pDacDevice, CHANNEL_ALL, PLUSMINUS_10VOLT);
			break;
	}
}

static void InitDacDevice(DAC_DEVICE *pDacDevice)
{	 
	/* Initialize Control Register */
	InitDacControlReg(pDacDevice);
	SendDacData(pDacDevice);
	
	/* Initialize Power Control Register */
	InitDacPowerControlReg(pDacDevice, CHANNEL_ALL);  
	SendDacData(pDacDevice);
	
	/* Initialize Range Control Register */
	switch(pDacDevice->uDevice)
	{
		case ANALOG_OUTPUT_DAC:
			InitDacOutputRangeReg(pDacDevice, CHANNEL_A); // PA_EXTERNAL_TVL_CMD
			SendDacData(pDacDevice);
			
			InitDacOutputRangeReg(pDacDevice, CHANNEL_B); // PA_TRAV_ENOUT
			SendDacData(pDacDevice);
			
			InitDacOutputRangeReg(pDacDevice, CHANNEL_C); // PA_EXTERNAL_WIRE_CMD
			SendDacData(pDacDevice);
			
			InitDacOutputRangeReg(pDacDevice, CHANNEL_D); // PA_WIRE_ENOUT
			SendDacData(pDacDevice);
			break;
			
		case CURRENT_SERVO_DAC:
			InitDacOutputRangeReg(pDacDevice, CHANNEL_A); // PA_AMP_COMMAND
			SendDacData(pDacDevice);

			InitDacOutputRangeReg(pDacDevice, CHANNEL_B); // PA_A0
			SendDacData(pDacDevice);

			InitDacOutputRangeReg(pDacDevice, CHANNEL_C); // PA_A1
			SendDacData(pDacDevice);

			InitDacOutputRangeReg(pDacDevice, CHANNEL_D); // PA_A4
			SendDacData(pDacDevice);
			break;
			
		default:
			// SHOULD NOT HIT HERE
			break;
	}
	
} 

void WriteDacData(WORD wChannel, WORD wData)
{

		BYTE uDacDevice = (BYTE)(wChannel >> 4);
		wChannel &= DACDEVICE_MASK;
		
		switch(uDacDevice)
		{
			case ANALOG_OUTPUT_DAC:
				stAnalogOutputDevice.uDevice  = uDacDevice;
				stAnalogOutputDevice.uChannel = wChannel;
				InitDacDacReg(&stAnalogOutputDevice, wData, wChannel);
				SendDacData(&stAnalogOutputDevice);
				break;
				
			case CURRENT_SERVO_DAC:
				stCurrentServoDevice.uDevice  = uDacDevice;
				stCurrentServoDevice.uChannel = wChannel;
				InitDacDacReg(&stCurrentServoDevice, wData, wChannel);
				SendDacData(&stCurrentServoDevice);
				break;
				
			default:
				// SHOULD NOT HIT HERE
				break;
		}
		
}

void Init_DAC(void)
{
	/* Deactivate SPI DAC Device Select lines */
	SelectSpiDevice(DESELECT_ALL);
	stCurrentServoDevice.uDevice = CURRENT_SERVO_DAC;  	  
	//SetDacClearState(&stCurrentServoDevice, __FALSE);
	//SetDacLoadState(&stCurrentServoDevice, __FALSE);
	clr2_LOW();
	ldac2_LOW();
	
	/* Initialize Current Servo Device */
	InitDacDevice(&stCurrentServoDevice);
	/* Deactivate SPI DAC Device Select lines */
	SelectSpiDevice(DESELECT_ALL);

	/* Deactivate SPI DAC Device Select lines */
	SelectSpiDevice(DESELECT_ALL);
	stAnalogOutputDevice.uDevice = ANALOG_OUTPUT_DAC;
	//SetDacClearState(&stAnalogOutputDevice, __FALSE);
	//SetDacLoadState(&stAnalogOutputDevice, __FALSE);
	clr1_LOW();
	ldac1_LOW();

	/* Initialize Analog Output Device */
	InitDacDevice(&stAnalogOutputDevice);

}

