/*
 * STM324xG-EVAL_callback.c
 *
 *  Created on: Apr 17, 2012
 *      Author: Atollic AB
 */

#include "stm324xg_eval_i2c_ee.h"


/*
 * Callback used by stm324xg_eval_i2c_ee.c.
 * Refer to stm324xg_eval_i2c_ee.h for more info.
 */
__attribute__((weak)) uint32_t sEE_TIMEOUT_UserCallback(void)
{
  /* TODO, implement your code here */
  return sEE_FAIL;
}

/*
 * Callback used by stm324xg_eval_audio_codec.c.
 * Refer to stm324xg_eval_audio_codec.h for more info.
 */
__attribute__((weak)) void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{
  /* TODO, implement your code here */
  return;
}
