#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    if (argc < 1)
    {
        printf("error: missing argument of ticks");
        return 1;
    }

    int ticks = atoi(argv[1]);
    sleep(ticks);
    exit(0);
}
