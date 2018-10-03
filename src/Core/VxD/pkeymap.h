#ifndef __PKMAP_H__
#define __PKMAP_H__

/************************************************************
----------------------------------------------------------
Outbound data (M327 to Pendant)
----------------------------------------------------------
 M327 will send two 8 bit values
 to be combined into one 16 bit word
 Format shall be as follows:
 D15 ->->->-> D00
 000AAAAAXXXXXXXX
 Legend    AAAAA = Address information
        XXXXXXXX = Character or LED Information
 Address 0x00 thru 0x0F = LCD Character 0 to LCD Charactor 15
 Address 0x1F = LED Register where data = 
 
 #define WELD_MODE_LED			(1<<0)
 #define TEST_MODE_LED			(1<<1)
 #define SEQ_STOP_LED			(1<<2)
 #define NOT_USED_LED			(1<<3)
 #define TVL_MODE_CW_LED		(1<<4)
 #define TVL_MODE_CCW_LED		(1<<5)
 #define WIRE_FEED_MODE_LED		(1<<6)
 #define MAN_PURGE_LED			(1<<7)
----------------------------------------------------------


----------------------------------------------------------
Inbound data (Pendant to M327)
----------------------------------------------------------
 M327 will receive two 8 bit values
 to be combined into one 16 bit word
 Format shall be as follows:

 D15 ->->->-> D00
 AAAAAAAAXXXXXXXX
 Legend AAAAAAAA = Steering Encoder change
 Where AAAA is represented as an 8 bit signed value (char)
 This number will be the number of encoder edges moved over the last 10 milliseconds.

 XXXXXXXX is the keycode of the button pressed.

 Details are listed in the Pendant Key #define section
 */

/////////////////////////////////////////////////////////////
// Pendant Keys
// The following are no KEYS per say, but commands send from the pendant to 
// the AD/IO board
#define PB_PENDANT_RESET    0x10 // Pendant has been reset
#define PB_UNICODE_PENDANT  0x11 // Pendant can accept BITMAPs

#define PB_SPARE_00         0x80 // Key 00
#define PB_MAN_PURGE        0x81 // Key 01
#define PB_WIRE_ON_OFF      0x82 // Key 02
#define PB_DECREMENT        0x83 // Key 03
#define PB_TVL_CCW_JOG      0x84 // Key 04
#define PB_WFD_RT_JOG       0x85 // Key 05
#define PB_AVC_DN_JOG       0x86 // Key 06
#define PB_MANIP_RIGHT      0x87 // Key 07
#define PB_MANIP_DOWN       0x88 // Key 08
#define PB_OSC_MAN          0x89 // Key 09
#define PB_TVL_MODE         0x8A // Key 10
#define PB_INCREMENT        0x8B // Key 11
#define PB_TVL_CW_JOG       0x8C // Key 12
#define PB_WFD_FD_JOG       0x8D // Key 13
#define PB_AVC_UP_JOG       0x8E // Key 14
#define PB_MANIP_UP         0x8F // Key 15

#define PB_MANIP_LEFT       0x90 // Key 16
#define PB_SLOT5_OUT        0x91 // Key 17
#define PB_LDLG_OUT         0x92 // Key 18
#define PB_X_AXIS_POS       0x93 // Key 19
#define PB_Y_AXIS_POS       0x94 // Key 20
#define PB_SPARE_21         0x95 // Key 21
#define PB_SPARE_22         0x96 // Key 22
#define PB_SPARE_23         0x97 // Key 23
#define PB_SPARE_24         0x98 // Key 24
#define PB_SLOT5_IN         0x99 // Key 25
#define PB_LDLG_IN          0x9A // Key 26
#define PB_X_AXIS_NEG       0x9B // Key 27
#define PB_Y_AXIS_NEG       0x9C // Key 28
#define PB_SPARE_29         0x9D // Key 29
#define PB_SPARE_30         0x9E // Key 30
#define PB_SPARE_31         0x9F // Key 31

#define PB_SPARE_32         0xA0 // Key 32
#define PB_AVC_SET          0xA1 // Key 33
#define PB_LAMP             0xA2 // Key 34
#define PB_MAN_ADV          0xA3 // Key 35
#define PB_ALL_STOP         0xA4 // Key 36
#define PB_AVC              0xA5 // Key 37
#define PB_DWL              0xA6 // Key 38
#define PB_WIRE             0xA7 // Key 39
#define PB_PLS_TIME         0xA8 // Key 40
#define PB_PREWRAP          0xA9 // Key 41
#define PB_WELD_TEST        0xAA // Key 42
#define PB_SEQ_START        0xAB // Key 43
#define PB_SEQ_STOP         0xAC // Key 44
#define PB_AMPS             0xAD // Key 45
#define PB_EXC              0xAE // Key 46
#define PB_TVL              0xAF // Key 47

#define PB_AVC_RESP         0xB0 // Key 48
#define PB_ID_NUM           0xB1 // Key 49
#define PB_POSITION_RESET   0xB2 // Key 50
#define PB_SERVO_SELECT     0xB3 // Key 51
#define PB_RIGHT            0xB4 // Key 52
#define PB_LEFT             0xB5 // Key 53
#define PB_ENTER            0xB6 // Key 54
#define PB_CLR              0xB7 // Key 55
#define PB_LBRY             0xB8 // Key 56

// Softkeys
#define PB_OSC_SET 			0xB9 



/*
// FROM M415 PKMAP.H
#define PB_SPARE_00         0x30 // Key 00
#define PB_MAN_PURGE        0x31 // Key 01
#define PB_WIRE_ON_OFF      0x32 // Key 02
#define PB_DECREMENT        0x33 // Key 03
#define PB_TVL_CCW_JOG      0x34 // Key 04
#define PB_WFD_RT_JOG       0x38 // Key 05
#define PB_AVC_DN_JOG       0x36 // Key 06
#define PB_MANIP_RIGHT      0x37 // Key 07
#define PB_MANIP_DOWN       0x38 // Key 08
#define PB_OSC_MAN          0x39 // Key 09

#define PB_TVL_MODE         0x3A // Key 10
#define PB_INCREMENT        0x3B // Key 11
#define PB_TVL_CW_JOG       0x3c // Key 12
#define PB_WFD_FD_JOG       0x3D // Key 13
#define PB_AVC_UP_JOG       0x3E // Key 14
#define PB_MANIP_UP         0x3F // Key 15
#define PB_MANIP_LEFT       0x40 // Key 16
#define PB_SLOT5_OUT        0x41 // Key 17
#define PB_LDLG_OUT         0x42 // Key 18
#define PB_X_AXIS_POS       0x43 // Key 19

#define PB_Y_AXIS_POS       0x44 // Key 20
#define PB_SPARE_21         0x45 // Key 21
#define PB_SPARE_22         0x46 // Key 22
#define PB_SPARE_23         0x47 // Key 23
#define PB_SPARE_24         0x48 // Key 24
#define PB_SLOT5_IN         0x49 // Key 25
#define PB_LDLG_IN          0x4A // Key 26
#define PB_X_AXIS_NEG       0x4B // Key 27
#define PB_Y_AXIS_NEG       0x4C // Key 28
#define PB_SPARE_29         0x4D // Key 29

#define PB_SPARE_30         0x4E // Key 30
#define PB_SPARE_31         0x4F // Key 31
#define PB_SPARE_32         0x50 // Key 32
#define PB_AVC_SET          0x51 // Key 33
#define PB_LAMP             0x52 // Key 34
#define PB_MAN_ADV          0x53 // Key 35
#define PB_ALL_STOP         0x54 // Key 36
#define PB_AVC              0x55 // Key 37
#define PB_DWL              0x56 // Key 38
#define PB_WIRE             0x57 // Key 39

#define PB_PLS_TIME         0x58 // Key 40
#define PB_PREWRAP          0x59 // Key 41
#define PB_WELD_TEST        0x5A // Key 42
#define PB_SEQ_START        0x5B // Key 43
#define PB_SEQ_STOP         0x5C // Key 44
#define PB_AMPS             0x5D // Key 45
#define PB_EXC              0x5E // Key 46
#define PB_TVL              0x5F // Key 47
#define PB_AVC_RESP         0x60 // Key 48
#define PB_ID_NUM           0x61 // Key 49

#define PB_POSITION_RESET   0x62 // Key 50
#define PB_SERVO_SELECT     0x63 // Key 51
#define PB_RIGHT            0x64 // Key 52
#define PB_LEFT             0x65 // Key 53
#define PB_ENTER            0x66 // Key 54
#define PB_CLR              0x67 // Key 55
#define PB_LBRY             0x68 // Key 56
*/

#define NUM_MAPPED_PENDANT_KEYS 57

/////////////////////////////////////////////////////////////
// FATORY AUTOMATION JOG KEYS
// The following list of commands
// Are used ONLY for Factory Automation jogs.
// The jog keys defined above are physical keys on the 
// Handheld Operators Pendant
/////////////////////////////////////////////////////////////
// FA Jog Command format
// Command + ServoSlot + Direction + Run
// D0 - 1 = RUN, 0 = OFF
// D1 - 0 = FORWARD, 1 = REVERSE
// Note if the RUN/Stop bit (D0) is set to off, direction (D1) is ignored
// D2,D3,D4 = Zero Based Servo Slot number
// Constants
// D5 = 0
// D6 = 1
// D7 = 1
// Servo Number is the SLOT that the servo sits in 
// Servo Number 0 = Slot A0  Typically: TRAVEL
// Servo Number 1 = Slot A1  Typically: WIREFEED
// Servo Number 2 = Slot B0  Typically: AVC
// Servo Number 3 = Slot B1  Typically: OSC
// Servo Number 4 = Slot C0  Typically: Extra Servo #1
// Servo Number 5 = Slot C1  Typically: Extra Servo #2
// Servo Number 6 = Slot D0  Typically: Extra Servo #3
// Servo Number 7 = Slot D1  Typically: Extra Servo #4
/////////////////////////////////////////////////////////////


#define PB_SERVO1_OFF	0xC0 		// 0x1100:0000   
#define PB_SERVO1_FWD	0xC1 		// 0x1100:0001
#define PB_SERVO1_REV	0xC3 		// 0x1100:0011

#define PB_SERVO2_OFF	0xC4 		// 0x1100:0100
#define PB_SERVO2_FWD	0xC5 		// 0x1100:0101
#define PB_SERVO2_REV	0xC7 		// 0x1100:0111

#define PB_SERVO3_OFF	0xC8 		// 0x1100:1000
#define PB_SERVO3_FWD	0xC9 		// 0x1100:1001
#define PB_SERVO3_REV	0xCB 		// 0x1100:1011

#define PB_SERVO4_OFF	0xCC 		// 0x1100:1100
#define PB_SERVO4_FWD	0xCD 		// 0x1100:1101
#define PB_SERVO4_REV	0xCF 		// 0x1100:1111

#define PB_SERVO5_OFF	0xD0 		// 0x1101:0000
#define PB_SERVO5_FWD	0xD1 		// 0x1101:0001
#define PB_SERVO5_REV	0xD3 		// 0x1101:0011

#define PB_SERVO6_OFF	0xD4 		// 0x1101:0100
#define PB_SERVO6_FWD	0xD5 		// 0x1101:0101
#define PB_SERVO6_REV	0xD7 		// 0x1101:0111

#define PB_SERVO7_OFF	0xD8 		// 0x1101:1000
#define PB_SERVO7_FWD	0xD9 		// 0x1101:1001
#define PB_SERVO7_REV	0xDB 		// 0x1101:1011

#define PB_SERVO8_OFF	0xDC 		// 0x1101:1100
#define PB_SERVO8_FWD	0xDD 		// 0x1101:1101
#define PB_SERVO8_REV	0xDF 		// 0x1101:1111


// Defs for other stuff
#define PENDANT_LED_WIDTH		16
#define NUM_LED_STRS			16
#define PENDANT_BITMAP_WIDTH	132

// Definitions for Library and ID# handling
#define PENDANT_MODE_NORMAL		0
#define PENDANT_MODE_LIB		1
#define PENDANT_MODE_ID			2

#define PENDANT_SCROLL_STOP		0
#define PENDANT_SCROLL_UP		1
#define PENDANT_SCROLL_DOWN		2

typedef struct tagPD_PENDANTINFO {
	char LibStrings[NUM_LED_STRS][PENDANT_LED_WIDTH];
	char IDStrings[NUM_LED_STRS][PENDANT_LED_WIDTH];
} PD_PENDANTINFO, *LPPD_PENDANTINFO;

#endif // __PKMAP_H__
