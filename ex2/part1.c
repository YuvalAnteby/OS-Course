// Yuval Anteby 212152896

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Writes a message in a child process
 * @param message the string to write to the file
 * @param count the number of times to write the message
 * @param the number of the child (1 or 2)
 * @param sleep_time used for the naive sync, will use sleep on one child only
 */
void child_write(const char *message, int count, int child_num, int sleep_time) {
    // Naive sync by using sleep
    if (sleep_time > 0)
        sleep(sleep_time);
        
    // Open file for appending using direct syscall
    int fd = open("output.txt", O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror("child%d: error opening file", child_num);
        exit(1);
    }

    // Get message length
    size_t msg_len = strlen(message);

    // Write message count times
    for (int i = 0; i < count; i++) {
        ssize_t written = write(fd, message, msg_len);
        if (written != msg_len) {
            perror("child: error writing to file");
            close(fd);
            _exit(1);
        }
    }

    // Close file using syscall
    close(fd);

    // exit successfully
    exit(0); 
}

int main(int argc, char *argv[]) {
    // Check command line arguments amount
    if (argc != 5) {
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
        printf("error: count must be a positive integer\n");
        return 1;
    }

    // Create/open the output file (truncate if exists) using open syscall
    int fd_init = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_init == -1) {
        perror("error opening output.txt");
        return 1;
    }
    close(fd_init);

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

    // Second child process, sleep to allow first child to finish (naive sync)
    if (pid2 == 0)
        child_write(child2_msg, count, 2, 1);

    // Parent process is waiting for both children to complete
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

    // Now parent writes to the file using open syscall
    int fd_parent = open("output.txt", O_WRONLY | O_APPEND, 0644);
    if (fd_parent == -1) {
        perror("parent: error opening file for write");
        return 1;
    }

    // write it count times
    size_t parent_len = strlen(parent_msg);
    for (int i = 0; i < count; i++) {
        ssize_t written = write(fd_parent, parent_msg, parent_len);
        if (written != parent_len) {
            perror("parent: error writing to file");
            close(fd_parent);
            return 1;
        }
    }

    // Close file
    close(fd_parent);

    printf("Parent process finished writing\n");

    return 0;
}