#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_CHILDREN 20
#define DEFAULT_CHILDREN 2
#define CALCULATION_LIMIT 4000000000

void create_child_process(int *x) {
    int z;
    printf(1, "Child %d created\n", getpid());
    // Useless calculation to consume CPU time
    for (z = 0; z < CALCULATION_LIMIT; z++) {
        *x = *x + 3.14 * 89.64;
    }
}

void handle_fork_error(int pid) {
    if (pid < 0) {
        printf(1, "%d failed in fork!\n", getpid());
    }
}

int parse_argument(int argc, char *argv[]) {
    int n = (argc < 2) ? DEFAULT_CHILDREN : atoi(argv[1]);
    return (n < 0 || n > MAX_CHILDREN) ? DEFAULT_CHILDREN : n;
}

void parent_process(int pid) {
    printf(1, "Parent %d creating child %d\n", getpid(), pid);
    wait(); // Wait for child process to finish
}

int main(int argc, char *argv[]) {
    int pid, x = 0;
    int num_children = parse_argument(argc, argv);

    for (int k = 0; k < num_children; k++) {
        pid = fork();
        handle_fork_error(pid);

        if (pid > 0) {
            // Parent process
            parent_process(pid);
        } else {
            // Child process
            create_child_process(&x);
            break; // Child exits after work
        }
    }

    exit();
}
