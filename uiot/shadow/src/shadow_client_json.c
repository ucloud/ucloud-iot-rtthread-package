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

#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "shadow_client_json.h"

#include "shadow_client.h"
#include "lite-utils.h"

/**
 * @brief 检查函数snprintf的返回值
 *
 * @param returnCode       函数snprintf的返回值
 * @param maxSizeOfWrite   可写最大字节数
 * @return                 返回ERR_JSON, 表示出错; 返回ERR_JSON_BUFFER_TRUNCATED, 表示截断
 */
int _check_snprintf_return(int32_t returnCode, size_t maxSizeOfWrite) 
{
    FUNC_ENTRY;

    if (returnCode >= maxSizeOfWrite) 
    {
        FUNC_EXIT_RC(ERR_JSON_BUFFER_TRUNCATED);
    } 
    else if (returnCode < 0) 
    { 
        FUNC_EXIT_RC(ERR_JSON);
    }

    FUNC_EXIT_RC(SUCCESS_RET);
}

int put_json_node(char *jsonBuffer, size_t sizeOfBuffer, const char *pKey, void *pData, JsonDataType type) 
{
    FUNC_ENTRY;

    POINTER_VALID_CHECK(pKey, ERR_PARAM_INVALID);

    int ret;
    int ret_of_snprintf = 0;
    size_t remain_size = 0;

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) 
    {
        FUNC_EXIT_RC(ERR_JSON_BUFFER_TOO_SMALL);
    }

    ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"%s\":", pKey);
    ret = _check_snprintf_return(ret_of_snprintf, remain_size);
    if (ret != SUCCESS_RET) 
    {
        FUNC_EXIT_RC(ret);
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) 
    {
        FUNC_EXIT_RC(ERR_JSON_BUFFER_TOO_SMALL);
    }

    if (pData == NULL) {
        ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "null,");
    } else {
        if (type == JINT32) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIi32
                                      ",", *(int32_t *) (pData));
        } else if (type == JINT16) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIi16
                                      ",", *(int16_t *) (pData));
        } else if (type == JINT8) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIi8
                                      ",", *(int8_t *) (pData));
        } else if (type == JUINT32) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIu32
                                      ",", *(uint32_t *) (pData));
        } else if (type == JUINT16) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIu16
                                      ",", *(uint16_t *) (pData));
        } else if (type == JUINT8) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%"
                                      PRIu8
                                      ",", *(uint8_t *) (pData));
        } else if (type == JDOUBLE) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%f,", *(double *) (pData));
        } else if (type == JFLOAT) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%f,", *(float *) (pData));
		} else if (type == JBOOL) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%u,",
                                      *(bool *) (pData) ? 1 : 0);		
        } else if (type == JSTRING) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"%s\",", (char *) (pData));
        } else if (type == JOBJECT) {
            ret_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "%s,", (char *) (pData));
        }
    }

    ret = _check_snprintf_return(ret_of_snprintf, remain_size);

    FUNC_EXIT_RC(ret);
}

bool parse_version_num(char *pJsonDoc, uint32_t *pVersionNumber) 
{
    FUNC_ENTRY;
    
	bool ret = false;

	char *version_num = LITE_json_value_of(VERSION_FIELD, pJsonDoc);
	if (version_num == NULL) FUNC_EXIT_RC(false);

	if (sscanf(version_num, "%" SCNu32, pVersionNumber) != 1) 
    {
		LOG_ERROR("parse shadow Version failed, errCode: %d\n", ERR_JSON_PARSE);
	}
	else 
    {
		ret = true;
	}

	HAL_Free(version_num);

	FUNC_EXIT_RC(ret);
}

bool parse_shadow_payload_retcode_type(char *pJsonDoc, uint32_t *pRetCode) 
{
    FUNC_ENTRY;

	bool ret = false;

	char *ret_code = LITE_json_value_of(PAYLOAD_RESULT_FIELD, pJsonDoc);
	if (ret_code == NULL) FUNC_EXIT_RC(false);

	if (sscanf(ret_code, "%" SCNu32, pRetCode) != 1) 
    {
		LOG_ERROR("parse RetCode failed, errCode: %d\n", ERR_JSON_PARSE);
	}
	else {
		ret = true;
	}

	HAL_Free(ret_code);

	FUNC_EXIT_RC(ret);
}

bool parse_shadow_payload_state_reported_state(char *pJsonDoc, char **pState)
{
	*pState = LITE_json_value_of(PAYLOAD_STATE_REPORTED_FIELD, pJsonDoc);
	return *pState == NULL ? false : true;
}

bool parse_shadow_payload_state_desired_state(char *pJsonDoc, char **pState)
{
	*pState = LITE_json_value_of(PAYLOAD_STATE_DESIRED_FIELD, pJsonDoc);
	return *pState == NULL ? false : true;
}

bool parse_shadow_method_type(char *pJsonDoc, char **pType)
{
	*pType = LITE_json_value_of(METHOD_FIELD, pJsonDoc);
	return *pType == NULL ? false : true;
}

bool parse_shadow_state_reported_type(char *pJsonDoc, char **pType)
{
	*pType = LITE_json_value_of(STATE_REPORTED_FIELD, pJsonDoc);
	return *pType == NULL ? false : true;
}

bool parse_shadow_state_desired_type(char *pJsonDoc, char **pType)
{
	*pType = LITE_json_value_of(STATE_DESIRED_FIELD, pJsonDoc);
	return *pType == NULL ? false : true;
}

bool parse_shadow_state_type(char *pJsonDoc, char **pType)
{
	*pType = LITE_json_value_of(STATE_FIELD, pJsonDoc);
	return *pType == NULL ? false : true;
}

char *find_value_if_key_match(char *pJsonDoc, DeviceProperty *pProperty) 
{
	char* property_data = LITE_json_value_of(pProperty->key, pJsonDoc);
	if ((property_data == NULL) || !(strncmp(property_data, "null", 4))
		 ||!(strncmp(property_data, "NULL", 4))) {
		 return NULL;
	}
	else {		
		return property_data;
	}

}

#ifdef __cplusplus
}
#endif
