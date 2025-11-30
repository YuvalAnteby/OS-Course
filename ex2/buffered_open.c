// Yuval Anteby 212152896

#include "buffered_open.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

// Function to wrap the original open function
buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    // Allocate structure
    buffered_file_t *bf = (buffered_file_t *)malloc(sizeof(buffered_file_t));
    if (!bf) {
        errno = ENOMEM;
        return NULL;
    }

    // Initialize struct zero
    memset(bf, 0, sizeof(buffered_file_t));

    // Check for O_PREAPPEND flag
    if (flags & O_PREAPPEND) {
        bf->preappend = 1;
        // Remove O_PREAPPEND so the OS open() doesnt fail 
        flags &= ~O_PREAPPEND;
        
        // If we are preappending, we need to be able to read the file 
        // to shift existing content down. Force O_RDWR.
        if ((flags & O_ACCMODE) == O_WRONLY) {
            flags = (flags & ~O_WRONLY) | O_RDWR;
        }
    } else {
        bf->preappend = 0;
    }
    
    bf->flags = flags;

    // Handle variable arguments (mode) if O_CREAT is specified
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
        bf->fd = open(pathname, flags, mode);
    } else {
        bf->fd = open(pathname, flags);
    }

    if (bf->fd == -1) {
        free(bf);
        return NULL;
    }

    // Allocate Buffers
    bf->read_buffer = (char *)malloc(BUFFER_SIZE);
    bf->write_buffer = (char *)malloc(BUFFER_SIZE);

    if (!bf->read_buffer || !bf->write_buffer) {
        // Cleanup if allocation fails
        if (bf->read_buffer) free(bf->read_buffer);
        if (bf->write_buffer) free(bf->write_buffer);
        close(bf->fd);
        free(bf);
        errno = ENOMEM;
        return NULL;
    }

    // Initialize sizes and positions
    // Empty initially
    bf->read_buffer_size = 0;      
    bf->read_buffer_pos = 0;
    // Capacity
    bf->write_buffer_size = BUFFER_SIZE; 
    // Empty initially
    bf->write_buffer_pos = 0;      

    return bf;
}

// Function to flush the write buffer to the file
int buffered_flush(buffered_file_t *bf) {
    if (!bf) return -1;

    // If buffer is empty we are done
    if (bf->write_buffer_pos == 0) {
        return 0;
    }

    // Logic for O_PREAPPEND
    if (bf->preappend) {
        // Get current file size (move to end)
        off_t file_len = lseek(bf->fd, 0, SEEK_END);
        if (file_len == -1) return -1;

        // Read the EXISTING content into a temp buffer
        char *temp_buf = NULL;
        if (file_len > 0) {
            temp_buf = (char *)malloc(file_len);
            if (!temp_buf) {
                errno = ENOMEM;
                return -1;
            }
            
            // Go to start to read
            if (lseek(bf->fd, 0, SEEK_SET) == -1) {
                free(temp_buf);
                return -1;
            }

            // Read everything
            ssize_t r = read(bf->fd, temp_buf, file_len);
            if (r != file_len) {
                free(temp_buf);
                return -1; // Read failed or incomplete
            }
        }

        // Go to start to overwrite
        if (lseek(bf->fd, 0, SEEK_SET) == -1) {
            if (temp_buf) free(temp_buf);
            return -1;
        }

        // Write the NEW buffer data first (The preappend)
        ssize_t w = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (w != bf->write_buffer_pos) {
            if (temp_buf) free(temp_buf);
            return -1;
        }

        // Write the old data back moving it down the file
        if (file_len > 0) {
            w = write(bf->fd, temp_buf, file_len);
            // Done with temp buffer
            free(temp_buf); 
            if (w != file_len) {
                return -1;
            }
        }
    } 
    // Logic for Normal Write
    else {
        ssize_t w = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (w != bf->write_buffer_pos) {
            return -1;
        }
    }

    // Reset buffer
    bf->write_buffer_pos = 0;
    return 0;
}

// Function to write to the buffered file
ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    if (!bf) return -1;

    const char *data = (const char *)buf;
    size_t bytes_written = 0;

    while (bytes_written < count) {
        // Calculate space left in buffer
        size_t space_left = bf->write_buffer_size - bf->write_buffer_pos;
        
        // Calculate how much we can copy right now
        size_t to_copy = (count - bytes_written < space_left) ? (count - bytes_written) : space_left;

        // Copy into buffer
        memcpy(bf->write_buffer + bf->write_buffer_pos, data + bytes_written, to_copy);
        
        bf->write_buffer_pos += to_copy;
        bytes_written += to_copy;

        // If buffer is full, flush it
        if (bf->write_buffer_pos == bf->write_buffer_size)
            if (buffered_flush(bf) == -1) 
                return -1; 
    }

    return bytes_written;
}

// Function to read from the buffered file
ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    if (!bf) return -1;

    // Flush any pending writes before reading to ensure consistency
    if (bf->write_buffer_pos > 0) 
        if (buffered_flush(bf) == -1) 
            return -1;

    char *out_buf = (char *)buf;
    size_t total_read = 0;

    while (total_read < count) {
        // Calculate how much data available in read buffer
        size_t available = bf->read_buffer_size - bf->read_buffer_pos;

        // If buffer is empty refill it from the file
        if (available == 0) {
            ssize_t r = read(bf->fd, bf->read_buffer, BUFFER_SIZE);
            if (r == -1) return -1;
            // EOF reached
            if (r == 0) break;

            bf->read_buffer_size = r;
            bf->read_buffer_pos = 0;
            available = r;
        }

        // Calculate how much to copy to user
        size_t to_copy = (count - total_read < available) ? (count - total_read) : available;

        memcpy(out_buf + total_read, bf->read_buffer + bf->read_buffer_pos, to_copy);

        bf->read_buffer_pos += to_copy;
        total_read += to_copy;
    }

    return total_read;
}

// Function to close the buffered file
int buffered_close(buffered_file_t *bf) {
    if (!bf) return -1;

    int flush_result = 0;

    // Flush remaining write buffer
    if (bf->write_buffer_pos > 0) {
        // If flush fails, we close resources, and return error

        // We continue cleanup, but result will be -1 eventually
        if (buffered_flush(bf) == -1) 
            if (buffered_flush(bf) == -1) 
                flush_result = -1;
    }

    // Close file descriptor
    int close_result = close(bf->fd);

    // Free all memory
    if (bf->read_buffer) free(bf->read_buffer);
    if (bf->write_buffer) free(bf->write_buffer);
    free(bf);

    // If either the flush OR the close failed, we return -1.
    if (flush_result == -1 || close_result == -1) {
        return -1;
    }

    return 0;
}