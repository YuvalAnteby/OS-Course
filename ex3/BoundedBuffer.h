#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

// --- Semaphore Declaration ---
class Semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    Semaphore(int init_count = 0);
    void down();
    void up();
};

// --- Bounded Buffer Declaration ---
class BoundedBuffer {
private:
    std::queue<std::string> q;
    int capacity;
    Semaphore* mutex; // Binary semaphore for mutual exclusion
    Semaphore* full;  // Counting semaphore for items count
    Semaphore* empty; // Counting semaphore for empty slots

public:
    BoundedBuffer(int size);
    ~BoundedBuffer();
    void insert(std::string s);
    std::string remove();
    
    // Helper accessors
    int size();
};

#endif