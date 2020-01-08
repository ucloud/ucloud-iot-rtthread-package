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

#ifndef C_SDK_UIOT_INTERNAL_H_
#define C_SDK_UIOT_INTERNAL_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "uiot_export.h"
#include "uiot_import.h"

#define NUMERIC_VALID_CHECK(num, err) \
    do { \
        if (0 == (num)) { \
            return (err); \
        } \
    } while(0)

#define POINTER_VALID_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            return (err); \
        } \
    } while(0)

#define POINTER_VALID_CHECK_RTN(ptr) \
    do { \
        if (NULL == (ptr)) { \
            return; \
        } \
    } while(0)

#define STRING_PTR_VALID_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            return (err); \
        } \
        if (0 == strlen((ptr))) { \
            return (err); \
        } \
    } while(0)

#if defined(__cplusplus)
}
#endif

#endif /* C_SDK_UIOT_INTERNAL_H_ */
