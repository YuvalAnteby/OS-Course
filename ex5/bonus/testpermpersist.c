#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
  printf("=== Persistence Test ===\n\n");
  
  int fd, perm;
  int pass = 0, fail = 0;
  char buf[100];
  
  // Test 1: Permissions persist after close/reopen
  printf("Test 1: Permissions persist after close/reopen\n");
  fd = open("persist_test.txt", O_CREATE | O_RDWR);
  write(fd, "initial", 7);
  close(fd);
  
  chmod("persist_test.txt", 0500);  // r-x
  perm = getperm("persist_test.txt");
  printf("  Set permissions to: %x\n", perm);
  
  // Reopen and check
  fd = open("persist_test.txt", O_RDONLY);
  if(fd >= 0) {
    close(fd);
    int new_perm = getperm("persist_test.txt");
    printf("  After reopen: %x\n", new_perm);
    if(perm == new_perm) {
      printf("  PASS: Permissions persisted\n\n");
      pass++;
    } else {
      printf("  FAIL: Permissions changed\n\n");
      fail++;
    }
  } else {
    printf("  FAIL: Could not reopen file\n\n");
    fail++;
  }
  
  // Test 2: Permissions persist after writing
  printf("Test 2: Permissions persist after write operations\n");
  chmod("persist_test.txt", 0700);
  perm = getperm("persist_test.txt");
  printf("  Initial permissions: %x\n", perm);
  
  fd = open("persist_test.txt", O_WRONLY);
  if(fd >= 0) {
    write(fd, "modified", 8);
    close(fd);
    
    int new_perm = getperm("persist_test.txt");
    printf("  After write: %x\n", new_perm);
    if(perm == new_perm) {
      printf("  PASS: Permissions unchanged by write\n\n");
      pass++;
    } else {
      printf("  FAIL: Permissions should not change\n\n");
      fail++;
    }
  } else {
    printf("  FAIL: Could not open for write\n\n");
    fail++;
  }
  
  // Test 3: Permissions persist after reading
  printf("Test 3: Permissions persist after read operations\n");
  chmod("persist_test.txt", 0600);
  perm = getperm("persist_test.txt");
  printf("  Initial permissions: %x\n", perm);
  
  fd = open("persist_test.txt", O_RDONLY);
  if(fd >= 0) {
    read(fd, buf, 10);
    close(fd);
    
    int new_perm = getperm("persist_test.txt");
    printf("  After read: %x\n", new_perm);
    if(perm == new_perm) {
      printf("  PASS: Permissions unchanged by read\n\n");
      pass++;
    } else {
      printf("  FAIL: Permissions should not change\n\n");
      fail++;
    }
  } else {
    printf("  FAIL: Could not open for read\n\n");
    fail++;
  }
  
  // Test 4: Create new file, verify default permissions
  printf("Test 4: New files get default permissions (0700)\n");
  fd = open("newfile.txt", O_CREATE | O_RDWR);
  if(fd >= 0) {
    write(fd, "new", 3);
    close(fd);
    
    perm = getperm("newfile.txt");
    printf("  New file permissions: %x (expected 0x1C0)\n", perm);
    if(perm == 0x1C0) {
      printf("  PASS: Default permissions correct\n\n");
      pass++;
    } else {
      printf("  FAIL: Default should be 0x1C0\n\n");
      fail++;
    }
  } else {
    printf("  FAIL: Could not create file\n\n");
    fail++;
  }
  
  // Test 5: Verify multiple files maintain independent permissions
  printf("Test 5: Multiple files have independent permissions\n");
  fd = open("file1.txt", O_CREATE | O_RDWR);
  write(fd, "one", 3);
  close(fd);
  
  fd = open("file2.txt", O_CREATE | O_RDWR);
  write(fd, "two", 3);
  close(fd);
  
  chmod("file1.txt", 0400);  // read-only
  chmod("file2.txt", 0200);  // write-only
  
  int perm1 = getperm("file1.txt");
  int perm2 = getperm("file2.txt");
  
  printf("  file1.txt: %x\n", perm1);
  printf("  file2.txt: %x\n", perm2);
  
  if(perm1 == 0x100 && perm2 == 0x80) {
    printf("  PASS: Files maintain independent permissions\n\n");
    pass++;
  } else {
    printf("  FAIL: Permissions should be independent\n\n");
    fail++;
  }
  
  printf("=== Results ===\n");
  printf("Passed: %d\n", pass);
  printf("Failed: %d\n", fail);
  printf("Total:  %d\n", pass + fail);
  
  exit(0);
}