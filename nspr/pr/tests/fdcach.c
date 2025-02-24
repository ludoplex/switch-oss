/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * File: fdcach.c
 * Description:
 *   This test verifies that the fd cache and stack are working
 *   correctly.
 */

#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Define ORDER_PRESERVED if the implementation of PR_SetFDCacheSize
 * preserves the ordering of the fd's when moving them between the
 * cache and the stack.
 */
#define ORDER_PRESERVED 1

/*
 * NUM_FDS must be <= FD_CACHE_SIZE.
 */
#define FD_CACHE_SIZE 1024
#define NUM_FDS 20

#ifdef NN_NINTENDO_SDK
int testMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    int i;
    PRFileDesc *fds[NUM_FDS];
    PRFileDesc *savefds[NUM_FDS];
    int numfds = sizeof(fds)/sizeof(fds[0]);

    /*
     * Switch between cache and stack when they are empty.
     * Then start with the fd cache.
     */
    PR_SetFDCacheSize(0, FD_CACHE_SIZE);
    PR_SetFDCacheSize(0, 0);
    PR_SetFDCacheSize(0, FD_CACHE_SIZE);

    /* Add some fd's to the fd cache. */
    for (i = 0; i < numfds; i++) {
        savefds[i] = PR_NewTCPSocket();
        if (NULL == savefds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    /*
     * Create some fd's.  These fd's should come from
     * the fd cache.  Verify the FIFO ordering of the fd
     * cache.
     */
    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
    }
    /* Put the fd's back to the fd cache. */
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    /* Switch to the fd stack. */
    PR_SetFDCacheSize(0, 0);

    /*
     * Create some fd's.  These fd's should come from
     * the fd stack.
     */
    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
#else
        savefds[numfds-1-i] = fds[i];
#endif
    }
    /* Put the fd's back to the fd stack. */
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    /*
     * Now create some fd's and verify the LIFO ordering of
     * the fd stack.
     */
    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
    }
    /* Put the fd's back to the fd stack. */
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    /* Switch to the fd cache. */
    PR_SetFDCacheSize(0, FD_CACHE_SIZE);

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
#else
        savefds[i] = fds[i];
#endif
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    /* Switch to the fd stack. */
    PR_SetFDCacheSize(0, 0);

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
#else
        savefds[numfds-1-i];
#endif
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        }
    }

    PR_Cleanup();
    printf("PASS\n");
    return 0;
}
