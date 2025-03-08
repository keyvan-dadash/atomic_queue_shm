// producer.cpp
#include "atomic_queue/atomic_queue.h"
#include "shm/posix_channel.h"
#include "shm/xsi_channel.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

//constexpr unsigned N = 1000000000;
constexpr unsigned N = 1000000;
constexpr unsigned CAPACITY = 1024;
constexpr unsigned NUM_OF_COND = 1;
using Element = uint32_t;

int main() {
    //int op = shm::xsi::XSI_CHANNEL_CREATE | shm::xsi::XSI_CHANNEL_CLEAN;
    int op = shm::posix::POSIX_CHANNEL_CREATE | shm::posix::POSIX_CHANNEL_CLEAN;
    //shm::xsi::XSIChannel<Element, CAPACITY, NUM_OF_COND> channel("/tmp/shared_queue_file", op);
    shm::posix::POSIXChannel<Element, CAPACITY, NUM_OF_COND> channel("shared_queue_file", op);
    auto sq = channel.GetQueue();

    for (Element n = N; n > 0; --n) {
/*        auto status = sq->queue.try_push(n);*/
        /*if (!status) {*/
            /*cond.wait(&mu);*/
        /*}*/

        while (!sq->queue.try_push(n)) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        //sq->queue.push(n);
        //std::cout << n << std::endl;
    }

    sq->queue.push(0);
    sq->queue.push(0);
    sq->queue.push(0);
    sq->queue.push(0);
    sq->queue.push(0);
    sq->queue.push(0);

    std::cout << "Producer finished pushing " << N << " elements." << std::endl;
    return 0;
}

