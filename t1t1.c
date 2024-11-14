#include "types.h"
#include "stat.h"
#include "user.h"

// Helper function to run each test with description and arguments
void run_test(char *desc, char *arg1, char *arg2) {
    // Print the description of the test being run
    printf(1, "Test: %s | Command: nice %s %s\n", desc, arg1, arg2 ? arg2 : "");

    // Create a child process to run the "nice" command
    if (fork() == 0) {
        char *args[] = {"nice", arg1, arg2, 0}; // Argument array for exec
        int exec_result = exec("nice", args);   // Run the nice command

        // If exec fails, exit the child process with an error
        if (exec_result < 0) {
            printf(1, "Error: exec failed for nice with args: %s %s\n", arg1, arg2 ? arg2 : "");
            exit();
        }
        exit(); // Exit the child process after executing
    }
    wait(); // Parent process waits for the child to finish
}

int main() {
    printf(1, "Running refactored test for nice CLI edge cases\n");

    // Start with valid test cases
    run_test("Setting priority to 1 for current process", "1", 0);
    run_test("Setting priority to 5 for current process", "5", 0);
    run_test("Setting priority to 5 for PID 1 (valid)", "1", "5");

    // Now test cases with specific PIDs
    run_test("Setting priority to 9 for PID 1 (invalid priority)", "1", "9");
    run_test("Setting priority to 5 for non-existing PID 40", "40", "5");
    run_test("Setting priority to 4 for PID 0 (invalid PID)", "0", "4");

    // Edge cases with invalid or non-numeric priorities
    run_test("Setting priority to 9 (out of range) for current process", "9", 0);
    run_test("Setting priority to 0 (out of range) for current process", "0", 0);
    run_test("Setting priority to -9 (invalid) for current process", "-9", 0);
    run_test("Setting priority to a non-numeric value ','", ",", 0);

    // Test with non-numeric PID values
    run_test("Setting priority to 'abc' for PID 1 (invalid priority)", "1", "abc");
    run_test("Setting priority to 3 for non-numeric PID 'abc'", "abc", "3");

    exit();
}