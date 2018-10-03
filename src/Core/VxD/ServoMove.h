//
// ServoMove.h
//

#ifndef _SERVOMOVE_H
#define _SERVOMOVE_H

#define DEFAULT_POS_ERROR	10
#define DEFAULT_VEL_ERROR	30
#define POSVEL_DIVISOR		10

void InitServoConstants(void);

//void fnDriveAvcServoOpenLoop(const SERVO_VALUES* ss);
//void fnCommandAvcServoOpenLoop(const SERVO_VALUES* ss);

void fnCommandOscServoOpenLoop(const OSC_VALUES *ss, int64_t nOscCommand);

void fnCommandVelocityServo(SERVO_VALUES* ss);
void fnCommandAvcServo(SERVO_VALUES* ss);
void fnCommandOscServo(OSC_VALUES *ss);
void fnCommandServo(SERVO_VALUES* ss);

void JogVelocityServo(SERVO_VALUES* ss, int nServoSpeed);

#endif	// _SERVOMOVE_H
