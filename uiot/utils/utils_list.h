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

#ifndef C_SDK_UTILS_LIST_H_
#define C_SDK_UTILS_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>


/*
 * ListNode迭代器的方向.
 */
typedef enum {
    LIST_HEAD,
    LIST_TAIL
} ListDirection;

/*
 * 链表节点定义.
 */
typedef struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    void *val;
} ListNode;

/*
 * 链表.
 */
typedef struct {
    ListNode *head;
    ListNode *tail;
    unsigned int len;
    void (*free)(void *val);
    int (*match)(void *a, void *b);
} List;

/*
 * 迭代器.
 */
typedef struct {
    ListNode *next;
    ListDirection direction;
} ListIterator;


/* 节点. */
ListNode *list_node_new(void *val);

/* 链表. */
List *list_new(void);

ListNode *list_rpush(List *self, ListNode *node);

ListNode *list_lpush(List *self, ListNode *node);

ListNode *list_find(List *self, void *val);

ListNode *list_at(List *self, int index);

ListNode *list_rpop(List *self);

ListNode *list_lpop(List *self);

void list_remove(List *self, ListNode *node);

void list_destroy(List *self);

/* 迭代器. */
ListIterator *list_iterator_new(List *list, ListDirection direction);

ListIterator *list_iterator_new_from_node(ListNode *node, ListDirection direction);

ListNode *list_iterator_next(ListIterator *self);

void list_iterator_destroy(ListIterator *self);

#ifdef __cplusplus
}
#endif
#endif //C_SDK_UTILS_LIST_H_
