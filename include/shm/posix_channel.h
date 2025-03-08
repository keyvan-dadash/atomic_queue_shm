#ifndef POSIX_CHANNEL_H
#define POSIX_CHANNEL_H

#include "atomic_queue/atomic_queue.h"
#include "shm/posix_shm_area.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

namespace shm {
namespace posix {

const std::string mutex_prefix = "_mutex";

enum POSIX_CHANNEL_OPS {
    POSIX_CHANNEL_CREATE = 0x1,
    POSIX_CHANNEL_EXC = 0x2,
    POSIX_CHANNEL_CLEAN = 0x4
};

template<typename T, unsigned CHANNEL_SIZE, unsigned NUM_OF_COND>
class POSIXChannel {
public:
    POSIXChannel(std::string name, int op)
        : name_(name)
        , shm_queue_(nullptr)
    {
        if(op & POSIX_CHANNEL_CLEAN) {
            clean_existing_files();
        }
        if(op & POSIX_CHANNEL_EXC) {
            if(!file_exists(name_) || !file_exists(name_ + mutex_prefix)) {
                throw std::runtime_error("Required shared memory objects do not exist for attach.");
            }
        }
        else if(op & POSIX_CHANNEL_CREATE) {
            create_files();
        }

        if(name_.front() != '/') {
            name_ = "/" + name_;
        }

        shm_ = std::make_unique<shm::posix::POSIXSharedMemory<SHMQueue>>(name_, sizeof(SHMQueue));
        shm_queue_ = shm_->AttachSHM();

        if(op & POSIX_CHANNEL_CREATE) {
            memset(shm_queue_, 0, sizeof(SHMQueue));
            new (shm_queue_) SHMQueue();
            init_mutexes();
        }
    }

    ~POSIXChannel() {
        shm_->DeattachSHM();
    }

    struct SHMQueue {
        pthread_mutex_t mutex[NUM_OF_COND];
        POSIXConditionVariable cond[NUM_OF_COND];
        atomic_queue::AtomicQueue2<T, CHANNEL_SIZE> queue;
    };

    SHMQueue* GetQueue() {
        return shm_queue_;
    }

private:
    bool file_exists(const std::string& filename) {
        return (access(filename.c_str(), F_OK) != -1);
    }
    void clean_existing_files() {
        std::remove(name_.c_str());
        std::remove((name_ + mutex_prefix).c_str());
    }
    void create_files() {
        if(!file_exists(name_)) {
            std::ofstream ofs(name_);
            ofs.close();
        }
        if(!file_exists(name_ + mutex_prefix)) {
            std::ofstream ofs(name_ + mutex_prefix);
            ofs.close();
        }
    }

    void init_mutexes() {
        pthread_mutexattr_t attr;
        if(pthread_mutexattr_init(&attr) != 0) {
            throw std::runtime_error("pthread_mutexattr_init failed");
        }
        if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
            throw std::runtime_error("pthread_mutexattr_setpshared failed");
        }
        for(unsigned i = 0; i < NUM_OF_COND; ++i) {
            if(pthread_mutex_init(&shm_queue_->mutex[i], &attr) != 0) {
                throw std::runtime_error("pthread_mutex_init failed for mutex " + std::to_string(i));
            }
        }
        pthread_mutexattr_destroy(&attr);
    }

    SHMQueue* shm_queue_;
    std::unique_ptr<shm::posix::POSIXSharedMemory<SHMQueue>> shm_;
    std::string name_;
};

} // namespace posix
} // namespace shm

#endif
