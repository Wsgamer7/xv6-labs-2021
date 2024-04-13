#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* args[]) {
    if (argc != 2) {
        fprintf(2, "ERROR: sleep time required\n");
        exit(1);
    } else {
        int sleep_count = atoi(args[1]);
        sleep(sleep_count);
        exit(0);
    }
}