#define _GNU_SOURCE /*enable function canonicalize_file_name*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/msg.h>
#include <linux/ipc.h>
#include <linux/limits.h>

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

        switch (flag) {
            case 'p':
                printf("Pid %s\n", optarg);
                break;
            case 't':
                printf("Time shift %s\n", optarg);
                break;
            case 'c':
                if ((optarg[0] == '.') || (optarg[0] == '/'))
                    commandPath = canonicalize_file_name(optarg);
                else
                    commandPath = findCommandInPath(optarg);
                printf("Command %s\n", commandPath);
                break;
            case 'F':
                break;
            case 'h':
                break;
        }
    }

    
/*    char fullPath[PATH_MAX];
    if (realpath(argv[0], fullPath)) {
        printf("My name is: %s\nMSGMAX is %i\n", fullPath, MSGMAX);
    }
*/
    return 0;
}
