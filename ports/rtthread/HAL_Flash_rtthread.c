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

#include "HAL_Flash_Platform.h"
#include <fal.h>

#define DOWNLOAD_PARTITION      "download"

void * HAL_Download_Name_Set(void * handle)
{
    return (void *)DOWNLOAD_PARTITION;
}


void * HAL_Download_Init(_IN_ void * name)
{
    const struct fal_partition * dl_part = RT_NULL;
    if ((dl_part = fal_partition_find((char *)name)) == RT_NULL)
    {
        LOG_ERROR("Firmware download failed! Partition (%s) find error!", name);
        return NULL;
    }

    LOG_INFO("Start erase flash (%s) partition!", dl_part->name);

    if (fal_partition_erase_all(dl_part) < 0)
    {
        LOG_ERROR("Firmware download failed! Partition (%s) erase error!", dl_part->name);
        return NULL;
    }
    LOG_INFO("Erase flash (%s) partition success!", dl_part->name);
    
    return (void *)dl_part;
}

int HAL_Download_Write(_IN_ void * handle,_IN_ uint32_t total_length,_IN_ uint8_t *buffer_read,_IN_ uint32_t length)
{
    const struct fal_partition * dl_part = (struct fal_partition *) handle;
    if(dl_part==NULL)
        return FAILURE_RET;
    if (fal_partition_write(dl_part, total_length, buffer_read, length) < 0){
        LOG_ERROR("Firmware download failed! Partition (%s) write data error!", dl_part->name);
        return FAILURE_RET;
    }
    return SUCCESS_RET;
}

int HAL_Download_End(_IN_ void * handle)
{
    LOG_INFO("System now will restart...");
    rt_thread_delay(rt_tick_from_millisecond(5));
    /* Reset the device, Start new firmware */
    extern void rt_hw_cpu_reset(void);
    rt_hw_cpu_reset();
    return SUCCESS_RET;
}


