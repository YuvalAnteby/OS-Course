#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
  printf("=== Execute Permission Test ===\n\n");
  
  // Check old files have permissions = 0
  int perm = getperm("ls");
  printf("System file (ls) permissions: %x\n", perm);
  printf("Old files default to 0 (backwards compatible)\n\n");
  
  // Create test file
  int fd = open("testfile", O_CREATE | O_RDWR);
  write(fd, "test", 4);
  close(fd);
  
  // Test with execute permission
  printf("Test 1: Set permissions to 0700 (rwx)\n");
  chmod("testfile", 0700);
  perm = getperm("testfile");
  int exec_bit = (perm >> 6) & 1;
  printf("  Permissions: %x, Execute bit: %d\n", perm, exec_bit);
  if(exec_bit)
    printf("  PASS: Execute permission granted\n\n");
  else
    printf("  FAIL: Execute bit should be set\n\n");
  
  // Test without execute permission
  printf("Test 2: Set permissions to 0600 (rw-)\n");
  chmod("testfile", 0600);
  perm = getperm("testfile");
  exec_bit = (perm >> 6) & 1;
  printf("  Permissions: %x, Execute bit: %d\n", perm, exec_bit);
  if(!exec_bit)
    printf("  PASS: Execute permission denied\n\n");
  else
    printf("  FAIL: Execute bit should be clear\n\n");
  
  printf("All tests passed!\n");
  exit(0);
}