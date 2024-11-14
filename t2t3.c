#include "types.h"
#include "stat.h"
#include "user.h"

// Function to convert an integer to a string
void int_to_string(int num, char *str) {
    int idx = 0;
    if (num == 0) {
        str[idx++] = '0';
    } else {
        int temp = num;
        while (temp != 0) {
            str[idx++] = (temp % 10) + '0';
            temp /= 10;
        }
    }
    str[idx] = '\0';

    // Reverse the string to get the correct order
    for (int i = 0; i < idx / 2; i++) {
        char temp = str[i];
        str[i] = str[idx - i - 1];
        str[idx - i - 1] = temp;
    }
}

// Helper function to execute 'processprime' command
void run_processprime(int pipe_fd) {
    // Convert the write end of the pipe to a string
    char fd_str[10];
    int_to_string(pipe_fd, fd_str);

    // Prepare arguments and execute processprime command
    char *args[] = {"processprime", "3", fd_str, 0};
    exec("processprime", args);
    printf(1, "Error: exec processprime failed\n");
    exit();
}

// Helper function to read and validate "done" signal from processprime
void validate_done_signal(int pipe_fd) {
    char buf[5];
    int n = read(pipe_fd, buf, 4);
    if (n != 4) {
        printf(1, "Error: Did not receive expected signal from processprime\n");
        exit();
    }
    buf[4] = '\0';  // Null-terminate string

    if (strcmp(buf, "done") == 0) {
        printf(1, "\nAll child processes of processprime have been created and are running.\n");
    } else {
        printf(1, "Error: Unexpected message from processprime: %s\n", buf);
        exit();
    }
}

// Helper function to run the 'nice' command to adjust process priority
void adjust_process_priority(int priority, int pid) {
    char priority_str[10], pid_str[10];
    int_to_string(priority, priority_str);
    int_to_string(pid, pid_str);

    char *args[] = {"nice", pid_str, priority_str, 0};
    exec("nice", args);
    printf(1, "Error: exec nice failed\n");
    exit();
}

int main() {
    int pipe_fd[2];
    int pid;

    // Create the pipe for communication between parent and child processes
    if (pipe(pipe_fd) < 0) {
        printf(1, "Error: Failed to create pipe\n");
        exit();
    }

    // Fork the process to run processprime in the background
    pid = fork();
    if (pid == 0) {  // Child process
        close(pipe_fd[0]); // Close the read end of the pipe
        run_processprime(pipe_fd[1]);  // Run the processprime command
    } else if (pid > 0) {  // Parent process
        close(pipe_fd[1]); // Close the write end of the pipe
        validate_done_signal(pipe_fd[0]);  // Validate "done" signal from processprime

        // Adjust the priority of the processprime process using nice
        adjust_process_priority(5, pid);
        wait();  // Wait for nice command to finish

        // Wait for the processprime process to complete
        wait();
    } else {
        printf(1, "Error: fork failed\n");
        exit();
    }

    exit();
}
