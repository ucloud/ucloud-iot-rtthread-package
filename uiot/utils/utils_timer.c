/*
* Copyright (C) 2012-2019 UCloud. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#ifdef __cplusplus
extern "C" {
#endif
    
#include "utils_timer.h"
    
bool has_expired(Timer *timer) {
    return HAL_Timer_Expired(timer);
}

void countdown_ms(Timer *timer, unsigned int timeout_ms) {
    HAL_Timer_Countdown_ms(timer, timeout_ms);
}

void countdown(Timer *timer, unsigned int timeout) {
    HAL_Timer_Countdown(timer, timeout);
}

int left_ms(Timer *timer) {
    return HAL_Timer_Remain_ms(timer);
}

void init_timer(Timer *timer) {
    HAL_Timer_Init(timer);
}
    
#ifdef __cplusplus
}
#endif
