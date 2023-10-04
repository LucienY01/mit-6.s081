#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int pipeP2C[2];
    pipe(pipeP2C);
    int pipeC2P[2];
    pipe(pipeC2P);

    if (fork() == 0)
    {
        close(pipeP2C[1]);
        close(pipeC2P[0]);
        char byte[1] = {0};
        read(pipeP2C[0], byte, 1);
        printf("%d: received ping\n", getpid());
        write(pipeC2P[1], byte, 1);
        exit(0);
    }
    else
    {
        close(pipeP2C[0]);
        close(pipeC2P[1]);
        char byte[1] = {90};
        write(pipeP2C[1], byte, 1);
        read(pipeC2P[0], byte, 1);
        printf("%d: received pong\n", getpid());
        exit(0);
    }

    return 0;
}
