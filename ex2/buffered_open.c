// Yuval Anteby 212152896

#include "buffered_open.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h> 
#include <fcntl.h> 

// Function to wrap the original open function
buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    // Allocate memory for buffered_file_t structure
    buffered_file_t *bf = malloc(sizeof(buffered_file_t));
    if (!bf) {
        errno = ENOMEM;
        return NULL;
    }

    // Initialize structure
    memset(bf, 0, sizeof(buffered_file_t));

    // Check if O_PREAPPEND flag is set
    bf->preappend = (flags & O_PREAPPEND) ? 1 : 0;

    // Remove O_PREAPPEND from flags before calling open
    int actual_flags = flags & ~O_PREAPPEND;
    
    // If O_PREAPPEND is set, we need read access to read existing content
    if (bf->preappend && !(actual_flags & O_RDWR)) {
        // Convert O_WRONLY to O_RDWR for O_PREAPPEND
        actual_flags = (actual_flags & ~O_WRONLY) | O_RDWR;
    }
    
    bf->flags = actual_flags;

    // Handle optional mode argument for O_CREAT
    mode_t mode = 0;
    if (actual_flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
        bf->fd = open(pathname, actual_flags, mode);
    } else {
        bf->fd = open(pathname, actual_flags);
    }

    if (bf->fd == -1) {
        free(bf);
        return NULL;
    }

    // Allocate read and write buffers
    bf->read_buffer = malloc(BUFFER_SIZE);
    bf->write_buffer = malloc(BUFFER_SIZE);

    if (!bf->read_buffer || !bf->write_buffer) {
        if (bf->read_buffer) free(bf->read_buffer);
        if (bf->write_buffer) free(bf->write_buffer);
        close(bf->fd);
        free(bf);
        errno = ENOMEM;
        return NULL;
    }

    // Initialize buffer sizes and positions
    bf->read_buffer_size = 0;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;

    return bf;
}

// Function to flush the write buffer to the file
int buffered_flush(buffered_file_t *bf) {
    if (!bf || bf->fd == -1) {
        errno = EBADF;
        return -1;
    }

    // Nothing to flush if buffer is empty
    if (bf->write_buffer_pos == 0) {
        return 0;
    }

    // Handle O_PREAPPEND logic
    if (bf->preappend) {
        // Get current file size
        off_t file_size = lseek(bf->fd, 0, SEEK_END);
        if (file_size == -1) {
            return -1;
        }

        // Read existing file content if file is not empty
        char *temp_buffer = NULL;
        if (file_size > 0) {
            temp_buffer = malloc(file_size);
            if (!temp_buffer) {
                errno = ENOMEM;
                return -1;
            }

            // Seek to beginning and read all content
            if (lseek(bf->fd, 0, SEEK_SET) == -1) {
                free(temp_buffer);
                return -1;
            }

            ssize_t bytes_read = read(bf->fd, temp_buffer, file_size);
            if (bytes_read != file_size) {
                free(temp_buffer);
                return -1;
            }
        }

        // Seek to beginning to write new data first
        if (lseek(bf->fd, 0, SEEK_SET) == -1) {
            if (temp_buffer) free(temp_buffer);
            return -1;
        }

        // Write buffered data at the beginning
        ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (written != (ssize_t)bf->write_buffer_pos) {
            if (temp_buffer) free(temp_buffer);
            return -1;
        }

        // Append the old content after the new data
        if (temp_buffer) {
            ssize_t old_written = write(bf->fd, temp_buffer, file_size);
            free(temp_buffer);
            if (old_written != file_size) {
                return -1;
            }
        }

        // Update file position to end
        lseek(bf->fd, 0, SEEK_END);
    } else {
        // Normal flush: just write the buffer
        ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (written != (ssize_t)bf->write_buffer_pos) {
            return -1;
        }
    }

    // Reset write buffer position after successful flush
    bf->write_buffer_pos = 0;

    return 0;
}

// Function to write to the buffered file
ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    if (!bf || bf->fd == -1 || !buf) {
        errno = EBADF;
        return -1;
    }

    const char *data = (const char *)buf;
    size_t total_written = 0;

    while (total_written < count) {
        size_t space_available = bf->write_buffer_size - bf->write_buffer_pos;
        size_t to_write = count - total_written;

        if (to_write <= space_available) {
            // Data fits in buffer
            memcpy(bf->write_buffer + bf->write_buffer_pos, 
                   data + total_written, to_write);
            bf->write_buffer_pos += to_write;
            total_written += to_write;
        } else {
            // Fill the buffer and flush
            if (space_available > 0) {
                memcpy(bf->write_buffer + bf->write_buffer_pos,
                       data + total_written, space_available);
                bf->write_buffer_pos += space_available;
                total_written += space_available;
            }

            // Flush the full buffer
            if (buffered_flush(bf) == -1) {
                return -1;
            }
        }
    }

    return total_written;
}

// Function to read from the buffered file
ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    if (!bf || bf->fd == -1 || !buf) {
        errno = EBADF;
        return -1;
    }

    // Flush write buffer before reading to maintain consistency
    if (bf->write_buffer_pos > 0) {
        if (buffered_flush(bf) == -1) {
            return -1;
        }
    }

    char *output = (char *)buf;
    size_t total_read = 0;

    while (total_read < count) {
        // Check if read buffer has data
        size_t available = bf->read_buffer_size - bf->read_buffer_pos;

        if (available > 0) {
            // Read from buffer
            size_t to_read = (count - total_read < available) ? 
                             count - total_read : available;
            memcpy(output + total_read, 
                   bf->read_buffer + bf->read_buffer_pos, to_read);
            bf->read_buffer_pos += to_read;
            total_read += to_read;
        } else {
            // Buffer is empty, refill it
            ssize_t bytes_read = read(bf->fd, bf->read_buffer, BUFFER_SIZE);
            
            if (bytes_read == -1) {
                return -1;
            }
            
            if (bytes_read == 0) {
                // EOF reached
                break;
            }

            bf->read_buffer_size = bytes_read;
            bf->read_buffer_pos = 0;
        }
    }

    return total_read;
}

// Function to close the buffered file
int buffered_close(buffered_file_t *bf) {
    if (!bf) {
        errno = EBADF;
        return -1;
    }

    int result = 0;

    // Flush any pending writes before closing
    if (bf->fd != -1 && bf->write_buffer_pos > 0) {
        if (buffered_flush(bf) == -1) {
            result = -1;
            // Continue with cleanup even if flush fails
        }
    }

    // Close file descriptor
    if (bf->fd != -1) {
        if (close(bf->fd) == -1) {
            result = -1;
        }
    }

    // Free buffers
    if (bf->read_buffer) free(bf->read_buffer);
    if (bf->write_buffer) free(bf->write_buffer);

    // Free structure
    free(bf);

    return result;
}