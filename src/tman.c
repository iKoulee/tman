#define _GNU_SOURCE /*enable function canonicalize_file_name*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h> /* because of sleep */
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <linux/limits.h>
#include <errno.h>
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
    static char *commandPath;
    static char *command;

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
        }
    }

    int msqid = msgget(ftok(commandPath, 'f'), 0666 | IPC_CREAT);
    if (msqid <= 0)
        error(0, errno,"errno: %d", errno);
    else
        printf("Queue id: %d\n", msqid);
/*
    struct mq_attr *qattr = malloc(sizeof(struct mq_attr));
    mq_getattr(msqid, qattr);
    printf("------------------------------");
    printf("Queue id: %d\nFlags: %ld\nMessages count: %ld\n", msqid, qattr.mq_flags,qattr.mq_flags);
*/
    struct timeMsgBuf message = { 1, {42}};
    int msgid = msgsnd(msqid, &message, (sizeof(struct timeMsgBuf) - sizeof(long)), 0);
    if (msgid != 0)
        error(0, errno,"errno: %d", errno);
    else
        printf("MSG ID: %d\n", msgid);
    sleep(1);

    if (!fork()) {
        char *args[] = {"date", NULL};
        const char *env[2];
        env[1] = "LD_PRELOAD=./libtman.so";
        env[2] = NULL;
        printf("Command '%s'\n", commandPath);
        int rc = execv(commandPath, args);
        if (rc < 0) {
            error(0,errno,"Error: %d", errno);
        } else {
            printf("'%s' returns %d\n", command, rc);
        }
    } else {
        printf("Waiting for child\n");
        sleep(10);
    }



    
/*    char fullPath[PATH_MAX];
    if (realpath(argv[0], fullPath)) {
        printf("My name is: %s\nMSGMAX is %i\n", fullPath, MSGMAX);
    }
*/
    return 0;
}
