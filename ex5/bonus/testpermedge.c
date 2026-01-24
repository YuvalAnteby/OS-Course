#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
  printf("=== Edge Cases Test ===\n\n");
  
  int fd, perm;
  int pass = 0, fail = 0;
  
  // Test 1: All permission bits set
  printf("Test 1: Set all permission bits (0777)\n");
  fd = open("test_all.txt", O_CREATE | O_RDWR);
  write(fd, "test", 4);
  close(fd);
  chmod("test_all.txt", 0777);
  perm = getperm("test_all.txt");
  printf("  Permissions: %x (expected 0x1FF)\n", perm);
  if(perm == 0x1FF) {
    printf("  PASS\n\n");
    pass++;
  } else {
    printf("  FAIL\n\n");
    fail++;
  }
  
  // Test 2: No permission bits set
  printf("Test 2: Clear all permission bits (0000)\n");
  chmod("test_all.txt", 0000);
  perm = getperm("test_all.txt");
  printf("  Permissions: %x (expected 0x0)\n", perm);
  if(perm == 0x0) {
    printf("  PASS\n\n");
    pass++;
  } else {
    printf("  FAIL\n\n");
    fail++;
  }
  
  // Test 3: Try to open with no permissions (should fail)
  printf("Test 3: Try to open file with no permissions\n");
  fd = open("test_all.txt", O_RDONLY);
  if(fd < 0) {
    printf("  PASS: Open denied with 0000 permissions\n\n");
    pass++;
  } else {
    printf("  FAIL: Should not be able to open\n\n");
    close(fd);
    fail++;
  }
  
  // Test 4: Restore permissions and verify access
  printf("Test 4: Restore permissions and verify\n");
  chmod("test_all.txt", 0700);
  fd = open("test_all.txt", O_RDONLY);
  if(fd >= 0) {
    printf("  PASS: Can open after restoring permissions\n\n");
    close(fd);
    pass++;
  } else {
    printf("  FAIL: Should be able to open\n\n");
    fail++;
  }
  
  // Test 5: Test individual permission bits
  printf("Test 5: Test individual permission bits\n");
  
  // Only read (0400)
  chmod("test_all.txt", 0400);
  perm = getperm("test_all.txt");
  int read_bit = (perm >> 8) & 1;
  int write_bit = (perm >> 7) & 1;
  int exec_bit = (perm >> 6) & 1;
  printf("  0400: r=%d w=%d x=%d ", read_bit, write_bit, exec_bit);
  if(read_bit && !write_bit && !exec_bit) {
    printf("PASS\n");
    pass++;
  } else {
    printf("FAIL\n");
    fail++;
  }
  
  // Only write (0200)
  chmod("test_all.txt", 0200);
  perm = getperm("test_all.txt");
  read_bit = (perm >> 8) & 1;
  write_bit = (perm >> 7) & 1;
  exec_bit = (perm >> 6) & 1;
  printf("  0200: r=%d w=%d x=%d ", read_bit, write_bit, exec_bit);
  if(!read_bit && write_bit && !exec_bit) {
    printf("PASS\n");
    pass++;
  } else {
    printf("FAIL\n");
    fail++;
  }
  
  // Only execute (0100)
  chmod("test_all.txt", 0100);
  perm = getperm("test_all.txt");
  read_bit = (perm >> 8) & 1;
  write_bit = (perm >> 7) & 1;
  exec_bit = (perm >> 6) & 1;
  printf("  0100: r=%d w=%d x=%d ", read_bit, write_bit, exec_bit);
  if(!read_bit && !write_bit && exec_bit) {
    printf("PASS\n\n");
    pass++;
  } else {
    printf("FAIL\n\n");
    fail++;
  }
  
  // Test 6: Multiple chmod operations
  printf("Test 6: Multiple sequential chmod operations\n");
  chmod("test_all.txt", 0700);
  chmod("test_all.txt", 0600);
  chmod("test_all.txt", 0400);
  perm = getperm("test_all.txt");
  printf("  Final permissions: %x (expected 0x100)\n", perm);
  if(perm == 0x100) {
    printf("  PASS\n\n");
    pass++;
  } else {
    printf("  FAIL\n\n");
    fail++;
  }
  
  // Test 7: getperm on non-existent file
  printf("Test 7: getperm on non-existent file\n");
  perm = getperm("nonexistent_file_xyz.txt");
  if(perm < 0) {
    printf("  PASS: Returns error for non-existent file\n\n");
    pass++;
  } else {
    printf("  FAIL: Should return error\n\n");
    fail++;
  }
  
  printf("=== Results ===\n");
  printf("Passed: %d\n", pass);
  printf("Failed: %d\n", fail);
  printf("Total:  %d\n", pass + fail);
  
  exit(0);
}