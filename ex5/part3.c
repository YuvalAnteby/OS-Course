#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-l] [-p] <source_directory> <destination_directory>\n", prog_name);
    fprintf(stderr, "  -l: Copy soft links as links\n");
    fprintf(stderr, "  -p: Copy file permissions\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int copy_symlinks = 0;
    int copy_permissions = 0;

    // using getopt to parse flags -l and -p
    while ((opt = getopt(argc, argv, "lp")) != -1) {
        // if the -l flag is off, the code uses regular open() which automatically follows links of soft links, 
        // copying the content of the target file. 
        // If -l is on, it uses readlink and soft links to recreate the pointer itself.
        switch (opt) {
            case 'l':
                copy_symlinks = 1;
                break;
            case 'p':
                copy_permissions = 1;
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // ensure we have exactly 2 path arguments left (src and dest)
    if (optind + 2 != argc) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[optind];
    const char *dest_dir = argv[optind + 1];

    copy_directory(src_dir, dest_dir, copy_symlinks, copy_permissions);

    return 0;
}