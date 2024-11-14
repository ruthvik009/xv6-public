#include "types.h"
#include "stat.h"
#include "user.h"

// Helper function to test setting a valid priority for a given PID
void test_valid_priority(int pid, int priority, const char* description) {
    int old_priority = nice(pid, priority);
    if (old_priority >= 0) {
        printf(1, "%s: Successfully changed priority to %d for PID %d. Old Priority: %d\n", description, priority, pid, old_priority);
    } else {
        printf(1, "%s: Failed to set priority to %d for PID %d\n", description, priority, pid);
    }
}

// Helper function to test setting an invalid priority for a given PID
void test_invalid_priority(int pid, int priority, const char* description) {
    int old_priority = nice(pid, priority);
    if (old_priority == -1) {
        printf(1, "%s: Correctly rejected invalid priority %d for PID %d\n", description, priority, pid);
    } else {
        printf(1, "%s: Failed to reject invalid priority %d for PID %d\n", description, priority, pid);
    }
}

// Function to run a batch of tests for a given PID (current or specific PID)
void run_priority_tests(int pid) {
    printf(1, "Running tests for PID %d\n", pid);

    // Valid priority tests
    test_valid_priority(pid, 1, "Test 1 - Set minimum valid priority (1)");
    test_valid_priority(pid, 4, "Test 2 - Set valid priority (4)");
    test_valid_priority(pid, 5, "Test 3 - Set maximum valid priority (5)");

    // Invalid priority tests
    test_invalid_priority(pid, -5, "Test 4 - Reject negative priority (-5)");
    test_invalid_priority(pid, 25, "Test 5 - Reject out-of-range priority (25)");
    test_invalid_priority(pid, 10, "Test 6 - Reject out-of-range priority (10)");

    // Non-existent PID tests
    test_invalid_priority(9999, 3, "Test 7 - Set priority for non-existent PID (9999)");
}

// Main function to execute comprehensive tests for nice system call
int main() {
    int pid = getpid();  // Current process PID

    printf(1, "Running comprehensive tests for nice system call\n");

    // Tests for the current process
    run_priority_tests(pid);

    // Tests for specific PIDs (e.g., PID 1, 2)
    printf(1, "Running tests for specific PIDs\n");
    run_priority_tests(1); // Testing with PID 1
    run_priority_tests(2); // Testing with PID 2

    printf(1, "All tests completed\n");
    exit();
}
