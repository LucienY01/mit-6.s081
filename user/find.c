#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// findir finds all files named filename in path and its subdirectories.
void finddir(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
        fprintf(2, "find: '%s' is a filename\n", path);
        return;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("ls: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0)
            {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            switch (st.type)
            {
            case T_FILE:
                if (strcmp(filename, p) == 0)
                {
                    printf("%s\n", buf);
                }
                break;

            case T_DIR:
                if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
                {
                    break;
                }
                finddir(buf, filename);
                break;
            }
        }
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(2, "find: arguments number is not 2");
    }

    finddir(argv[1], argv[2]);
    exit(0);
    return 0;
}