#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// Helper function to convert a string to an integer, return -1 if invalid
int string_to_int(const char *str) {
    int result = 0;

    // Convert string to integer, ensure all characters are digits
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return -1; // Return -1 if non-numeric character is found
        }
        result = result * 10 + (str[i] - '0');
    }

    return result;
}

// Helper function to validate priority (1 to 5)
int validate_priority(const char *str) {
    int priority = string_to_int(str);
    return (priority >= 1 && priority <= 5) ? priority : -1;
}

// Helper function to validate PID (positive integer greater than 0)
int validate_pid(const char *str) {
    int pid = string_to_int(str);
    return (pid > 0) ? pid : -1;
}

// Function to display an error message and terminate the program
void handle_error(const char *msg) {
    printf(2, "%s\n", msg);
    exit();
}

int main(int argc, char *argv[]) {
    int pid, priority, old_priority;

    // Parse command-line arguments
    if (argc == 2) {
        pid = getpid();  // Default to current process
        priority = validate_priority(argv[1]);
    } else if (argc == 3) {
        pid = validate_pid(argv[1]);
        priority = validate_priority(argv[2]);
    } else {
        handle_error("Usage: nice <priority> or nice <pid> <priority>");
    }

    // Validate the parsed PID and priority
    if (pid == -1) {
        handle_error("Error: Invalid PID. Must be a positive integer.");
    }
    if (priority == -1) {
        handle_error("Error: Invalid priority. Must be an integer between 1 and 5.");
    }

    // Attempt to change the priority and print the result
    old_priority = nice(pid, priority);
    if (old_priority == -1) {
        printf(2, "Error: Failed to set priority for PID %d. Process may not exist.\n", pid);
    } else {
        printf(1, "PID %d: priority changed from %d to %d\n", pid, old_priority, priority);
    }

    exit();
}
