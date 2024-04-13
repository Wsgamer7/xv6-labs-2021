#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_LEN 1024
void copy(char **p1, char *p2) {
    *p1 = malloc(strlen(p2) + 1);
    strcpy(*p1, p2);
}
int readLine(char *pars[], int i) {
    char a_par[MAX_LEN];
    int j = 0;
    while (read(0, a_par + j, sizeof(char)) > 0) {
        if (a_par[j] == '\n') {
            // a_par[j] = 0;
            break;
        }
        j++;
    }
    if (j == 0) {
        return -1;
    }
    int k = 0;
    while (k < j) {
        while ((k < j) && (a_par[k] == ' ')) {
            k++;
        }
        int l = k;
        while ((k < j) && (a_par[k] != ' '))
        {
            k++;
        }
        a_par[k] = 0;
        // k++;
        copy(&pars[i], a_par + l);
        i++;
    }

    return i;
}
int main(int argc, char *argv[]){
    if (argc < 2){
        printf("Please enter more parameters!\n");
        exit(1);
    }
    char *pars[MAXARG];
    int i;
    for (i = 1; i < argc; i++) {
        copy(&pars[i - 1], argv[i]);
    }
    int end;
    while ((end = readLine(pars, argc - 1)) != -1) {
        pars[end] = 0;
        if (fork() == 0) {
            exec(pars[0], pars);
            exit(0);
        } else {
            wait(0);
        }
    }
    exit(0);
}