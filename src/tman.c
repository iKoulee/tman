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
#include "tman.h"


char *findCommandInPath(const char *command) {
    char const *delim = ":";
    char *envPath = getenv("PATH");
    int comLen, pathLen;
    char *absPath = malloc(PATH_MAX);
    struct stat *BUF = malloc(sizeof(stat));
    int statVal;

    if (envPath) {
        char *path = strtok(envPath, delim);
        comLen = strlen(command);
        while (path != NULL) {
            int i;
            for (i = 0; i<PATH_MAX; i++)
                absPath[i]=0;
            pathLen = strlen(path);
            if ((pathLen >= PATH_MAX) || ((pathLen + comLen + 1) >= PATH_MAX))
                return NULL;
            strcpy(absPath, path);
            absPath[pathLen] = '/';
            strncat(absPath, command, comLen);
            if ((statVal = stat(absPath,BUF)) == 0) 
                return absPath;
            path = strtok(NULL, delim);
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    char *commandPath;
    char *command;
    char mqName[MQ_MAXNAMELENGTH];
    mqd_t mQ;
    tmanMSG_t msg;
    int childStatus;
    struct mq_attr mqAttr;

    if (argc <= 1) {
        printf("Nothing to do\n");
        return 1;
    }
    while (1) {
        static int indexPtr = 0;
        static int flag;
        static struct option longOpts[] = {
            {"pid", required_argument, NULL, 'p'},
            {"time-shift", required_argument, NULL, 't'},
            {"command", required_argument, NULL, 'c'},
            {"fuzzer", no_argument, NULL, 'F'},
            {"help", no_argument, NULL, 'h'}
        };

        static char *shortOpts = "p:t:c:Fh";

        flag = getopt_long(argc, argv, shortOpts, longOpts, &indexPtr);

        if (flag == -1)
            break;

        printf("optarg='%s'\n", optarg);
        switch (flag) {
            case 'p':
                printf("Pid %s\n", optarg);
                break;
            case 't':
                printf("Time shift %s\n", optarg);
                break;
            case 'c':
                command = optarg;
                if ((optarg[0] == '.') || (optarg[0] == '/')) {
                    commandPath = canonicalize_file_name(optarg);
                } else
                    commandPath = findCommandInPath(optarg);
                printf("Command %s\n", commandPath);
                break;
            case 'F':
                break;
            case 'h':
                break;
            default:
                printf("optarg='%s'\n", optarg);
        }
    }
        
    int chpid;
    if (! (chpid = fork())) {
        char *args[] = {"", NULL};
        args[0] = commandPath; 
        char *env[] = {"LD_PRELOAD=./libtman.so", NULL};
        int rc = execve(args[0], args, env);
        if (rc < 0) {
            error(0,errno,"Error: %d", errno);
        } else {
            printf("'%s' returns %d\n", command, rc);
        }
    } else {
        printf("Child pid is: %d\n", chpid);
        printf("Waiting for child\n");

/*
 * Open queue
 */

        mqAttr.mq_msgsize = MQ_MSGSIZE;
        mqAttr.mq_maxmsg = MQ_MAXMSG;

        snprintf(mqName, MQ_MAXNAMELENGTH, "%s.%d", MQ_PREFIX, chpid);
        mQ = mq_open(mqName, O_CREAT | O_WRONLY, 0600, &mqAttr);
        if (mQ == -1) {
            perror("Cannot open message queue.\n");
            exit(128+mQ);
        }
        memset(msg.data, 0, MQ_MSGSIZE);
        msg.member.delta.tv_sec = 1800;
        msg.member.delay.tv_sec = 666;
        msg.member.type = T_ADD;
        
        if (! mq_send(mQ, msg.data, MQ_MSGSIZE, 0)) {
            switch (errno) {
                case EAGAIN:
                    perror("The Queue is full");
                break;
                case EBADF:
                    perror("Invalid Queue");
                break;
                case EINTR:
                    perror("Interupted by signal");
                break;
                case EINVAL:
                    perror("Invalid timeout");
                break;
                case EMSGSIZE:
                    perror("Message is too long");
                break;
                case ETIMEDOUT:
                    perror("Timeout");
                break;
            }
        }

        waitpid(-1, &childStatus, 0);
        mq_close(mQ);
        mq_unlink(mqName);
        printf("Exiting\n");
        exit(childStatus);
    }
    return 0;
}
