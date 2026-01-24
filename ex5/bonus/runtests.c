#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  printf("====================================\n");
  printf("  XV6 PERMISSIONS TEST SUITE\n");
  printf("====================================\n\n");
  
  printf("Running all permission tests...\n");
  printf("This will take a moment.\n\n");
  
  int pid;
  
  // Test 1: Basic permissions
  printf("[1/6] Running basic permission tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testperm", 0 };
    exec("testperm", args);
    printf("Failed to exec testperm\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  // Test 2: Execute permissions
  printf("[2/6] Running execute permission tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testexec", 0 };
    exec("testexec", args);
    printf("Failed to exec testexec\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  // Test 3: Edge cases
  printf("[3/6] Running edge case tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testpermedge", 0 };
    exec("testpermedge", args);
    printf("Failed to exec testperm_edge\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  // Test 4: Persistence
  printf("[4/6] Running persistence tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testpermpersist", 0 };
    exec("testpermpersist", args);
    printf("Failed to exec testperm_persist\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  // Test 5: Comprehensive
  printf("[5/6] Running comprehensive tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testpermcomprehensive", 0 };
    exec("testpermcomprehensive", args);
    printf("Failed to exec testperm_comprehensive\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  // Test 6: Real-world scenarios
  printf("[6/6] Running real-world scenario tests...\n");
  pid = fork();
  if(pid == 0) {
    char *args[] = { "testpermscenarios", 0 };
    exec("testpermscenarios", args);
    printf("Failed to exec testperm_scenarios\n");
    exit(1);
  }
  wait(0);
  printf("\n");
  
  printf("====================================\n");
  printf("  ALL TESTS COMPLETED\n");
  printf("====================================\n");
  printf("Review the output above for any failures.\n");
  printf("All tests should show PASS status.\n");
  
  exit(0);
}