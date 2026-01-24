#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void print_test_header(char* name) {
  printf("\n--- %s ---\n", name);
}

int main(int argc, char *argv[])
{
  printf("=== Real-World Scenarios Test ===\n");
  printf("This test simulates practical permission use cases\n");
  
  int fd, perm;
  char buf[100];
  int pass = 0, fail = 0;
  
  // Scenario 1: Read-only configuration file
  print_test_header("Scenario 1: Read-only config file");
  printf("Creating a read-only configuration file...\n");
  
  fd = open("config.txt", O_CREATE | O_RDWR);
  write(fd, "setting=value\n", 14);
  close(fd);
  
  chmod("config.txt", 0400);  // Read-only
  printf("Set to read-only (0400)\n");
  
  // Should be able to read
  fd = open("config.txt", O_RDONLY);
  if(fd >= 0) {
    int n = read(fd, buf, 100);
    buf[n] = 0;
    printf("Read succeeded: %s", buf);
    close(fd);
    pass++;
  } else {
    printf("FAIL: Should be able to read\n");
    fail++;
  }
  
  // Should NOT be able to write
  fd = open("config.txt", O_WRONLY);
  if(fd < 0) {
    printf("Write blocked successfully\n");
    printf("PASS: Config file protected from modification\n");
    pass++;
  } else {
    printf("FAIL: Should not be able to write\n");
    close(fd);
    fail++;
  }
  
  // Scenario 2: Log file (append-only simulation)
  print_test_header("Scenario 2: Write-only log file");
  printf("Creating a write-only log file...\n");
  
  fd = open("logfile.txt", O_CREATE | O_RDWR);
  write(fd, "Log started\n", 12);
  close(fd);
  
  chmod("logfile.txt", 0200);  // Write-only
  printf("Set to write-only (0200)\n");
  
  // Should be able to write
  fd = open("logfile.txt", O_WRONLY);
  if(fd >= 0) {
    write(fd, "New log entry\n", 14);
    close(fd);
    printf("Write succeeded\n");
    pass++;
  } else {
    printf("FAIL: Should be able to write\n");
    fail++;
  }
  
  // Should NOT be able to read
  fd = open("logfile.txt", O_RDONLY);
  if(fd < 0) {
    printf("Read blocked successfully\n");
    printf("PASS: Log file hidden from reading\n");
    pass++;
  } else {
    printf("FAIL: Should not be able to read\n");
    close(fd);
    fail++;
  }
  
  // Scenario 3: Temporary working file
  print_test_header("Scenario 3: Temporary working file");
  printf("Creating a temporary file with full access...\n");
  
  fd = open("temp.txt", O_CREATE | O_RDWR);
  write(fd, "temporary data", 14);
  close(fd);
  
  chmod("temp.txt", 0700);  // Full access
  perm = getperm("temp.txt");
  printf("Set to rwx (0700), permissions: %x\n", perm);
  
  // Should be able to read
  fd = open("temp.txt", O_RDONLY);
  if(fd >= 0) {
    read(fd, buf, 14);
    close(fd);
    printf("Read succeeded\n");
    pass++;
  } else {
    printf("FAIL: Should be able to read\n");
    fail++;
  }
  
  // Should be able to write
  fd = open("temp.txt", O_WRONLY);
  if(fd >= 0) {
    write(fd, "modified", 8);
    close(fd);
    printf("Write succeeded\n");
    pass++;
  } else {
    printf("FAIL: Should be able to write\n");
    fail++;
  }
  
  printf("PASS: Temporary file fully accessible\n");
  
  // Scenario 4: Locking down sensitive data
  print_test_header("Scenario 4: Securing sensitive data");
  printf("Creating sensitive data file...\n");
  
  fd = open("secrets.txt", O_CREATE | O_RDWR);
  write(fd, "password=secret123", 18);
  close(fd);
  
  printf("Initially writable, reading and modifying...\n");
  chmod("secrets.txt", 0600);
  
  // Read the secret
  fd = open("secrets.txt", O_RDONLY);
  read(fd, buf, 18);
  close(fd);
  printf("Read secret data\n");
  
  // Now lock it down completely
  printf("Locking down with 0000 permissions...\n");
  chmod("secrets.txt", 0000);
  
  fd = open("secrets.txt", O_RDONLY);
  if(fd < 0) {
    printf("Read blocked\n");
    pass++;
  } else {
    printf("FAIL: Should not be able to read\n");
    close(fd);
    fail++;
  }
  
  fd = open("secrets.txt", O_WRONLY);
  if(fd < 0) {
    printf("Write blocked\n");
    printf("PASS: Sensitive data fully protected\n");
    pass++;
  } else {
    printf("FAIL: Should not be able to write\n");
    close(fd);
    fail++;
  }
  
  // Scenario 5: Progressive restriction
  print_test_header("Scenario 5: Progressive permission restriction");
  printf("Creating file with progressive restrictions...\n");
  
  fd = open("progressive.txt", O_CREATE | O_RDWR);
  write(fd, "data", 4);
  close(fd);
  
  // Start with full access
  chmod("progressive.txt", 0700);
  printf("Stage 1: rwx (0700) - Full access\n");
  fd = open("progressive.txt", O_RDWR);
  if(fd >= 0) {
    close(fd);
    printf("  Can read and write\n");
    pass++;
  } else {
    printf("  FAIL: Should have full access\n");
    fail++;
  }
  
  // Remove write
  chmod("progressive.txt", 0500);
  printf("Stage 2: r-x (0500) - Read-only\n");
  fd = open("progressive.txt", O_RDONLY);
  if(fd >= 0) {
    close(fd);
    printf("  Can read\n");
  } else {
    printf("  FAIL: Should be able to read\n");
    fail++;
    goto skip;
  }
  fd = open("progressive.txt", O_WRONLY);
  if(fd < 0) {
    printf("  Cannot write\n");
    pass++;
  } else {
    printf("  FAIL: Should not be able to write\n");
    close(fd);
    fail++;
  }
  
  // Remove all access
  chmod("progressive.txt", 0000);
  printf("Stage 3: --- (0000) - No access\n");
  fd = open("progressive.txt", O_RDONLY);
  if(fd < 0) {
    printf("  Cannot access\n");
    printf("  PASS: Progressive restriction worked\n");
    pass++;
  } else {
    printf("  FAIL: Should not be able to access\n");
    close(fd);
    fail++;
  }
  
skip:
  printf("\n=== Final Results ===\n");
  printf("Scenarios passed: %d\n", pass);
  printf("Scenarios failed: %d\n", fail);
  printf("Total tests: %d\n", pass + fail);
  
  if(fail == 0) {
    printf("\n*** SUCCESS! All real-world scenarios work correctly ***\n");
  } else {
    printf("\n*** Some scenarios failed - review implementation ***\n");
  }
  
  exit(0);
}