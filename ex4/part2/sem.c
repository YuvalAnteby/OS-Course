#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// semaphore struct
struct sem {
  struct spinlock lock; // Protects the value
  int value;            // The resource count
  int init_value;
  int used;             // Allocation flag (1 = active, 0 = free)
};

// Global array of semaphores
struct sem sems[MAX_SEMS];

// Meta lock to protect the allocation of slots in the array
struct spinlock sem_array_lock;


// init is called from main during xv6 boot
void
seminit(void)
{
  int i;
  
  initlock(&sem_array_lock, "sem_array");
  
  for(i = 0; i < MAX_SEMS; i++){
    initlock(&sems[i].lock, "sem");
    sems[i].used = 0;
    sems[i].value = 0;
  }
}

// Semaphore management functions

// Finds a free slot, initializes it, and returns the ID (index)
int
sem_create(int init_value)
{
int i;
  acquire(&sem_array_lock);
  for(i = 0; i < MAX_SEMS; i++){
    if(sems[i].used == 0){
      sems[i].used = 1;
      sems[i].value = init_value;
      sems[i].init_value = init_value;
      release(&sem_array_lock);
      return i;
    }
  }
  release(&sem_array_lock);
  return -1;
}

// Frees the semaphore slot
int
sem_free(int id)
{
  if(id < 0 || id >= MAX_SEMS)
    return -1;

  acquire(&sem_array_lock);

  if(sems[id].used == 0){
    release(&sem_array_lock);
    return -1;
  }

  acquire(&sems[id].lock);

  if(sems[id].value != sems[id].init_value){
      release(&sems[id].lock);
      release(&sem_array_lock);
      return -1;
  }

  sems[id].used = 0;
  sems[id].value = 0;
  sems[id].init_value = 0; // Clean up

  release(&sems[id].lock);
  release(&sem_array_lock);

  return 0;
}

// Sync Logic

int
sem_down(int id)
{
  if(id < 0 || id >= MAX_SEMS)
    return -1;

  acquire(&sems[id].lock);

  // If used check fails, release and fail
  if(sems[id].used == 0){
    release(&sems[id].lock);
    return -1;
  }

  // THE CRITICAL LOOP
  while(sems[id].value == 0){
    // sleep atomically releases the lock and sleeps.
    sleep(&sems[id], &sems[id].lock);
  }

  sems[id].value--;
  
  release(&sems[id].lock);
  return 0;
}

int
sem_up(int id)
{
  if(id < 0 || id >= MAX_SEMS)
    return -1;

  acquire(&sems[id].lock);

  if(sems[id].used == 0){
    release(&sems[id].lock);
    return -1;
  }

  sems[id].value++;
  
  // Wake up ALL processes sleeping on this specific semaphore channel
  wakeup(&sems[id]);

  release(&sems[id].lock);
  return 0;
}

// Sys call Wrappers

uint64
sys_sem_create(void)
{
  int init_value;
  argint(0, &init_value);
  return sem_create(init_value);
}

uint64
sys_sem_free(void)
{
  int id;
  argint(0, &id);
  return sem_free(id);
}

uint64
sys_sem_down(void)
{
  int id;
  argint(0, &id);
  return sem_down(id);
}

uint64
sys_sem_up(void)
{
  int id;
  argint(0, &id);
  return sem_up(id);
}