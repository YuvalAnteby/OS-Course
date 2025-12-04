#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <random>
#include <algorithm> // For remove
#include "BoundedBuffer.h"

using namespace std;

// --- Configuration Structs ---
struct ProducerConfig {
    int id;
    int numProducts;
    int queueSize;
};

struct Config {
    vector<ProducerConfig> producers;
    int coEditorQueueSize = 0;
};

// --- Globals ---
vector<BoundedBuffer*> producerQueues;
BoundedBuffer* sQueue;
BoundedBuffer* nQueue;
BoundedBuffer* wQueue;
BoundedBuffer* screenQueue;

// --- Helper: Safe String Parser ---
// Removes carriage returns (\r) often found in Windows files running on Linux
string cleanString(string s) {
    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

// Helper to get next non-empty line
bool getNextLine(ifstream& file, string& line) {
    while (getline(file, line)) {
        line = cleanString(line);
        if (!line.empty()) {
            return true;
        }
    }
    return false;
}

// --- Thread Functions ---

void producerFunc(int id, int numProducts, int queueIndex) {
    const string types[] = {"SPORTS", "NEWS", "WEATHER"};
    int counts[] = {0, 0, 0}; 

    for (int i = 0; i < numProducts; ++i) {
        int typeIndex = rand() % 3;
        string type = types[typeIndex];
        int count = counts[typeIndex]++;

        stringstream ss;
        ss << "producer " << id << " " << type << " " << count;
        producerQueues[queueIndex]->insert(ss.str());
    }
    producerQueues[queueIndex]->insert("DONE");
}

void dispatcherFunc(int numProducers) {
    int producersDone = 0;
    vector<bool> producerFinished(numProducers, false);

    while (producersDone < numProducers) {
        for (int i = 0; i < numProducers; ++i) {
            if (producerFinished[i]) continue;

            // Non-blocking check
            if (producerQueues[i]->size() > 0) {
                string msg = producerQueues[i]->remove();

                if (msg == "DONE") {
                    producerFinished[i] = true;
                    producersDone++;
                } else {
                    if (msg.find("SPORTS") != string::npos) {
                        sQueue->insert(msg);
                    } else if (msg.find("NEWS") != string::npos) {
                        nQueue->insert(msg);
                    } else if (msg.find("WEATHER") != string::npos) {
                        wQueue->insert(msg);
                    }
                }
            }
        }
        // Yield to prevent high CPU usage
        this_thread::yield();
    }

    sQueue->insert("DONE");
    nQueue->insert("DONE");
    wQueue->insert("DONE");
}

void coEditorFunc(BoundedBuffer* inQueue) {
    while (true) {
        string msg = inQueue->remove();

        if (msg == "DONE") {
            screenQueue->insert("DONE");
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
        screenQueue->insert(msg);
    }
}

void screenManagerFunc() {
    int doneCount = 0;
    while (true) {
        string msg = screenQueue->remove();

        if (msg == "DONE") {
            doneCount++;
            if (doneCount == 3) {
                cout << "DONE" << endl;
                break;
            }
        } else {
            cout << msg << endl;
        }
    }
}

Config parseConfig(string filename) {
    Config config;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "ERROR: Could not open file: " << filename << endl;
        exit(1);
    }

    string line;
    while (getNextLine(file, line)) {
        if (line.find("PRODUCER") != string::npos) {
            ProducerConfig pc;
            stringstream ss(line);
            string temp;
            ss >> temp >> pc.id;

            if (getNextLine(file, line)) {
                pc.numProducts = stoi(line);
            }

            if (getNextLine(file, line)) {
                size_t eqPos = line.find("=");
                if (eqPos != string::npos) {
                    pc.queueSize = stoi(line.substr(eqPos + 1));
                }
            }
            config.producers.push_back(pc);
            
            // Debug Print
            // cout << "Loaded Producer " << pc.id << ", Products: " << pc.numProducts << ", QSize: " << pc.queueSize << endl;
            
        } else if (line.find("Co-Editor queue size") != string::npos) {
            size_t eqPos = line.find("=");
            if (eqPos != string::npos) {
                config.coEditorQueueSize = stoi(line.substr(eqPos + 1));
            }
        }
    }
    
    if (config.producers.empty()) {
        cerr << "WARNING: No producers found in config file!" << endl;
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <config_file>" << endl;
        return 1;
    }

    srand(time(0));

    // 1. Parse Config
    Config config = parseConfig(argv[1]);

    // 2. Setup Queues
    for (const auto& p : config.producers) {
        producerQueues.push_back(new BoundedBuffer(p.queueSize));
    }
    
    // Fallback if coEditorQueueSize wasn't found or parsed correctly
    int coEditSize = (config.coEditorQueueSize > 0) ? config.coEditorQueueSize : 10;
    
    sQueue = new BoundedBuffer(coEditSize);
    nQueue = new BoundedBuffer(coEditSize);
    wQueue = new BoundedBuffer(coEditSize);
    screenQueue = new BoundedBuffer(coEditSize); 

    // 3. Start Threads
    vector<thread> producers;
    for (size_t i = 0; i < config.producers.size(); ++i) {
        producers.push_back(thread(producerFunc, config.producers[i].id, config.producers[i].numProducts, i));
    }

    thread dispatcher(dispatcherFunc, config.producers.size());
    thread coEditorS(coEditorFunc, sQueue);
    thread coEditorN(coEditorFunc, nQueue);
    thread coEditorW(coEditorFunc, wQueue);
    thread screenManager(screenManagerFunc);

    // 4. Join Threads
    for (auto& t : producers) t.join();
    dispatcher.join();
    coEditorS.join();
    coEditorN.join();
    coEditorW.join();
    screenManager.join();

    // 5. Cleanup
    for (auto p : producerQueues) delete p;
    delete sQueue;
    delete nQueue;
    delete wQueue;
    delete screenQueue;

    return 0;
}