#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    char c;
    char *argbuf[MAXARG - 1] = {0};
    int argp = 0;
    int cp = 0;
    int child = 0;
    int p;
    char *cmdargv[MAXARG - 1];

    if (strcmp(argv[1], "-n") == 0 && strcmp(argv[2], "1") == 0)
    {
        // Copy the arguments we actually want to execute.
        for (int i = 0; i < argc - 3; i++)
        {
            cmdargv[i] = argv[i + 3];
        }
        p = argc - 3;
    }
    else
    {
        // Copy the arguments we actually want to execute.
        for (int i = 0; i < argc - 1; i++)
        {
            cmdargv[i] = argv[i + 1];
        }
        p = argc - 1;
    }

    int count = read(0, &c, 1);
    while (count == 1)
    {
        if (c == ' ')
        {
            argbuf[argp][cp] = 0;
            argp++;
            cp = 0;
        }
        else if (c == '\n')
        {
            argbuf[argp][cp] = 0;
            argp++;
            cp = 0;
            if (fork() == 0)
            {
                // Append arguments in the line.
                for (int i = 0; i < argp; i++)
                {
                    cmdargv[p++] = argbuf[i];
                }
                cmdargv[p] = 0;

                exec(cmdargv[0], cmdargv);
                printf("xargs: exec failed");
                exit(1);
            }
            else
            {
                child++;
                argp = 0;
            }
        }
        else
        {
            if (argbuf[argp] == 0)
            {
                argbuf[argp] = (char *)malloc(20);
            }
            argbuf[argp][cp++] = c;
        }

        count = read(0, &c, 1);
    }
    if (count < 0)
    {
        printf("xargs: reading from standard input encounters error");
        exit(1);
    }
    if (count == 0)
    {
        for (int i = 0; i < MAXARG - 1; i++)
        {
            if (argbuf[i] != 0)
            {
                free(argbuf[i]);
            }
            else
            {
                continue;
            }
        }

        while (child > 0)
        {
            wait(0);
            child--;
        }
    }

    exit(0);
    return 0;
}
