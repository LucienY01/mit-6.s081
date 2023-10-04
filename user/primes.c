#include "kernel/types.h"
#include "user/user.h"
#define rend 0
#define wend 1

void parent(int self, int next_prime, int pipe_left[], int pipe_right[])
{
    close(pipe_right[rend]);
    write(pipe_right[wend], &next_prime, 4);

    // Read from left neighbour and write to right neighbour.
    int number[1];
    int count = read(pipe_left[rend], number, 4);
    while (count != 0)
    {
        if (number[0] % self != 0)
        {
            write(pipe_right[wend], number, 4);
        }
        count = read(pipe_left[rend], number, 4);
    }

    close(pipe_left[rend]);
    close(pipe_right[wend]);
    wait(0);
    exit(0);
}

void child(int pipe_right[2])
{
    int pipe_left[2] = {pipe_right[0], pipe_right[1]};
    close(pipe_left[wend]);
    pipe(pipe_right);

    int number[1] = {0};
    read(pipe_left[rend], number, 4);
    int self = number[0];
    printf("prime %d\n", number[0]);
    int count = read(pipe_left[rend], number, 4);
    if (count == 0)
    {
        exit(0);
    }
    else
    {
        if (fork() == 0)
        {
            close(pipe_left[rend]);
            child(pipe_right);
        }
        else
        {
            parent(self, number[0], pipe_left, pipe_right);
        }
    }
}

int main(int argc, char const *argv[])
{
    int pipe_right[2];
    pipe(pipe_right);
    if (fork() == 0)
    {
        child(pipe_right);
    }
    else
    {
        close(pipe_right[rend]);
        for (int i = 2; i <= 35; i++)
        {
            write(pipe_right[wend], &i, 4);
        }
        close(pipe_right[wend]);
        wait(0);
        exit(0);
    }

    printf("error: control should not reach here");
    return -1;
}