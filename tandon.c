#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(void) {
    char *filename = "README";
    int file_desc = open(filename, O_CREATE | O_RDONLY);
    printf(1, "File Descriptor: %d\n", file_desc);

    // Try to write to the file
    int write_result = write(file_desc, "Test Write\n", 12);
    printf(1, "Write Operation Result: %d\n", write_result);

    close(file_desc);
    exit();
}