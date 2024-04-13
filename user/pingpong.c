#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    char buf[2];
    char *par_msg = "p";
    char *chi_msg = "c";
    if (fork() == 0) {
        if (read(p[0], buf, 1) != 1) {
            fprintf(2, "Can't read from parent\n");
            exit(1);
        }
        // close(p[0]);
        printf("%d: ", getpid());
        printf("received ping\n");
        if (write(p[1], chi_msg, 1) != 1) {
            fprintf(2, "Can't send to parent\n");
            exit(1);
        }
        // close(p[1]);
        exit(0);
    } else {
        if (write(p[1], par_msg, 1) != 1) {
            fprintf(2, "Can't send to child\n");
            exit(1);
        }
        // close(p[1]);
        wait(0);
        printf("%d: ", getpid());
        printf("received pong\n");
        if (read(p[0], buf, 1) != 1) {
            fprintf(2, "Can't read from child\n");
            exit(1);
        }
        // close(p[0]);
        exit(0);
    }
}
