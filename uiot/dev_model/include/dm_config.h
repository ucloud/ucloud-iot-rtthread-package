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

#ifndef C_SDK_DM_CONFIG_H_
#define C_SDK_DM_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DM_TOPIC_BUF_LEN         (128)
#define DM_MSG_REPLY_BUF_LEN     (128)
#define DM_MSG_REPORT_BUF_LEN    (2048)
#define DM_EVENT_POST_BUF_LEN    (2048)
#define DM_CMD_REPLY_BUF_LEN     (2048)

//pub
#define PROPERTY_RESTORE_TOPIC_TEMPLATE                  "/$system/%s/%s/tmodel/property/restore"
#define PROPERTY_POST_TOPIC_TEMPLATE                     "/$system/%s/%s/tmodel/property/post"
#define PROPERTY_SET_REPLY_TOPIC_TEMPLATE                "/$system/%s/%s/tmodel/property/set_reply"
#define PROPERTY_DESIRED_GET_TOPIC_TEMPLATE              "/$system/%s/%s/tmodel/property/desired/get"
#define PROPERTY_DESIRED_DELETE_TOPIC_TEMPLATE           "/$system/%s/%s/tmodel/property/desired/delete"
#define EVENT_POST_TOPIC_TEMPLATE                        "/$system/%s/%s/tmodel/event/post"
#define COMMAND_REPLY_TOPIC_TEMPLATE                     "/$system/%s/%s/tmodel/command_reply/%s"

//sub
#define PROPERTY_RESTORE_REPLY_TOPIC_TEMPLATE            "/$system/%s/%s/tmodel/property/restore_reply"
#define PROPERTY_POST_REPLY_TOPIC_TEMPLATE               "/$system/%s/%s/tmodel/property/post_reply"
#define PROPERTY_SET_TOPIC_TEMPLATE                      "/$system/%s/%s/tmodel/property/set"
#define PROPERTY_DESIRED_GET_REPLY_TOPIC_TEMPLATE        "/$system/%s/%s/tmodel/property/desired/get_reply"
#define PROPERTY_DESIRED_DELETE_REPLY_TOPIC_TEMPLATE     "/$system/%s/%s/tmodel/property/desired/delete_reply"
#define EVENT_POST_REPLY_TOPIC_TEMPLATE                  "/$system/%s/%s/tmodel/event/post_reply"
#define COMMAND_TOPIC_TEMPLATE                           "/$system/%s/%s/tmodel/command"

#ifdef __cplusplus
}
#endif

#endif //C_SDK_DM_CONFIG_H_
