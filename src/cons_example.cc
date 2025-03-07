// consumer.cpp
#include "atomic_queue/atomic_queue.h"
#include "shm/xsi_channel.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

constexpr unsigned CAPACITY = 1024;
constexpr unsigned NUM_OF_COND = 1;
using Element = uint32_t;

int main() {
    shm::xsi::XSIChannel<Element, CAPACITY, NUM_OF_COND> channel("/tmp/shared_queue_file",
        shm::xsi::XSI_CHANNEL_EXC);
    auto* sq = channel.GetQueue();

    uint64_t sum = 0;
    int index = 0;
    auto start = std::chrono::high_resolution_clock::now();
    while (true) {
        Element n;
        if (!sq->queue.try_pop(n)) {
            continue;
        }
        //std::cout << n << std::endl;
        if (n == 0)
            break;
        sum += n;
        index++;
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Consumer computed sum: " << sum << std::endl;
    std::cout << "Average time per element (μs): "
              << (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / index
              << std::endl;
    return 0;
}

