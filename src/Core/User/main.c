/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    14-January-2013
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gl_mgr.h"

#include "mod_ethernet.h"
#include "mod_system.h"
#include "mod_console.h"
#include "Weldloop.h"
#include "GPIO.h"
#include "CAN.h"
#include "SPI.h"
#include "dac.h"
#include "adc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Handler_PRIO    		( tskIDLE_PRIORITY  + 1 )
#define Handler_STACK   		( 1024 )

#define Timer_Task_PRIO    		( tskIDLE_PRIORITY  + 2 )
#define Timer_Task_STACK   		( 1024 )

#define Background_Task_PRIO    ( tskIDLE_PRIORITY  + 10 )
#define Background_Task_STACK   ( 768 )

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

xTaskHandle                   Task_Handle;
xTaskHandle                   Timer_Task_Handle;
xTaskHandle                   HandlerTask_Handle;

/* Private function prototypes -----------------------------------------------*/
static void Timer_Task(void * pvParameters);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* Create background task */
	xTaskCreate(Timer_Task, (signed char const*)"TIMER", Timer_Task_STACK,
	        NULL, Timer_Task_PRIO, &Timer_Task_Handle);

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();
}

static void Timer_Task(void * pvParameters)
{
	static portTickType oldticks = 0;
	static portTickType newticks = 0;
	static portTickType deltaticks = 0;

	/* Init Modules manager */
	MOD_PreInit();

	/*Add modules here */
	MOD_AddModule(&mod_ethernet, CONNECTIVITY_GROUP);

	CONSOLE_LOG((LPBYTE)"[SYSTEM] System modules added.");

	/* Show startup Page */
	GL_Startup();

	/* Init Libraries and stack here */
	MOD_LibInit();

	/* Show the main menu and start*/
	GL_Init();

	GL_GotoServer();

	/* Initialize the SPI FLASH driver */
	Init_GPIO();
	Init_SPI();
	Init_ADC();
	Init_DAC();

	//Make sure CAN work when GUI connect to RxData.
	CANinit();

	StartUp();

	/* Create Main 10ms Entry task */
	//HandlerTask_Handle = sys_thread_new("Handler", fnHandler, NULL, Handler_STACK, Handler_PRIO);

	while (1)
	{
		newticks = xTaskGetTickCount();
		deltaticks = newticks - oldticks;

		if (deltaticks >= 10)
		{
			STM_EVAL_LEDToggle(LED1);
			//fnHandler();
			oldticks = newticks;
		}
	}
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
