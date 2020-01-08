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

#ifndef C_SDK_HAL_TIMER_PLATFORM_H_
#define C_SDK_HAL_TIMER_PLATFORM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <rtthread.h>


/**
 * definition of the Timer struct. Platform specific
 */
struct Timer {
    rt_tick_t end_time;
};

#ifdef __cplusplus
}
#endif
#endif //C_SDK_HAL_TIMER_PLATFORM_H_
