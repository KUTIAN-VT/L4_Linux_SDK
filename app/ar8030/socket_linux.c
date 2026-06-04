#include "socketfd_port.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int set_recv_timeout(SOCKETFD sockfd, int ms)
{
    // LINUX
    struct timeval tv;
    tv.tv_sec  = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    return 0;
}

int connect_timeout(SOCKETFD connectSOCK, struct sockaddr_in* addrsrv, int timeoutms)
{
    int flg;
    flg = fcntl(connectSOCK, F_GETFL, NULL);
    fcntl(connectSOCK, F_SETFL, flg | O_NONBLOCK);

    int ret = connect(connectSOCK, (struct sockaddr*)addrsrv, sizeof(struct sockaddr));
    if (ret < 0) {
        if (errno == EINPROGRESS) {
            do {
                struct timeval tv = {
                    .tv_sec  = 0,
                    .tv_usec = timeoutms * 1000,
                };
                fd_set myset;

                FD_ZERO(&myset);
                FD_SET(connectSOCK, &myset);

                ret = select(connectSOCK + 1, NULL, &myset, NULL, &tv);
                if (ret < 0 && errno != EINTR) {
                    fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                    return -1;
                } else if (ret > 0) {
                    // Socket selected for write
                    socklen_t lon = sizeof(int);
                    int       valopt;
                    if (getsockopt(connectSOCK, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        return -2;
                    }
                    // Check the value returned...
                    if (valopt) {
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        return -3;
                    }
                    break;
                } else {
                    fprintf(stderr, "Timeout in select() - Cancelling!\n");
                    return -4;
                }
            } while (1);
        }
    }

    flg = fcntl(connectSOCK, F_GETFL, NULL);
    fcntl(connectSOCK, F_SETFL, flg & (~O_NONBLOCK));
    return 0;
}