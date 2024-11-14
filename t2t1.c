#include "types.h"
#include "stat.h"
#include "user.h"

// Integer to ASCII conversion
void itoa(int num, char *str) {
    int i = 0, is_negative = 0;

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;  // Make the number positive
    }

    // Handle zero case
    if (num == 0) {
        str[i++] = '0';
    } else {
        // Convert integer to string (reverse order)
        while (num > 0) {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }

    // If negative, add the '-' sign
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';  // Null-terminate the string

    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char swap = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = swap;
    }
}

// Helper function to close both ends of the pipe
void close_pipe(int *pipe_fd) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

// Helper function to execute a command
void run_exec(char *cmd, char **args) {
    int result = exec(cmd, args);
    if (result < 0) {
        printf(1, "Error: exec failed for %s\n", cmd);
        exit();
    }
}

int main() {
    int pid;
    int pipe_fd[2];

    // Create the pipe for communication
    if (pipe(pipe_fd) < 0) {
        printf(1, "Error: Pipe creation failed\n");
        exit();
    }

    // Start the processprime process in the background with argument 4
    printf(1, "Launching 'processprime 4 &' as a background process\n");

    pid = fork();
    if (pid == 0) {  // Child process to execute processprime
        close(pipe_fd[0]);  // Close read end in child

        // Convert the write-end file descriptor to a string
        char fd_str[10];
        itoa(pipe_fd[1], fd_str);

        char *args[] = {"processprime", "4", fd_str, 0};
        run_exec("processprime", args);  // Execute processprime
    } else if (pid > 0) {  // Parent process
        close(pipe_fd[1]);  // Close write end in parent

        // Wait for signal from processprime
        char buf[5];
        int n = read(pipe_fd[0], buf, 4);
        if (n != 4) {
            printf(1, "Error: Signal from processprime was not received correctly\n");
            exit();
        }

        buf[4] = '\0';
        if (strcmp(buf, "done") == 0) {
            printf(1, "\nprocessprime has successfully created all child processes.\n");
        } else {
            printf(1, "Error: Unexpected message from processprime: %s\n", buf);
            exit();
        }

        // Close the pipe and execute 'ps' to display process status
        close(pipe_fd[0]);

        // Fork a new child to run 'ps'
        if (fork() == 0) {
            char *args[] = {"ps", 0};
            printf(1, "\nExecuting 'ps' to display process status\n");
            run_exec("ps", args);  // Execute ps
        }

        // Wait for child processes to finish
        wait();  // Wait for 'ps'
        wait();  // Wait for processprime to finish
    } else {
        printf(1, "Error: Fork operation failed\n");
        exit();
    }

    exit();
}