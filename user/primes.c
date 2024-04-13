#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primeproc(int *p) {
    close(p[1]);
    int from;
    int next_p[2];
    pipe(next_p);
    if (read(p[0], &from, sizeof(from)) == sizeof(from)) {
        printf("prime %d\n", from);
        if (fork() == 0) {
            primeproc(next_p);
        }
        close(next_p[0]);
        int to;
        while (read(p[0], &to, sizeof(to)) == sizeof(to)) {
            if (to % from != 0) {
                write(next_p[1], &to, sizeof(to));
            }
        }
        close(p[0]);
        close(next_p[1]);
        wait(0);
    } 
    exit(0);
}
int
main(int argc, char *args[]) {
    int p[2];
    pipe(p);
    if (fork() == 0) {
        primeproc(p);
    }
    int limit = 32;
    if (argc == 2) {
        limit = atoi(args[1]);
    }
    close(p[0]);
    for (int i = 2; i <= limit; i++) {
        if (write(p[1], &i, 4) != 4) {
            fprintf(2, "cannot write %d to pipe, %d bytes \n", i);
        }
    }
    close(p[1]);
    wait(0);
    exit(0);
}