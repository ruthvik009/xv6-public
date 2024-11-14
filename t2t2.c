#include "types.h"
#include "stat.h"
#include "user.h"

// Helper function to convert an integer to a string
void int_to_string(int num, char *str) {
    int idx = 0;

    // Handle the case where num is 0
    if (num == 0) {
        str[idx++] = '0';
    } else {
        // Convert number to string
        int temp_num = num;
        while (temp_num != 0) {
            str[idx++] = (temp_num % 10) + '0';
            temp_num /= 10;
        }
    }
    str[idx] = '\0';

    // Reverse the string to get the correct number representation
    for (int i = 0; i < idx / 2; i++) {
        char temp = str[i];
        str[i] = str[idx - i - 1];
        str[idx - i - 1] = temp;
    }
}

// Function to run a child process and execute the processprime command
void run_processprime(int prime_count, int pipe_fd) {
    char pipe_fd_str[10];
    int_to_string(pipe_fd, pipe_fd_str);

    char *args[] = {"processprime", itoa(prime_count), pipe_fd_str, 0};
    exec("processprime", args);
    printf(1, "Error: exec failed for processprime\n");
    exit();
}

// Function to wait for and check the completion signal from a pipe
void check_completion_signal(int pipe_fd) {
    char signal_buf[5];
    int bytes_read = read(pipe_fd, signal_buf, 4);

    if (bytes_read != 4) {
        printf(1, "Error: Did not receive expected signal from processprime\n");
        exit();
    }

    signal_buf[4] = '\0';
    if (strcmp(signal_buf, "done") == 0) {
        printf(1, "\nAll child processes of processprime are created and running.\n");
    } else {
        printf(1, "Error: Unexpected signal from processprime: %s\n", signal_buf);
        exit();
    }
}

// Function to run the nice command with priority
void run_nice(int priority, int pid) {
    char priority_str[10], pid_str[10];
    int_to_string(priority, priority_str);
    int_to_string(pid, pid_str);

    char *args[] = {"nice", pid_str, priority_str, 0};
    exec("nice", args);
    printf(1, "Error: exec failed for nice\n");
    exit();
}

int main() {
    int pipe_fd[2];
    int pid;

    // Create a pipe for communication between processes
    if (pipe(pipe_fd) < 0) {
        printf(1, "Error: Unable to create pipe\n");
        exit();
    }

    // Start the background process to run 'processprime'
    printf(1, "Launching 'processprime 3 &' in the background\n");
    pid = fork();

    if (pid == 0) { // Child process to run processprime
        close(pipe_fd[0]); // Close the read end in the child process
        run_processprime(3, pipe_fd[1]); // Run processprime and pass the pipe write end
    } else if (pid > 0) { // Parent process
        close(pipe_fd[1]); // Close the write end in the parent process
        check_completion_signal(pipe_fd[0]); // Wait for processprime to finish

        // Adjust the priority of the processprime process
        run_nice(5, pid); // Set priority to 5 for the current process
        wait(); // Wait for the nice command to complete

        // Wait for processprime to finish execution
        wait();
    } else {
        printf(1, "Error: fork failed\n");
        exit();
    }

    exit();
}
