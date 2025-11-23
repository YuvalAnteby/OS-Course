// Yuval Anteby 212152896

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define LOCK_FILE "lockfile.lock"

/**
 * The prrovided function by TAs to write messages with random delays
 * @param message the message to write
 * @param count amount of times to write the message
 */
void write_message(const char *message, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", message);
        // Random delay between 0 and 99 milliseconds
        usleep((rand() % 100) * 1000); 
    }
}

// Function to acquire the lock
void acquire_lock() {
    int fd;
    // Try to create the lock file exclusively
    while ((fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
        if (errno == EEXIST) {
            // Lock file exists, another process has the lock
            // Wait a bit and try again
            usleep(10000); // Sleep for 10 milliseconds
        } else {
            // Some other error occurred
            perror("Error acquiring lock");
            exit(1);
        }
    }
    // Successfully created lock file, we have the lock
    close(fd);
}

/**
 * Function to release the lock
 */
void release_lock() {
    if (unlink(LOCK_FILE) == -1) {
        perror("Error releasing lock");
        exit(1);
    }
}

/**
 * Child process function
 */
void child_process(const char *message, int count, int child_num) {
    // Seed random number generator with unique value per process
    srand(time(NULL) ^ (getpid() << 16));
    
    // Acquire the lock before writing
    acquire_lock();
    
    // Critical section: write to stdout
    write_message(message, count);
    
    // Release the lock after writing
    release_lock();
    
    exit(0);
}

int main(int argc, char *argv[]) {
    // Check command-line arguments (at least 3 messages + count)
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <message1> <message2> <message3> ... <count>\n", argv[0]);
        return 1;
    }

    // Parse count (last argument)
    int count = atoi(argv[argc - 1]);
    
    // Validate count
    if (count <= 0) {
        fprintf(stderr, "Error: count must be a positive integer\n");
        return 1;
    }

    // Number of child processes = number of messages
    int num_children = argc - 2; // Exclude program name and count
    
    // Remove any existing lock file before starting
    unlink(LOCK_FILE);
    
    // Array to store child PIDs
    pid_t *child_pids = malloc(num_children * sizeof(pid_t));
    if (child_pids == NULL) {
        perror("Error allocating memory");
        return 1;
    }
    
    // Fork children in a loop
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Error forking child");
            free(child_pids);
            return 1;
        }
        
        if (pid == 0) {
            // Child process
            free(child_pids); // Child doesn't need this
            child_process(argv[i + 1], count, i + 1);
            // child_process calls exit(), so we never reach here
        }
        
        // Parent stores the child PID
        child_pids[i] = pid;
    }
    
    // Parent waits for all children to complete
    for (int i = 0; i < num_children; i++) {
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid == -1) {
            perror("Error waiting for child");
            free(child_pids);
            return 1;
        }
    }
    
    // Clean up
    free(child_pids);
    
    // Ensure lock file is removed (in case of any issues)
    unlink(LOCK_FILE);
    
    return 0;
}