/*
 ******************************************************************************
 * Copyright (c) 2014 Red Hat, Inc., Jakub Prokeš <jprokes@redhat.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 *
 ******************************************************************************
 */

#define _GNU_SOURCE /*enable function canonicalize_file_name*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h> /* because of sleep */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/errno.h>
#include <error.h>
#include <ctype.h>
#include <wordexp.h>
#include <libgen.h>
#include <math.h>
#include "tman.h"

void printUsage(char *progName) {
    puts("Usage:");
    printf("%s [-c <command> | -p <pid>] <-t <change> | -F | -f <file_name> >\n", progName);
    printf("%s <-t <change> | -F | -f <file_name> > -- <command> <command_arguments>\n\n", progName);
    puts("\t-c <cmd>\tCommand to execute");
    puts("\t-f <file>\tScript file with time manipulations");
    puts("\t-F\t\tFuzzer mode: Send randomly modified time for each request");
    puts("\t-h\t\tShow this help");
    puts("\t-p <pid>\tManipulate time on existing process");
    puts("\t-t <time>\tTime manipulation\n");
}

char *findCommandInPath(const char *command) {
    char const *delim = ":";
    char *envPath = getenv("PATH");
    int comLen, pathLen;
    char *absPath = malloc(PATH_MAX);
    struct stat stat_buf;
    int statVal;

    if (envPath) {
        char *path = strtok(envPath, delim);
        comLen = strlen(command);
        while (path != NULL) {
	    memset(absPath, '\0', PATH_MAX);
            pathLen = strlen(path);
            if ((pathLen >= PATH_MAX) || ((pathLen + comLen + 1) >= PATH_MAX))
                return NULL;
            strcpy(absPath, path);
            absPath[pathLen] = '/';
            strncat(absPath, command, comLen);
            if ((statVal = stat(absPath, &stat_buf)) == 0) 
                return absPath;
            path = strtok(NULL, delim);
        }
    }
    return NULL;
}


int str2msg(char *command, tmanMSG_t *msg) {
    int i;
    int point = 0;

    if ((!command) || (!msg)) {
        return -1;
    }
    memset(msg->data, 0, MQ_MSGSIZE);
    for (i = 0; command[i] != 0; i++) {
        switch (command[i]) {
            case '#':
                /* comment */
                return 0;
            case '=':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_SET;
                point = 0;
                break;
            case '@':
                if (msg->member.type)
                    return -1;
                point = 0;
                msg->member.type = T_MOV;
                break;
            case '-':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_SUB;
                point = 0;
                break;
            case '+':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_ADD;
                point = 0;
                break;
            case '*':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_MUL;
                point = 0;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (msg->member.type)
                    if (point) {
                        msg->member.delta.tv_nsec = msg->member.delta.tv_nsec + (command[i] - 48) * (int) pow10(point--);
#ifdef __DEBUG
                        printf("%d = pow10(%d+1)\n", (int)  pow10(point+1), point);
                        printf("nsec + (%d - 48) * %d\n", command[i], (int) pow10(point+1));
#endif
                    } else
                        msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + command[i] - 48;
                else
                    if (point)
                        msg->member.delay.tv_nsec = msg->member.delay.tv_nsec + (command[i] - 48) * (int) pow10(point--);
                    else
                        msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + command[i] - 48;
                break;
            case '.':
            case ',':
                point = 8;
                break;
            case ' ':
            case '\t':
            case '\n':
                /* Whitespace just ignore it! */
                break;
            default:
                return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    char *command = NULL;
    char mqName[MQ_MAXNAMELENGTH];
    mqd_t mQ;
    tmanMSG_t msg;
    int childStatus;
    struct mq_attr mqAttr;
    char *scriptName = NULL;
    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    if (argc <= 1) {
        fprintf(stderr,"Nothing to do!\n");
        exit(EXIT_FAILURE);
    }
    while (1) {
        int indexPtr = 0;
        int flag;
        static struct option longOpts[] = {
            {"pid", required_argument, NULL, 'p'},
            {"time-shift", required_argument, NULL, 't'},
            {"command", required_argument, NULL, 'c'},
            {"fuzzer", no_argument, NULL, 'F'},
            {"help", no_argument, NULL, 'h'},
            {"file", required_argument, NULL, 'f'},
            {NULL, 0, 0, 0}
        };

        static char *shortOpts = "p:t:f:c:Fh";
        flag = getopt_long(argc, argv, shortOpts, longOpts, &indexPtr);

        if (flag == -1)
            break;

        switch (flag) {
            case 't':
                if (str2msg(optarg,&msg) == -1) {
                    fprintf(stderr, "Unable to parse time shift specification: '%s'.\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':
                command = optarg;
                break;
            case 'h':
                printUsage(basename(argv[0]));
                exit(EXIT_SUCCESS);
            case 'f':
                scriptName = optarg;
                break;
            case '?':
                printUsage(basename(argv[0]));
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "This feature hasn't been implemented yet.\n");
                exit(EXIT_FAILURE);
        }
    }

    if ((!scriptName) && (!msg.member.type)) {
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int chpid; 
    if ((chpid = fork()) == -1) {
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    } else if ( chpid == 0 ) {
        wordexp_t arg;
        if (command) {
            switch (wordexp(command, &arg, WRDE_SHOWERR)) {
                case 0:
                    break;
                case WRDE_BADCHAR:
                    perror("Ilegal character");
                    return 1;
                case WRDE_BADVAL:
                    perror("Undefined shell variable");
                    return 1;
                case WRDE_CMDSUB:
                    perror("Command substitution is disalowed");
                    return 1;
                case WRDE_NOSPACE:
                    perror("Memory allocation failed");
                    return 1;
                case WRDE_SYNTAX:
                    perror("Syntax error");
                    return 1;
            }
        } else {
            if (argc > optind) {
                arg.we_wordc = argc - optind;
                arg.we_wordv = &argv[optind];
                printf("Exec: %s\n", arg.we_wordv[0]);
            } else {
                return 1;
            }
        }

        putenv("LD_PRELOAD=libtman.so");
        switch (arg.we_wordv[0][0]) {
            case '.':
            case '/':
                command = arg.we_wordv[0];
                break;
            default:
                command = findCommandInPath(arg.we_wordv[0]);
        }
#ifdef __DEBUG
        printf("File %s\nCanonical %s\n", arg.we_wordv[0] , command);
#endif
        switch (execve(command, arg.we_wordv, environ)) {
            case E2BIG:
                perror("Argument list is to long");
                break;
            case ENOEXEC:
                perror("File cannot be executed");
                break;
            case ENOMEM:
                perror("Not enough memory");
                break;
            case EPERM:
            case EACCES:
                perror("Permission denied");
                break;
            case EAGAIN:
                perror("Resources limited");
                break;
            case EFAULT:
                perror("PRUSER!");
                break;
            case EIO:
                perror("I/O error!");
                break;
            case EINVAL:
            case EISDIR:
            case ELIBBAD:
                perror("Bad interpreter");
                break;
            case ELOOP:
                perror("Loop in symbolic links detected");
                break;
            case ENFILE:
            case EMFILE:
                perror("Maximum count of open files reached");
                break;
            case ENAMETOOLONG:
                perror("Filename is too long");
                break;
            case ENOENT:
                perror("Missing file");
                break;
            case ETXTBSY:
                perror("Executable is open for writing");
                break;
            default:
                fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
        wordfree(&arg);
    } else {
        /* Open the kernel message qeue */
        mqAttr.mq_msgsize = MQ_MSGSIZE;
        mqAttr.mq_maxmsg = MQ_MAXMSG;

        snprintf(mqName, MQ_MAXNAMELENGTH, "%s.%d", MQ_PREFIX, chpid);
        mQ = mq_open(mqName, O_CREAT | O_WRONLY, 0600, &mqAttr);
        if (mQ == -1) {
            fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (msg.member.type) {
            if (mq_send(mQ, msg.data, MQ_MSGSIZE, 0) == -1) {
                fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            } else {
#ifdef __DEBUG
                printf("MSG send sucessfully: %d\n", msg.member.type);
#endif
            }
        }

        if (scriptName) {
            fp = fopen(scriptName, "r");
            if (fp == NULL) {
                fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            }

            while ((read = getline(&line, &len, fp)) != -1) {
                if (str2msg(line, &msg) != -1) {
                    if (mq_send(mQ, msg.data, MQ_MSGSIZE, 0) == -1) {
                        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        waitpid(-1, &childStatus, 0);
        mq_close(mQ);
        mq_unlink(mqName);
        exit(childStatus);
    }
    return EXIT_SUCCESS;
}
