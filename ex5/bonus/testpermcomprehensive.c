#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Test a specific permission combination
int test_permission(int mode, int should_read, int should_write, char* desc)
{
  int fd;
  
  printf("  Testing %s (0%x): ", desc, mode);
  
  // Create and set permissions
  fd = open("testfile.tmp", O_CREATE | O_RDWR);
  if(fd < 0) {
    printf("FAIL - cannot create\n");
    return 0;
  }
  write(fd, "test", 4);
  close(fd);
  
  chmod("testfile.tmp", mode);
  
  // Test read
  fd = open("testfile.tmp", O_RDONLY);
  int can_read = (fd >= 0);
  if(fd >= 0) close(fd);
  
  // Test write
  fd = open("testfile.tmp", O_WRONLY);
  int can_write = (fd >= 0);
  if(fd >= 0) close(fd);
  
  // Check results
  int pass = 1;
  if(can_read != should_read) {
    printf("FAIL - read access incorrect ");
    pass = 0;
  }
  if(can_write != should_write) {
    printf("FAIL - write access incorrect ");
    pass = 0;
  }
  
  if(pass) {
    printf("PASS\n");
  } else {
    printf("(can_read=%d expected=%d, can_write=%d expected=%d)\n",
           can_read, should_read, can_write, should_write);
  }
  
  return pass;
}

int main(int argc, char *argv[])
{
  printf("=== Comprehensive Permission Test ===\n\n");
  
  int total = 0, passed = 0;
  
  printf("Testing all owner permission combinations:\n");
  
  // Test all 8 combinations of rwx for owner (bits 8-6)
  total++; passed += test_permission(0000, 0, 0, "--- (none)");
  total++; passed += test_permission(0100, 0, 0, "--x (exec only)");
  total++; passed += test_permission(0200, 0, 1, "-w- (write only)");
  total++; passed += test_permission(0300, 0, 1, "-wx (write+exec)");
  total++; passed += test_permission(0400, 1, 0, "r-- (read only)");
  total++; passed += test_permission(0500, 1, 0, "r-x (read+exec)");
  total++; passed += test_permission(0600, 1, 1, "rw- (read+write)");
  total++; passed += test_permission(0700, 1, 1, "rwx (all)");
  
  printf("\nTesting O_RDWR access:\n");
  
  // Create test file
  int fd = open("rdwr_test.txt", O_CREATE | O_RDWR);
  write(fd, "test", 4);
  close(fd);
  
  // Test with read-only permission
  chmod("rdwr_test.txt", 0400);
  fd = open("rdwr_test.txt", O_RDWR);
  total++;
  if(fd < 0) {
    printf("  PASS: O_RDWR denied with r-- permissions\n");
    passed++;
  } else {
    printf("  FAIL: O_RDWR should be denied with r-- permissions\n");
    close(fd);
  }
  
  // Test with write-only permission
  chmod("rdwr_test.txt", 0200);
  fd = open("rdwr_test.txt", O_RDWR);
  total++;
  if(fd < 0) {
    printf("  PASS: O_RDWR denied with -w- permissions\n");
    passed++;
  } else {
    printf("  FAIL: O_RDWR should be denied with -w- permissions\n");
    close(fd);
  }
  
  // Test with read+write permission
  chmod("rdwr_test.txt", 0600);
  fd = open("rdwr_test.txt", O_RDWR);
  total++;
  if(fd >= 0) {
    printf("  PASS: O_RDWR allowed with rw- permissions\n");
    close(fd);
    passed++;
  } else {
    printf("  FAIL: O_RDWR should be allowed with rw- permissions\n");
  }
  
  printf("\nTesting chmod boundary conditions:\n");
  
  // Test chmod with values > 0777
  fd = open("boundary.txt", O_CREATE | O_RDWR);
  write(fd, "test", 4);
  close(fd);
  
  chmod("boundary.txt", 0xFFF);  // Set all bits
  int perm = getperm("boundary.txt");
  total++;
  // Should only keep lower 9 bits (0x1FF)
  if((perm & 0x1FF) == 0x1FF) {
    printf("  PASS: chmod masks to 9 bits correctly\n");
    passed++;
  } else {
    printf("  FAIL: chmod should mask to 9 bits (got %x)\n", perm);
  }
  
  printf("\n=== Results ===\n");
  printf("Passed: %d / %d\n", passed, total);
  printf("Failed: %d / %d\n", total - passed, total);
  
  if(passed == total) {
    printf("\n*** ALL TESTS PASSED! ***\n");
  } else {
    printf("\n*** SOME TESTS FAILED ***\n");
  }
  
  exit(0);
}