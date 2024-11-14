#include "types.h"
#include "stat.h"
#include "user.h"

// Wait for the specified number of seconds
void wait_for_seconds(int seconds) {
    int start_ticks = uptime();
    while (uptime() - start_ticks < seconds * 100); // Wait for `seconds` seconds
}

// Check if a number is prime
int is_prime(int num) {
    if (num <= 1) return 0;
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) return 0; // Not a prime number
    }
    return 1; // Prime number
}

// Print a prime number with its position
void print_prime(int position, int prime, int mutexid) {
    lock(mutexid);
    printf(1, "PID %d: %d-th prime number is %d\n", getpid(), position, prime);
    unlock(mutexid);
}

// Calculate prime numbers up to the given limit
void calculate_primes(int limit, int mutexid) {
    int num = 2, count = 0;
    while (count < limit) {
        if (is_prime(num)) {
            print_prime(count + 1, num, mutexid); // Print the prime number and its position
            count++;
            wait_for_seconds(1); // Wait for 1 second between prints
        }
        num++;
    }
}

// Create child processes and distribute prime calculation among them
void spawn_children(int num_children, int prime_limit, int mutexid) {
    for (int i = 0; i < num_children; i++) {
        int pid = fork();
        if (pid < 0) {
            printf(1, "Fork failed\n");
            exit();
        } else if (pid == 0) {
            // Child process starts prime calculation
            printf(1, "Child %d started\n", getpid());
            calculate_primes(prime_limit, mutexid);
            printf(1, "Child %d finished\n", getpid());
            exit();
        } else {
            // Parent process logs the creation of child
            printf(1, "Parent %d created child %d\n", getpid(), pid);
        }
    }

    // Parent waits for all child processes to finish
    for (int i = 0; i < num_children; i++) {
        wait();
    }
}

// Notify that all child processes have been created (through pipe if needed)
void notify_creation_complete(int pipe_fd) {
    if (pipe_fd >= 0) {
        write(pipe_fd, "done", 4); // Send completion message to the pipe
        close(pipe_fd);
    }
}

// Main function to handle input, create mutex, and spawn children
int main(int argc, char *argv[]) {
    int num_children = (argc > 1) ? atoi(argv[1]) : 1; // Default to 1 child if no argument
    int pipe_fd = (argc > 2) ? atoi(argv[2]) : -1;     // If a pipe FD is passed

    // Ensure at least 1 child process is created
    if (num_children < 1) num_children = 1;

    // Get a mutex to ensure safe printing from multiple processes
    int mutexid = getmutex();
    if (mutexid < 0) {
        printf(1, "Error: Unable to acquire mutex\n");
        exit();
    }

    // Notify that child processes have been created (via pipe)
    notify_creation_complete(pipe_fd);

    // Spawn the child processes to calculate prime numbers
    spawn_children(num_children, 1000, mutexid);

    exit(); // Exit the parent process
}