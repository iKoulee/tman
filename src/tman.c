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
/*
char ** tokenize(char *input) {
    int i = 0;
    int j = 0;
    static int bookmark[64];

    while (input[i]) {
        if (!isspace(input[i]) && (i) && (isspace(input[i-1]))) {
            bookmark[j++] = i;
        }
        i++;
        printf("bookmark[%d]=%d(%c/%d),input[%d]=%c\n\n", j, bookmark[j], input[bookmark[j-1]], isspace(input[bookmark[j-1]]), i, input[i]);
    }
}
*/

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

int str2msg(char *command, tmanMSG_t *msg) {
    int i;
    struct timespec startTime;

    if ((!command) || (!msg)) {
        return -1;
    }
    memset(msg->data, 0, MQ_MSGSIZE);
    for (i = 0; command[i] != 0; i++) {
        switch (command[i]) {
            case '=':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_SET;
                break;
            case '-':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_SUB;
                break;
            case '+':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_ADD;
                break;
            case 'x':
                if (msg->member.type)
                    return -1;
                msg->member.type = T_MUL;
                break;
            case '0':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10;
                else
                    msg->member.delay.tv_sec = msg->member.delay.tv_sec * 10;
                break;
            case '1':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + 1;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 1;
                break;
            case '2':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + 2;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 2;
                break;
            case '3':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + 3;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 3;
                break;
            case '4':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + 4;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 4;
                break;
            case '5':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = msg->member.delta.tv_sec * 10 + 5;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 5;
                break;
            case '6':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = (msg->member.delta.tv_sec * 10) + 6;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 6;
                break;
            case '7':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = (msg->member.delta.tv_sec * 10) + 7;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 7;
                break;
            case '8':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = (msg->member.delta.tv_sec * 10) + 8;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 8;
                break;
            case '9':
                if (msg->member.type) 
                    msg->member.delta.tv_sec = (msg->member.delta.tv_sec * 10) + 9;
                else
                    msg->member.delay.tv_sec = (msg->member.delay.tv_sec * 10) + 9;
                break;
            case '@':
                if (i > 0)
                    return -1;
                if (clock_gettime(CLOCK_REALTIME, &startTime) != 0)
                    return -1;
            case ' ':
            case '\t':
            case '\n':
                /* Whitespace just ignore it! */
                break;
            default:
                return -1;
        }
    }
    if (command[0] == '@') {
        msg->member.delay.tv_sec = startTime.tv_sec - msg->member.delay.tv_sec;
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
            {"help", no_argument, NULL, 'h'},
            {"file", required_argument, NULL, 'f'}
        };

        static char *shortOpts = "p:t:f:c:Fh";

        flag = getopt_long(argc, argv, shortOpts, longOpts, &indexPtr);


        if (flag == -1)
            break;

        switch (flag) {
            case 'p':
                printf("Pid %s\n", optarg);
                break;
            case 't':
                if (str2msg(optarg,&msg) == -1) {
                    perror("Unable to parse time shift specification");
                    exit(1);
                }
                break;
            case 'c':
                command = optarg;
                break;
            case 'F':
                break;
            case 'h':
                break;
            case 'f':
                scriptName = optarg;
                break;
            default:
                printUsage(canonicalize_file_name(argv[0]));
        }
    }

    if ((!scriptName) && (!msg.member.type)) {
        printUsage(argv[0]);
        exit(1);
    }

    int chpid; 
    if (! (chpid = fork())) {
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
            printf("argc: %ld\nCmdName: %s\n", arg.we_wordc, arg.we_wordv[0]);
        } else {
            if (argc > optind) {
                arg.we_wordc = argc - optind;
                arg.we_wordv = &argv[optind];
                printf("Exec: %s\n", arg.we_wordv[0]);
            } else {
                return 1;
            }
        }

        putenv("LD_PRELOAD=./libtman.so");
        /*
        int i;
        for (i=0; environ[i]; i++)
            printf("Environment: %s\n", environ[i]);
        
        char *env[] = {"LD_PRELOAD=./libtman.so", NULL};
        */
        switch (arg.we_wordv[0][0]) {
            case '.':
            case '/':
                command = arg.we_wordv[0];
                break;
            default:
                command = findCommandInPath(arg.we_wordv[0]);
        }
        printf("File %s\nCanonical %s\n", arg.we_wordv[0] , command);
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
                error(0,errno,"Error: %d", errno);
        }
        wordfree(&arg);
    } else {
        printf("File: %s\n",scriptName);
        printf("Child pid is: %d\n", chpid);
        printf("Waiting for child\n");

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
        printf("Exiting\n");
        exit(childStatus);
    }
    return EXIT_SUCCESS;
}
