#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int child_pid = fork();  // Create first child process
    
    if(child_pid < 0) {  // If the first fork failed
        printf(1, "Failed to fork first child.\n");
    } else if(child_pid == 0) {  // In first child process
        printf(1, "First child is executing\n");
    } else {  // In parent process
        child_pid = fork();  // Create second child process
        
        if(child_pid < 0) {  // If the second fork failed
            printf(1, "Failed to fork second child.\n");
        } else if(child_pid == 0) {  // In second child process
            printf(1, "Second child is executing\n");
        } else {  // Parent process
            printf(1, "Parent is waiting for children to finish.\n");
            wait();  // Wait for the first child
            wait();  // Wait for the second child
            
            printf(1, "Both children have exited.\n");
            printf(1, "Parent process is continuing.\n");
            printf(1, "Parent process is exiting.\n");
        }
    }
    
    exit();  // Exit the current process
}