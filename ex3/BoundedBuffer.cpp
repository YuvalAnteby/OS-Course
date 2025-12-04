#include "BoundedBuffer.h"

// --- Semaphore Implementation ---

Semaphore::Semaphore(int init_count) : count(init_count) {}

void Semaphore::down() {
    std::unique_lock<std::mutex> lock(mtx);
    while (count == 0) {
        cv.wait(lock);
    }
    count--;
}

void Semaphore::up() {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    cv.notify_one();
}

// --- Bounded Buffer Implementation ---

BoundedBuffer::BoundedBuffer(int size) : capacity(size) {
    mutex = new Semaphore(1);    // Mutex starts at 1 (unlocked)
    full = new Semaphore(0);     // Initially 0 items
    empty = new Semaphore(size); // Initially 'size' empty slots
}

BoundedBuffer::~BoundedBuffer() {
    delete mutex;
    delete full;
    delete empty;
}

void BoundedBuffer::insert(std::string s) {
    empty->down(); // Wait for an empty slot
    mutex->down(); // Lock critical section
    
    q.push(s);
    
    mutex->up();   // Unlock critical section
    full->up();    // Signal that there is a new item
}

std::string BoundedBuffer::remove() {
    full->down();  // Wait for an item
    mutex->down(); // Lock critical section
    
    std::string s = q.front();
    q.pop();
    
    mutex->up();   // Unlock critical section
    empty->up();   // Signal that there is an empty slot
    
    return s;
}

int BoundedBuffer::size() {
    // Note: Standard queue::size() is not atomic, 
    // but we use this primarily for the Dispatcher's heuristic check.
    // For strict thread safety on size(), you would surround this with the mutex,
    // but that could cause the Dispatcher to block if the mutex is held by a Producer.
    return q.size();
}