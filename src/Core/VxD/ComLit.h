#ifndef __AMI_COMLIT_H__
#define __AMI_COMLIT_H__
//#define CURRENT_VERSION_NUMBER	   (299 - 2)  	// TEST VERSION CHECKING
#define CURRENT_VERSION_NUMBER	 360
#define CURRENT_BUILD_NUMBER	1725

#include "M327.h"
#include "typedefs.h"

// System Options
#define AAO_ENABLED_OPTION	 ((DWORD)0x01 << 0) // 0x01
#define ENSA_ENABLED_OPTION	 ((DWORD)0x01 << 1) // 0x02
#define CB_ENABLED_OPTION	 ((DWORD)0x01 << 2) // 0x04
#define NO_DAS_OPTION	     ((DWORD)0x01 << 3) // 0x08
#define PING_ENABLED_OPTION  ((DWORD)0x01 << 4) // 0x10	Testing Only

#define STARTING_VERSION		   290
#define THIS_VERSION_NUMBER		  (CURRENT_VERSION_NUMBER - STARTING_VERSION)  

typedef struct tagVERSIONINFO
{
	float fPowerSupplyVersion;
	float fRemoteWeldingVersion;
} VERSIONINFO;

typedef struct tagCB_Faults
{
    BYTE uColumn;
    BYTE uBoom;
    BYTE uRotation;
	BYTE uCommStatus;
} CB_FAULTS;

typedef struct tagCB_AXISFEEDBACK
{
    float fReadPosition;
    float fPastPosition;
    float fCurrentPosition;
    int   nCommandDac;
} CB_AXISFEEDBACK;

typedef struct tagCB_FEEDBACK
{
    CB_AXISFEEDBACK stColumnUpload;
    CB_AXISFEEDBACK stBoomUpload;
    CB_AXISFEEDBACK stRotationUpload;
} CB_FEEDBACK;

typedef struct tagPOSITION_RESET_COMMAND
{
    BYTE uServoIndex;
    BYTE uServoSlot;
    BYTE uHeadPostionValues;
    BYTE uPadding;  // Required for DWORD boundry alignment
    int nPosition;
} POSITION_RESET_COMMAND;

// dwVxdFlags bitwise flags
//#define W32IF_QUEUEINUSE			(1 <<  0)
#define W32IF_MSG_FLAG_LIB_STR		(1 <<  1)
#define W32IF_MSG_FLAG_ARC_ON		(1 <<  2)

#ifndef WM_USER
#define WM_USER                         0x0400
#endif // WM_USER 

// HandHeld Operator Pendant Messages
#define SCHEDULE_NAME_SENT          0
#define SCHEDULE_STRUCTURE_SENT     1

// System Status Codes
#define SERVOTAB_FAULT     (1 << 0)
#define SYSTEMTAB_FAULT    (1 << 1)
#define SUPPORTTAB_FAULT   (1 << 2)
#define USERTAB_FAULT      (1 << 3)
#define MSTSLVTAB_FAULT    (1 << 4)
#define MACHINE_FAULT      (SERVOTAB_FAULT | SYSTEMTAB_FAULT | SUPPORTTAB_FAULT | USERTAB_FAULT)

// Choices for WM_CLEARALLFAULTS
#define INTERNAL_COMMAND	100  // Internally generated Clear Faults command
#define EXTERNAL_COMMAND	200  // Externally generated Clear Faults (MS_CLEAR_FAULTS)
#define FORCE_SHOW            1  // Used to force display of the Status Screen

enum _M327_MESSAGES
{
	WM_CLEARALLFAULTS = (WM_USER + 100),
	WM_SETACTIVEDISPLAY,
	WM_SCHED_DCLICK,			// schedule double-clicked
	WM_LAUNCHEASYSTART,
	WM_SAVEWELDSCHEDULE,
	WM_DISPATCH_WELDSCHEDULE,
	WM_VIEW_STATUSSCREEN,
	WM_INSEQVALUECHANGED,
	WM_CARD_INSERTED,
	WM_CARD_REMOVED,
	WM_CARD_LOGIN_AVAILABLE,
	WM_SETSTATUSTEXT,
	WM_PENDANT_UP,
	WM_PENDANT_ACTION,
	WM_PENDANT_SETCHECK,
	WM_PENDANT_ENBFUNCTION,
	WM_PENDANT_POSITION,
	WM_GOTFAULTS,				// Status Screen messages
	WM_UNIT_MEASURE_CHANGE,		// System Setup Messages
	WM_NEED_RESTART,
	WM_CLOSE_WELDCALC,			// Weld Calculator Messages
	WM_DA_WRITE_FINISHED,		// Data Aqi. Messages
	WM_EDIT_DATAAQI,			// Data Aqi. Messages
	WM_SCHED_SELECTION,			// HandHeld Operator Pendant Messages
	WM_CAL_DATA,				// GainDac calibration data
	WM_OFFSETGAIN_DATA,
	WM_HEAD_POSITION,
	WM_RELOADWELDSCHEDULE,
	WM_PERFORM_START,
	WM_LAUNCH_SERVER,
	FA_CONNECT,
	FA_DATAREADY,
	FA_CLOSED,
	WM_CAL_CONSTANTS,			// Used for Open Loop Servo Positon and Velocity constants
    WM_WDC_MSG,
	WM_EDIT_PROJECTDATA,
	APPBAR_CALLBACK, 			// Special On-Screen Pendant Application Bar Message
	WM_ENSA_MSG,
	WM_PROJECTDATA_UPDATE,
	WM_SENDSERVO_FILE
};

typedef enum tagWDC_t
{
    WDC_CONNECTED = 0x01,
    WDC_DISCONNECTED,
    WDC_MSG_IN,
    WDC_CWND,
    WDC_CONNECTION_ERR,
    WDC_STORE_REPLY_ACK,
    WDC_STORE_REPLY_NAK,
    WDC_STORE_REPLY_TIMEOUT
} WDC_t;


typedef enum tagENSA_t
{
    ENSA_SERVER_CONNECTED = 0x01,
    ENSA_SERVER_DISCONNECTED,
    ENSA_LISTEN_CONNECTED,
    ENSA_LISTEN_DISCONNECTED,
    ENSA_MSG_IN,
    ENSA_CONNECTION_ERR,
} ENSA_t;

// WM_SETACTIVEDISPLAY WPARAM messages
enum _SETACTIVEDISPLAY_MESSAGES
{
	ID_LIBRARY_ACTIVE = 101,
	ID_LIBRARY_INACTIVE,
	ID_STATUSSCREEN_ACTIVE,
	ID_STATUSSCREEN_INACTIVE,
	ID_WELDSCREEN_ACTIVE,
	ID_WELDSCREEN_INACTIVE,
	ID_PENDANTS_ACTIVE,
	ID_PENDANTS_INACTIVE
};


typedef struct tagSetPosition
{
	BYTE uServoSlot;
	int  nPosition;
} SET_POSITION;


/////////////////////////////////////////////
// GUI to Server messages
#define WELD_XMIT_START			0x51
#define NUM_LEVELS				0x52
#define SCHED_PARMS_DATA		0x53
#define ONE_LEVEL_DATA			0x54
#define LEVEL_DATA				0x55
#define WELD_HEAD_DATA			0x56
#define WELD_XMIT_DONE			0x57
#define WELDCMD_TESTMODE		0x58
#define WELDCMD_MANPRGE			0x59
#define WELDCMD_WIREOFF			0x5A
#define WELDCMD_MANADV			0x5B
#define SET_WELD_LEVEL			0x5C
#define WELDCMD_LAMPOFF			0x5D
#define WELDCMD_OSCMAN			0x5E
#define WELDCMD_TVLDIR			0x5F
#define RETURN_TO_HOME			0x60
#define CHECK_GROUND_FAULT		0x61
#define WELDCMD_SET				0x62
#define WELDCMD_SEQSTART		0x63
#define WELDCMD_SEQSTOP			0x64
#define WELDCMD_ALLSTOP			0x65
#define WELDCMD_PWRAP			0x66
#define AMI_CLEAR_FAULTS		0x67
#define AMI_SET_WELDSCHED       0x68
#define WRITE_LIB_STRINGS		0x69
#define WRITE_LED_STRING		0x6A
#define SERVO_CONSTANTS			0x6B
#define SINGLE_MULTILEVEL_DATA	0x6C
#define CMD_HEARTBEAT			0x6D
#define SCHED_DATAAQI_DATA		0x6E
#define WELDCMD_SERVOJOG		0x6F
#define WELDCMD_MANIPJOG		0x70
#define POSITION_RESET			0x71
#define WELDCMD_SVRCHG			0x72
#define ONE_LEVEL_UPDATE		0x73
#define GAIN_CALIBRATION		0x74
#define OFFSETGAIN_LOWER		0x75
#define OFFSETGAIN_UPPER		0x76
#define NO_SCHED_LOADED			0x77
#define SET_LAMP_TIME			0x78
#define SET_HOME_DIRECTION		0x79
#define SET_POSTPURGEHOME		0x7A
#define SET_BOOSTER_VOLTAGE 	0x7B
#define SET_FAULT_ENABLES		0x7C
#define SET_AUXLINE_FUNCTION	0x7D
#define ENABLE_OSCILLATOR	    0x7E
#define OSC_SPEEDSTEER          0x7F
#define XMIT_SERVODEF_START		0x80
#define XMIT_ONESERVODEF		0x81
#define XMIT_SERVODEF_DONE		0x82
#define DIGITAL_OUTPUT_TEST		0x83
#define ANALOG_OUTPUT_TEST		0x84
#define RF_TEST					0x85
#define WRITE_PDBITMAP			0x86
#define OSC_LIMIT_DATA			0x87
#define SET_LEVEL_TIME			0x88
#define GUI_CONNECTED           0x89
#define WM_AVC_POSTION			0x8A
#define CROSSSEAM_SLEWTO        0x8B
#define VIRT_POSITION_RESET     0x8C
#define CB_IN_USE				0x8D
#define SET_AUXOUT_FUNCTION     0x8E
#define READYTOWELD_TRUE        0x8F
#define READYTOWELD_FALSE       0x90
#define LAST_COMMAND			READYTOWELD_FALSE

/////////////////////////////////////////////
// Client to Server Travel Directions
#define AMI_PK_TVLDIR_CW		1	// Travel direction clockwise
#define AMI_PK_TVLDIR_CCW		2	// ---------------- counter cw
#define AMI_PK_TVLDIR_OFF		3	// ---------------- off

/////////////////////////////////////////////
// Server to Client Message Packet Types
#define SCM_MODBUS_FEEDBACK			0xCD // Flag for sending feedback structure from Column, Boom, rotator interface
#define SCM_MODBUS_FAULT			0xCC // Flag for sending faults structure from Column, Boom, rotator interface
#define SCM_MODBUS_COMMAND			0xCB // Flag for sending commands to the Column, Boom, rotator interface
#define SCM_FEEDBACK_BURST          0xCA // Flag indicating feedback data is ready to be read
#define SCM_SERVER_VERSION          0xC9 // DWORD Data Contains Server Version
#define SCM_TIME_UPDATE 			0xC8 // request may come from remote software via factory automation channel
#define SCM_LONGINFO				0xC7 // DWORD Data
#define SCM_LIBRARY_STRING_UPDATE	0xC6 // Lib/ID str 
#define SCM_FAULT_UPDATE			0xC5 // Fault data
#define SCM_PENDANT_UPDATE			0xC4 // Pendant data
#define SCM_HEATINPUT_UPDATE		0xC3 // HeatInput data
#define SCM_BARGRAPH_UPDATE			0xC2 // Bargraph data
#define SCM_ARCINFO_UPDATE			0xC1 // ARC ON Info


#ifndef EV_WELDPHASE_PREPURGE
#define EV_WELDPHASE_PREPURGE    0xD0
#define EV_WELDPHASE_UPSLOPE     0xD1
#define EV_WELDPHASE_LEVELWELD   0xD2
#define EV_WELDPHASE_DOWNSLOPE   0xD3
#define EV_WELDPHASE_POSTPURGE   0xD4
#define EV_WELDPHASE_EXIT        0xD5
#define EV_ARCSTART_COMPLETE     0xD6
#define EV_HOME_COMPLETE         0xD7
#define EV_SET_POT_COMPLETE      0xD8
#define EV_LEVELCHANGE			 0xD9
#define EV_LIB_SELECTION		 0xDA // Passes data
#define EV_ID_SELECTION			 0xDB // Passes data
#define EV_STOPPED_ON_FAULT		 0xDC
#define EV_TVL_MODE_CCW			 0xDD
#define EV_TVL_MODE_OFF			 0xDE
#define EV_TVL_MODE_CW			 0xDF
#define EV_BADSTART				 0xE0
#define EV_HEARTBEAT			 0xE1
#define EV_WELDPHASE_SEQSTART	 0xE2

#define SERVO_UNLOCK		0x01
#define SERVO_LOCK			0x02
#define FAULT_SWITCH1		0x03
#define FAULT_SWITCH2		0x04
#define FAULT_SWITCH3		0x05
#define FAULT_SWITCH4		0x06
#define LAST_SERVO_EVENT	0x07
#endif


// end of no-change section
#define EV_OSCSTEERCHANGE	     0xE4
#define EV_SERVOLOADED		     0xE5
#define EV_DASTREAM_ON			 0xE6
#define EV_DASTREAM_OFF			 0xE7
#define EV_STOPPED_BY_USER		 0xE8



// Literals used with EV_LEVELCHANGE 
// to indicate the direction change
#define LEVEL_RST	0x00
#define LEVEL_INC	0x01
#define LEVEL_DEC	0x02


/////////////////////////////////////////////
// Server to Client BAR_GRAPH messages and Data Aqi
#define BG_SLOT0		0
#define BG_SLOT1		1
#define BG_SLOT2		2
#define BG_SLOT3		3
#define BG_SLOT4		4
#define BG_SLOT5		5
#define BG_SLOT6		6
#define BG_SLOT7		7
#define BG_SLOT8		8
#define BG_GAS			9
#define BG_LEVEL_TIME	10
#define BG_EVENTS		11
#define BG_HEAD_POS		12
#define BG_ELAPSED_TIME	13
#define BG_BANK_VOLTS	14
#define BG_HEATINPUT	15
#define BG_INPUTCHANNELS	16

#define CHANNEL_MESSAGE_POS     28

// Data Aqi EVENTS will be sent along with bargraph information
// the format will be as follows
// "aaaabbbbcccccccc" where:
// "aaaa************" = BG_EVENTS
// "****bbbb********" = the servo number
// "********cccccccc" = the actual event number
// For Servo Events the upper 4 bits of the 12 will 
// represent the SERVO NUMBER, not the slot it is installed in.
//
// For events not specific to any servo
// the servo number will be set to all 1's

/////////////////////////////////////////////
// Server to Client LONGINFO messages

// Long Info format will be as follows
// "aaaaccc...ccc" where:
// "aaaa***...***" = LI_EVENTS
// "****ccc...ccc" = the actual LongInfo data 2^27 + 2^28 for sign

////////////////////////////////////////////////////////////////////////////////
// Override commands
#define NO_OVERRIDE_SELECTED	0
#define OVRD_PAMPS				1
#define OVRD_BAMPS				2
#define OVRD_PAVC				3
#define OVRD_BAVC				4
#define OVRD_OSCAMP				5
#define OVRD_EXCTIME			6
#define OVRD_INDELL				7
#define OVRD_OUTDWELL			8
#define OVRD_PTVL				9
#define OVRD_BTVL				10
#define OVRD_PWIREF				11
#define OVRD_BWIREF				12
#define OVRD_AVCRESP			13
#define OVRD_PPLSTIME			14
#define OVRD_BPLSTIME			15
#define OVRD_TVLMODE			16

////////////////////////////////////////////////////////////////////////
// M327 GUI to AD/IO BootLoader constants

#define MAX_PACKET_SIZE					64
#define LOADER_PORT						900

// from FW to UI
#define _szComConst_LoaderReady 		"R"
#define _szComConst_LdrErr_CSFailed 	"0"
#define _szComConst_LdrErr_NoLineStart 	"1"
#define _szComConst_LdrErr_RecType2 	"2"
#define _szComConst_LdrErr_RecType3 	"3"
#define _szComConst_LdrErr_PacketTooBig "4" 
#define _szComConst_LdrErr_MemCheck 	"5" 
#define _szComConst_LdrErr_RecTypeU 	"6" 
#define _szComConst_LdrErr_RecType4 	"7"
#define _szComConst_EOF 				"E"


//from UI to FW
#define _szComConst_ResetLoad  "FSI_RESET_LOAD"
#define _szComConst_StartLoad  "FSI_START_LOAD"
#define _szComConst_Version    "FSI_VERSION"
#define _szComConst_ExecuteApp "FSI_EXE_APP"   

/////////////////////////////////////////////////////////////
// ENSURE ALL OF THE FOLLOWING STURCTURES ARE BYTE ALIGNED //
/////////////////////////////////////////////////////////////
#pragma pack(1)

#define RT_MESSAGE_SIZE       256
#define MAX_QUEUE_SIZE (RT_MESSAGE_SIZE - 2)   // one byte for number of message and one byte for queue type

typedef struct tagQUEUEDATA {
	BYTE  uNumBytes;
	BYTE  QueueID[ MAX_QUEUE_SIZE ];
} QUEUEDATA, *LPQUEUEDATA;

#define MAX_NUM_LONGINFO (MAX_QUEUE_SIZE / sizeof(DWORD))     // 4 bytes per message
typedef struct tagLONGINFO {
	BYTE  uNumBytes;
	DWORD dwLongInfoID[MAX_NUM_LONGINFO];
} LONGINFO, *LPLONGINFO;

#define MAX_NUM_BARGRAPHS (MAX_QUEUE_SIZE / sizeof(DWORD))    // 4 bytes per message
typedef struct tagBARGRAPH {
	BYTE  uNumBytes;
	DWORD dwBarGraphID[MAX_NUM_BARGRAPHS];
} BARGRAPH;

#define MAX_NUM_HEATINPUTS (MAX_QUEUE_SIZE / sizeof(WORD))    // 2 bytes per message
typedef struct tagHEATINPUTMSG 
{
	BYTE  uNumHeatInputs;
	WORD  wHeatInputID[MAX_NUM_HEATINPUTS];
} HEATINPUTMSG;

#define MAX_NUM_FAULTS MAX_QUEUE_SIZE
typedef struct tagFAULTDATA 
{
	BYTE uNumBytes;
	BYTE FaultID[MAX_NUM_FAULTS];
} FAULTDATA, *LPFAULTDATA;

#define MAX_NUM_PENDANT MAX_QUEUE_SIZE
typedef struct tagPENDANTDATA {
	BYTE uNumBytes;
	BYTE PendantID[MAX_NUM_PENDANT];
} PENDANTDATA, * LPPENDANTDATA;

typedef struct tagPENDANTSELECTION 
{
	BYTE uNumBytes;	//  Number of Bytes (always 0 or 16 + 2)
	BYTE uSelectedString[20];
} PENDANTSELECTION, *LPPENDANTSELECTION;

typedef struct tagARCWRAPPER 
{
	BYTE uNumBytes;	
	WORD uArcOn;
} ARCWRAPPER, *LPARCWRAPPER;

typedef struct tagWELDCOMMAND 
{
	WORD   uCommand;
	WORD   wLength;
	BYTE   uByteData[64];
} WELDCOMMAND, *LPWELDCOMMAND;
#define LPCWELDCOMMAND const WELDCOMMAND *

// GUI to AD/IO Servo File Structure

// File Header Information
typedef struct tagTS_HEADERINFO
{
	BYTE uServoIndex;
	BYTE uServoSlot;
	WORD wFileLength;
} TS_HEADERINFO, *LPTS_HEADERINFO;
#define LPCHEADERINFO const HEADERINFO *

// Servo File Structure Handshake packet
#define TS_BUFFER_SIZE	64
typedef struct tagTS_TRANSFERPACKET
{
	BYTE uThisBlock;
	BYTE uLastBlock;
	BYTE uBuffer[TS_BUFFER_SIZE];
} TS_TRANSFERPACKET, *LPTS_TRANSFERPACKET;
#define LPCTS_TRANSFERPACKET const TS_TRANSFERPACKET *

#define TS_BUFFERSIZE	4096
typedef struct tagTS_FILEBUFFER
{
	BYTE uServoIndex;
	BYTE uServoSlot;
	WORD wFileLength;
	BYTE uBuffer[TS_BUFFERSIZE];
} TS_FILEBUFFER, *LPTS_FILEBUFFER;
#define LPCTS_FILEBUFFER const TS_FILEBUFFER *

#define BITMAP_ARRAY_SIZE  128
typedef struct tagPD_BITMAP 
{
	BYTE   uXpos;
	BYTE   uYpos;
	BYTE   uWidth;
	BYTE   uCheckSum;
	WORD   wArray[BITMAP_ARRAY_SIZE];
} PD_BITMAP;

#pragma pack() // RETURN TO NORMAL / DEFAULT PACKING

typedef struct tagJOG_STRUCT {
	BYTE uServo;  // The number of the servo
	signed char uSpeed;  // The speed and direction to jog -100 to +100
} JOG_STRUCT, * LPJOG_STRUCT;
#define LPC_JOG_STRUCT const JOG_STRUCT *

typedef struct tagJOG_COMMAND 
{
	BYTE uServo;    // The number of the servo
	char cSpeed;    // The speed and direction to jog -100 to +100
    WORD wDuration; // The amount of time to execute the Jog Command Note: 0 means forever
} JOG_COMMAND;

// End of pendant message definitions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Start of AC Mains Structures

// AD/IO to AC Mains Commands
#define	SEND_ACMAIN_VOLTAGES		(BYTE)(1 << 0)
#define	SEND_ACMAIN_CURRENTS		(BYTE)(1 << 1)
#define	SEND_ACMAIN_PHASES			(BYTE)(1 << 2)
#define	SEND_ACMAIN_TEMP			(BYTE)(1 << 3)
#define	SEND_ACMAIN_STATUS			(BYTE)(1 << 4)
#define	SEND_ACMAIN_ALL				(SEND_ACMAIN_VOLTAGES | SEND_ACMAIN_CURRENTS | SEND_ACMAIN_PHASES | SEND_ACMAIN_TEMP | SEND_ACMAIN_STATUS)
#define RECEIVED_ACMAIN_COMMAND		0x96

#define PHASE1_ACTIVE	(1 << 0)
#define PHASE2_ACTIVE	(1 << 1)
#define PHASE3_ACTIVE	(1 << 2)

typedef struct tagACMAIN_DATA {
	WORD   wVoltage[3];
	WORD   wAmps[3];
	short  nTemperature[8];
	BYTE   uPhases;
	BYTE   uStatus;
} ACMAIN_DATA, *LPACMAIN_DATA;
#define LPCACMAIN_DATA const ACMAIN_DATA *


// End of AC Mains Structures
//////////////////////////////////////////////////////////////////////



#endif
