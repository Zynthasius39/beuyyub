#include <iostream>
#include <thread>
#include <future>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::mutex mtx2;
std::queue<std::function<void()>> tasks;
// std::queue<std::vector<uint8_t>> pcms;
std::queue<std::string> pcms;
std::condition_variable cv;
std::condition_variable cv2;
void worker() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !tasks.empty(); });

        std::function<void()> task = tasks.front();
        tasks.pop();
        lock.unlock();

        std::async(std::launch::async, task);
    }
}

void streamer(std::queue<std::string> &pcms) {   
    while (true) {
        std::unique_lock<std::mutex> lock(mtx2);
        cv2.wait(lock, [&pcms] { return !pcms.empty(); });
        std::string pcm = pcms.front();
        pcms.pop();
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Streamed: " << pcm << std::endl;
        // std::cout << "Streamed: " << std::endl;
    }
}

template <typename Func, typename... Args>
void add_task(Func&& func, Args&&... args) {
    std::function<void()> task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(task);
    cv.notify_one();
}

void download(std::string str) {
    std::this_thread::sleep_for(std::chrono::seconds(rand() % 5));
    std::cout << "Did the job: " << str << std::endl;
    std::lock_guard<std::mutex> lock(mtx);
    pcms.push(str+".mp3");
    cv2.notify_one();
}

int main() {
    srand(time(NULL));
    std::thread t(worker);
    std::thread s(streamer, std::ref(pcms));
    t.detach();
    s.detach();
    int n;
    std::cin >> n;
    std::string arr[n];
    for (int i = 0; i < n; i++)
        std::cin >> arr[i];
    for (int i = 0; i < n; i++)
        add_task(download, arr[i]);

    std::this_thread::sleep_for(std::chrono::minutes(5));

    return 0;
}