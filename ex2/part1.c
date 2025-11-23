// Yuval Anteby 212152896

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

/**
 * Function to handle child process writing to file 
 * 
 * @param message the message to print
 * @param count
 * @param child_num
 * @param sleep_time
 */ 
void child_write(const char *message, int count, int child_num, int sleep_time) {
    // Naive synchronization: sleep if needed
    if (sleep_time > 0)
        if (sleep(sleep_time) != 0)
            perror("Sleep interrupted");
        
    

    // Open file for appending
    FILE *file = fopen("output.txt", "a");
    if (file == NULL) {
        perror("Child: Error opening file");
        exit(1);
    }

    // Write message count times
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s\n", message);
        fflush(file);  // Ensure data is written
    }

    fclose(file);
    exit(0);  // Child exits successfully
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <parent_msg> <child1_msg> <child2_msg> <count>\n", argv[0]);
        return 1;
    }

    // Parse arguments
    char *parent_msg = argv[1];
    char *child1_msg = argv[2];
    char *child2_msg = argv[3];
    int count = atoi(argv[4]);

    // Validate count
    if (count <= 0) {
        fprintf(stderr, "Error: count must be a positive integer\n");
        return 1;
    }

    // Create/open the output file (truncate if exists)
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening output.txt");
        return 1;
    }
    close(fd);

    pid_t pid1, pid2;

    // Fork first child process
    pid1 = fork();
    if (pid1 < 0) {
        perror("Error forking first child");
        return 1;
    }

    // First child process - no sleep needed (writes first)
    if (pid1 == 0)
        child_write(child1_msg, count, 1, 0);

    // Fork second child process
    pid2 = fork();
    if (pid2 < 0) {
        perror("Error forking second child");
        return 1;
    }

    // Second child process - sleep to allow child1 to finish using naive sync
    if (pid2 == 0)
        child_write(child2_msg, count, 2, 1);

    // Parent process - wait for both children to complete
    int status;
    pid_t finished_pid;

    // Wait for first child
    finished_pid = wait(&status);
    if (finished_pid == -1) {
        perror("Error waiting for first child");
        return 1;
    }
    printf("Child process %d finished\n", finished_pid);

    // Wait for second child
    finished_pid = wait(&status);
    if (finished_pid == -1) {
        perror("Error waiting for second child");
        return 1;
    }
    printf("Child process %d finished\n", finished_pid);

    // Now parent writes to the file
    FILE *file = fopen("output.txt", "a");
    if (file == NULL) {
        perror("Parent: Error opening file");
        return 1;
    }

    // Write parent message count times
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s\n", parent_msg);
        fflush(file);  // Ensure data is written
    }

    fclose(file);
    printf("Parent process finished writing\n");

    return 0;
}