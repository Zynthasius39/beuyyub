#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
int threadQue = 0;

void long_work(int id) {
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [id]{ return id == threadQue; });

    // Simulate long work
    std::cout << "Thread " << id << " is working" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Thread " << id << " finished work" << std::endl;

    threadQue = (threadQue + 1) % 3; // Known thread number
        cv.notify_all();
}

int main() {
    unsigned int maxThreads = std::thread::hardware_concurrency();
    std::cout << "Max threads supported: " << maxThreads << std::endl;
    int id = 0;
    std::thread(long_work, id++).detach();
    std::thread(long_work, id++).detach();
    std::thread(long_work, id++).detach();
    return 0;
}