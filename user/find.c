#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *filename) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot access path %s\n", path);
        exit(1);
    }
    if ((fstat(fd, &st) < 0) || st.type != T_DIR) {
        fprintf(2, "find: cannot stat path %s or path %s is not dir\n", path, path);
        exit(1);
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;
    // printf("%d\n", st.type);
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("ls: cannot stat %s\n", buf);
            continue;
        }
        
        switch (st.type)
        {
        case T_FILE:
            if (strcmp(de.name, filename) == 0) {
                printf("%s\n", buf);
            }
            break;
        
        case T_DIR:
            find(buf, filename);
            break;
        }

    }
    return;
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "please input 1 directory and 1 string\n");
        exit(1);
    }
    char *path = argv[1];
    char *filename = argv[2];
    find(path, filename);
    exit(0);
}