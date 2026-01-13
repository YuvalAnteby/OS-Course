#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// copy files recursively from src to dest
void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat st;
    
    // Use lstat to detect symlinks (soft link file's link) properly
    if (lstat(src, &st) < 0) {
        perror("lstat failed");
        return;
    }

    // CASE 1: soft links
    if (S_ISLNK(st.st_mode) && copy_symlinks) {
        char link_target[PATH_MAX];
        ssize_t len = readlink(src, link_target, sizeof(link_target) - 1);
        if (len < 0) {
            perror("readlink failed");
            return;
        }
        link_target[len] = '\0';

        if (symlink(link_target, dest) < 0) {
            perror("symlink failed");
            return;
        }
        // permissions on soft links generally aren't changed on Linux according to some info i found
        return; 
    }

    // CASE 2: hard links (Regular Files)
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("open src failed");
        return;
    }

    // open dest: create, truncate if exists, write only.
    // default permissions 0644 (rw-r--r--), modified by umask.
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("open dest failed");
        close(src_fd);
        return;
    }

    // create a buffer and move the files
    char buf[4096];
    ssize_t nread;
    while ((nread = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dest_fd, buf, nread) != nread) {
            perror("write failed");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (nread < 0) {
        perror("read failed");
    }

    close(src_fd);
    close(dest_fd);

    // CASE 3: handle permissions
    if (copy_permissions) {
        // if we followed a link, we need the stat of the target, not the link itself
        // but if copy_symlinks was false, 'st' from lstat above is the link. 
        // we need 'stat' to get target permissions.
        struct stat target_st;
        if (stat(src, &target_st) == 0) {
             if (chmod(dest, target_st.st_mode) < 0) {
                 perror("chmod failed");
             }
        } else {
             // fallback if stat fails (unlikely)
             perror("stat failed for chmod");
        }
    }
}

// copy directories recursively from src to dest
void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat st;
    
    // Check source directory
    if (stat(src, &st) < 0) {
        perror("stat src dir failed");
        return;
    }

    // create destination directory
    // if copying permissions, use source mode, otherwise default to 0755.
    mode_t dir_mode = copy_permissions ? st.st_mode : 0755;
    
    if (mkdir(dest, dir_mode) < 0) {
        // mkdir fails if directory exists. 
        // in Nadav's README he said dest should be created new, so error.
        perror("mkdir failed");
        return;
    }

    DIR *d = opendir(src);
    if (!d) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];
        
        // construct full paths
        snprintf(src_path, PATH_MAX, "%s/%s", src, entry->d_name);
        snprintf(dest_path, PATH_MAX, "%s/%s", dest, entry->d_name);

        struct stat entry_st;
        if (lstat(src_path, &entry_st) < 0) {
            perror("lstat entry failed");
            continue;
        }

        // logic to determine recursion vs file copy
        int is_dir = S_ISDIR(entry_st.st_mode);

        // if it's a symlink and we are NOT copying soft link files - symlinks (-l not set),
        // we must check what it points to.
        // if it points to a dir, we work recusrsively.
        if (S_ISLNK(entry_st.st_mode) && !copy_symlinks) {
            struct stat target_st;
            if (stat(src_path, &target_st) == 0) {
                if (S_ISDIR(target_st.st_mode)) {
                    is_dir = 1; 
                }
            }
        }

        if (is_dir) {
            copy_directory(src_path, dest_path, copy_symlinks, copy_permissions);
        } else {
            copy_file(src_path, dest_path, copy_symlinks, copy_permissions);
        }
    }

    closedir(d);
}