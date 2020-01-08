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

#ifndef C_SDK_UTILS_TIMER_H_
#define C_SDK_UTILS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif
    
// Add the platform specific timer includes to define the Timer struct
#include "uiot_import.h"

/**
 * @brief Check if a timer is expired
 *
 * Call this function passing in a timer to check if that timer has expired.
 *
 * @param timer - pointer to the timer to be checked for expiration
 * @return bool - true = timer expired, false = timer not expired
 */
bool has_expired(Timer *timer);

/**
 * @brief Create a timer (milliseconds)
 *
 * Sets the timer to expire in a specified number of milliseconds.
 *
 * @param timer - pointer to the timer to be set to expire in milliseconds
 * @param timeout_ms - set the timer to expire in this number of milliseconds
 */
void countdown_ms(Timer *timer, unsigned int timeout_ms);

/**
 * @brief Create a timer (seconds)
 *
 * Sets the timer to expire in a specified number of seconds.
 *
 * @param timer - pointer to the timer to be set to expire in seconds
 * @param timeout - set the timer to expire in this number of seconds
 */
void countdown(Timer *timer, unsigned int timeout);

/**
 * @brief Check the time remaining on a give timer
 *
 * Checks the input timer and returns the number of milliseconds remaining on the timer.
 *
 * @param timer - pointer to the timer to be set to checked
 * @return int - milliseconds left on the countdown timer
 */
int left_ms(Timer *timer);

/**
 * @brief Initialize a timer
 *
 * Performs any initialization required to the timer passed in.
 *
 * @param timer - pointer to the timer to be initialized
 */
void init_timer(Timer *timer);
    
#ifdef __cplusplus
}
#endif

#endif //C_SDK_UTILS_TIMER_H_

