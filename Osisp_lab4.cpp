#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <iomanip>
#include <string>
#include <atomic>
#include <random>


const int amount_of_phil = 4;  
const int runtime = 10;     
const int bantime = 555;  

std::vector<HANDLE> forks(amount_of_phil);
std::vector<std::thread> philosophers(amount_of_phil);

struct PhilosopherStats {
    double thinkingTime = 0;
    double eatingTime = 0;
    double waitingTime = 0;
    int successfulMeals = 0;
    int failedMeals = 0;
};

std::vector<PhilosopherStats> philosopherStats(amount_of_phil);

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> thinkTime(1500, 2500);
std::uniform_int_distribution<> eatTime(1500, 2000);

void philosopherFunction(int philosopherNum, std::atomic<bool>& running) {
    int leftFork = philosopherNum;
    int rightFork = (philosopherNum + 1) % amount_of_phil;

    while (running.load()) {
        auto startThinking = std::chrono::steady_clock::now();
        std::cout << "Philosopher " << philosopherNum << " is thinking...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkTime(gen)));
        auto endThinking = std::chrono::steady_clock::now();
        philosopherStats[philosopherNum].thinkingTime += std::chrono::duration<double>(endThinking - startThinking).count();

        std::cout << "Philosopher " << philosopherNum << " is waiting for forks...\n";
        auto startWaiting = std::chrono::steady_clock::now();
        DWORD leftResult = WaitForSingleObject(forks[leftFork], bantime);
        DWORD rightResult = WaitForSingleObject(forks[rightFork], bantime);

        
        if (leftResult == WAIT_OBJECT_0 && rightResult == WAIT_OBJECT_0) {
            auto endWaiting = std::chrono::steady_clock::now();
            philosopherStats[philosopherNum].waitingTime += std::chrono::duration<double>(endWaiting - startWaiting).count();

            auto startEating = std::chrono::steady_clock::now();
            std::cout << "Philosopher " << philosopherNum << " is eating...\n";
            philosopherStats[philosopherNum].successfulMeals++;
            std::this_thread::sleep_for(std::chrono::milliseconds(eatTime(gen)));
            auto endEating = std::chrono::steady_clock::now();
            philosopherStats[philosopherNum].eatingTime += std::chrono::duration<double>(endEating - startEating).count();

            ReleaseMutex(forks[leftFork]);
            ReleaseMutex(forks[rightFork]);
        }
        else {
            philosopherStats[philosopherNum].failedMeals++;
            auto endWaiting = std::chrono::steady_clock::now();
            philosopherStats[philosopherNum].waitingTime += std::chrono::duration<double>(endWaiting - startWaiting).count();

            //отпускаем
            if (leftResult == WAIT_OBJECT_0) ReleaseMutex(forks[leftFork]);
            if (rightResult == WAIT_OBJECT_0) ReleaseMutex(forks[rightFork]);
        }
    }
}

int main() {
    for (int i = 0; i < amount_of_phil; ++i) {
        forks[i] = CreateMutex(NULL, FALSE, NULL);
    }

    std::atomic<bool> running(true);

    for (int i = 0; i < amount_of_phil; ++i) {
        philosophers[i] = std::thread(philosopherFunction, i, std::ref(running));
    }


    std::this_thread::sleep_for(std::chrono::seconds(runtime));
    running.store(false);

    for (auto& philosopher : philosophers) {
        philosopher.join();
    }

    std::cout << "\n=== Results ===\n";
    for (int i = 0; i < amount_of_phil; ++i) {
        const auto& stats = philosopherStats[i];
        double totalTime = stats.thinkingTime + stats.eatingTime + stats.waitingTime;

        std::cout << "Philosopher " << i << ":\n"
            << "  Thinking Time: " << stats.thinkingTime << "s (" << std::fixed << std::setprecision(2)
            << (stats.thinkingTime / totalTime * 100) << "%)\n"
            << "  Eating Time: " << stats.eatingTime << "s (" << (stats.eatingTime / totalTime * 100) << "%)\n"
            << "  Waiting Time: " << stats.waitingTime << "s (" << (stats.waitingTime / totalTime * 100) << "%)\n"
            << "  Successful Meals: " << stats.successfulMeals << "\n"
            << "  Failed Meals: " << stats.failedMeals << "\n";
    }

    for (int i = 0; i < amount_of_phil; ++i) {
        CloseHandle(forks[i]);
    }

    return 0;
}
