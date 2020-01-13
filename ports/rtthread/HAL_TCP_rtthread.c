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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <rtthread.h>
#include "uiot_import.h"

static uint64_t rtthread_get_time_ms(void)
{
#if (RT_TICK_PER_SECOND == 1000)
    /* #define RT_TICK_PER_SECOND 1000 */
    return (unsigned long)rt_tick_get();
#else
    unsigned long tick = 0;
            
    tick = rt_tick_get();
    tick = tick * 1000;
    return (unsigned long)((tick + RT_TICK_PER_SECOND - 1)/RT_TICK_PER_SECOND);
#endif

}

static uint64_t rtthread_time_left(uint64_t t_end, uint64_t t_now)
{
    uint64_t t_left;

    if (t_end > t_now) {
        t_left = t_end - t_now;
    } else {
        t_left = 0;
    }

    return t_left;
}

uintptr_t HAL_TCP_Connect(_IN_ const char *host, _IN_ uint16_t port) {
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = 0;
    char service[6];

    memset(&hints, 0, sizeof(hints));

    printf("establish tcp connection with server(host='%s', port=[%u])\n", host, port);

    hints.ai_family = AF_INET; /* only IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%u", port);

    if ((rc = getaddrinfo(host, service, &hints, &addrInfoList)) != 0) {
        printf("getaddrinfo error(%d), host = '%s', port = [%d]\n", rc, host, port);
        return (uintptr_t) (-1);
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            printf("socket type error\n");
            rc = -1;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            printf("create socket error\n");
            rc = -1;
            continue;
        }

        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        close(fd);
        printf("connect error\n");
        rc = -1;
    }

    if (-1 == rc) {
        printf("fail to establish tcp\n");
    } else {
        printf("success to establish tcp, fd=%d\n", rc);
    }
    freeaddrinfo(addrInfoList);

    //忽略SIGPIPE，防止在网络异常时进程退出
    signal(SIGPIPE, SIG_IGN);

    return (uintptr_t) rc;
}


int32_t HAL_TCP_Disconnect(_IN_ uintptr_t fd) {
    int rc;

    rc = close((int) fd);
    if (0 != rc) {
        printf("close socket error\n");
        return FAILURE_RET;
    }

    return SUCCESS_RET;
}


int32_t HAL_TCP_Write(_IN_ uintptr_t fd, _IN_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    int ret,tcp_fd;
    size_t len_sent;
    uint64_t t_end;
    fd_set sets;
    IoT_Error_t net_err = SUCCESS_RET;

    t_end = rtthread_get_time_ms() + timeout_ms;
    len_sent = 0;
    ret = 1; /* send one time if timeout_ms is value 0 */

    if (fd >= FD_SETSIZE) {
        return -1;
    }
    tcp_fd = (int)fd;

    do {
        uint64_t t_left = rtthread_time_left(t_end, rtthread_get_time_ms());

        if (0 != t_left) {
            struct timeval timeout;

            FD_ZERO(&sets);
            FD_SET(tcp_fd, &sets);

            timeout.tv_sec = t_left / 1000;
            timeout.tv_usec = (t_left % 1000) * 1000;

            ret = select(tcp_fd + 1, NULL, &sets, NULL, &timeout);
            if (ret > 0) {
                if (0 == FD_ISSET(tcp_fd, &sets)) {
                    printf("Should NOT arrive\n");
                    /* If timeout in next loop, it will not sent any data */
                    ret = 0;
                    continue;
                }
            } else if (0 == ret) {
                printf("select-write timeout %d\n", tcp_fd);
                break;
            } else {
                if (EINTR == errno) {
                    printf("EINTR be caught\n");
                    continue;
                }

                printf("select-write fail, ret = select() = %d\n", ret);
                net_err = ERR_TCP_WRITE_FAILED;
                break;
            }
        }

        if (ret > 0) {
            ret = send(tcp_fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                printf("No data be sent\n");
            } else {
                if (EINTR == errno) {
                    printf("EINTR be caught\n");
                    continue;
                }

                printf("send fail, ret = send() = %d\n", ret);
                net_err = ERR_TCP_WRITE_FAILED;
                break;
            }
        }
    } while (!net_err && (len_sent < len) && (rtthread_time_left(t_end, rtthread_get_time_ms()) > 0));

    return net_err != SUCCESS_RET ? net_err : len_sent;
}


int32_t HAL_TCP_Read(_IN_ uintptr_t fd, _OU_ unsigned char *buf, _IN_ size_t len, _IN_ uint32_t timeout_ms) {
    int tcp_fd;
    IoT_Error_t err_code;
    size_t len_recv;
    uint64_t t_end;
    fd_set sets;
    struct timeval timeout;

    t_end = rtthread_get_time_ms() + timeout_ms;
    len_recv = 0;
    err_code = SUCCESS_RET;

    if (fd >= FD_SETSIZE) {
        return FAILURE_RET;
    }
    tcp_fd = (int)fd;

    do {
        uint64_t t_left = rtthread_time_left(t_end, rtthread_get_time_ms());
        if (0 == t_left) {
            break;
        }
        FD_ZERO(&sets);
        FD_SET(tcp_fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        int ret = select(tcp_fd + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            ret = recv(tcp_fd, buf + len_recv, len - len_recv, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                printf("connection is closed\n");
                err_code = ERR_TCP_PEER_SHUTDOWN;
                break;
            } else {
                if (EINTR == errno) {
                    continue;
                }
                printf("recv fail\n");
                err_code = ERR_TCP_READ_FAILED;
                break;
            }
        } else if (0 == ret) {
            break;
        } else {
            if (EINTR == errno) {
                continue;
            }
            printf("select-recv fail\n");
            err_code = ERR_TCP_READ_FAILED;
            break;
        }
    } while ((len_recv < len));

    return (0 != len_recv) ? len_recv : err_code;
}
