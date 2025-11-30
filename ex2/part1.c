// Yuval Anteby 212152896

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

/**
 * Function to handle child process writing to file 
 * @param message the message to print
 * @param count number of times to print the message
 * @param child_num the child process number
 * @param sleep_time time to sleep before writing (for naive sync)
 */ 
void child_write(const char *message, int count, int child_num, int sleep_time) {
    // Naive sync sleep if needed
    if (sleep_time > 0)
        if (sleep(sleep_time) != 0)
            perror("sleep interrupted");

    // Open file for appending
    int fd = open("output.txt", O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror("child: error opening file");
        exit(1);
    }

    // Prepare message string with newline
    size_t len = strlen(message);
    // +1 for null terminator char
    char *full_msg = (char *)malloc(len + 1); 
    if (full_msg == NULL) {
        perror("malloc failed");
        close(fd);
        exit(1);
    }
    strcpy(full_msg, message);
    full_msg[len] = '\0';
    size_t write_len = len;

    // Write message count times
    for (int i = 0; i < count; i++) {
        if (write(fd, full_msg, write_len) != write_len) {
            perror("child: error writing to file");
            free(full_msg);
            close(fd);
            exit(1);
        }
    }

    free(full_msg);
    // Close file
    if (close(fd) == -1) {
        perror("child: error closing file");
        exit(1);
    }

    // Child exits successfully
    exit(0); 
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 5) {
        // Use 'fprintf' to stderr, as 'write' is more verbose for this
        printf("Usage: %s <parent_msg> <child1_msg> <child2_msg> <count>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    char *parent_msg = argv[1];
    char *child1_msg = argv[2];
    char *child2_msg = argv[3];
    int count = atoi(argv[4]);

    // Validate count
    if (count <= 0) {
        // Using 'fprintf(stderr, ...)' is better than 'printf(stderr, ...)'
        printf("error: count must be a positive integer\n"); 
        return 1;
    }

    // Create/open the output file (truncate if exists) using 'open'
    // This part is already using the low-level style, so minimal change
    int fd_parent_init = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_parent_init == -1) {
        perror("error opening output.txt");
        return 1;
    }
    close(fd_parent_init);

    pid_t pid1, pid2;

    // Fork first child process
    pid1 = fork();
    if (pid1 < 0) {
        perror("error forking first child");
        return 1;
    }

    // First child process, no sleep needed since it writes first
    if (pid1 == 0)
        child_write(child1_msg, count, 1, 0);

    // Fork second child process
    pid2 = fork();
    if (pid2 < 0) {
        perror("error forking second child");
        return 1;
    }

    // Second child process, sleep to allow first child to finish (using naive sync)
    if (pid2 == 0)
        child_write(child2_msg, count, 2, 1);

    // Parent process, wait for both children to complete
    int status;
    pid_t finished_pid;

    // Wait for first child
    finished_pid = wait(&status);
    if (finished_pid == -1) {
        perror("error waiting for first child");
        return 1;
    }
    printf("Child process %d finished\n", finished_pid); 

    // Wait for second child
    finished_pid = wait(&status);
    if (finished_pid == -1) {
        perror("error waiting for second child");
        return 1;
    }
    printf("Child process %d finished\n", finished_pid);

    // Now parent writes to the file using low-level I/O
    int fd_parent_write = open("output.txt", O_WRONLY | O_APPEND, 0644);
    if (fd_parent_write == -1) {
        perror("parent: error opening file for write");
        return 1;
    }

    // Prepare parent message string
    size_t parent_len = strlen(parent_msg);
    char *parent_full_msg = (char *)malloc(parent_len + 1);
    if (parent_full_msg == NULL) {
        perror("malloc failed");
        close(fd_parent_write);
        return 1;
    }
    strcpy(parent_full_msg, parent_msg);
    parent_full_msg[parent_len + 1] = '\0';
    size_t parent_write_len = parent_len;

    // Write parent message count times
    for (int i = 0; i < count; i++) {
        if (write(fd_parent_write, parent_full_msg, parent_write_len) != parent_write_len) {
            perror("parent: error writing to file");
            free(parent_full_msg);
            close(fd_parent_write);
            return 1;
        }
    }
    free(parent_full_msg);

    // Close file, finished writing everything
    if (close(fd_parent_write) == -1) {
        perror("parent: error closing file");
        return 1;
    }

    printf("Parent process finished writing");

    return 0;
}