#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
  int fd;
  
  printf("Test 1: Create file with full permissions\n");
  
  // Create a file (should have default 0700 permissions)
  fd = open("testfile.txt", O_CREATE | O_RDWR);
  if(fd < 0){
    printf("FAIL: Could not create file\n");
    exit(1);
  }
  write(fd, "hello", 5);
  close(fd);
  
  // Check permissions
  int perm = getperm("testfile.txt");
  printf("Default permissions: %x (expected 0700)\n", perm);
  
  // Test: Read the file (should work)
  fd = open("testfile.txt", O_RDONLY);
  if(fd < 0){
    printf("FAIL: Could not read file\n");
    exit(1);
  }
  printf("PASS: Can read file with permission 0700\n");
  close(fd);
  
  printf("\nTest 2: Remove read permission\n");
  
  // Change to write-only (0200)
  chmod("testfile.txt", 0200);
  perm = getperm("testfile.txt");
  printf("New permissions: %x (expected 0200)\n", perm);
  
  // Try to read (should fail)
  fd = open("testfile.txt", O_RDONLY);
  if(fd < 0){
    printf("PASS: Cannot read file without read permission\n");
  } else {
    printf("FAIL: Should not be able to read file\n");
    close(fd);
  }
  
  printf("\nTest 3: Remove write permission\n");
  
  // Change to read-only (0400)
  chmod("testfile.txt", 0400);
  perm = getperm("testfile.txt");
  printf("New permissions: %x (expected 0400)\n", perm);
  
  // Try to write (should fail)
  fd = open("testfile.txt", O_WRONLY);
  if(fd < 0){
    printf("PASS: Cannot write to file without write permission\n");
  } else {
    printf("FAIL: Should not be able to write to file\n");
    close(fd);
  }
  
  // But reading should work
  fd = open("testfile.txt", O_RDONLY);
  if(fd >= 0){
    printf("PASS: Can read file with read permission\n");
    close(fd);
  } else {
    printf("FAIL: Should be able to read file\n");
  }
  
  printf("\nAll basic tests completed!\n");
  exit(0);
}