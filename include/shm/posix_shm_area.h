#ifndef POSIX_SHM_AREA_H
#define POSIX_SHM_AREA_H

#include "shm/base_shm_area.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <memory>
#include <pthread.h>

namespace shm {
namespace posix {

class POSIXConditionVariable final : public BaseSHMConditionVariable {
public:
    POSIXConditionVariable() {
        pthread_condattr_t attr;
        int ret = pthread_condattr_init(&attr);
        if(ret != 0) {
            throw std::runtime_error("pthread_condattr_init failed: " + std::string(strerror(ret)));
        }
        ret = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if(ret != 0) {
            pthread_condattr_destroy(&attr);
            throw std::runtime_error("pthread_condattr_setpshared failed: " + std::string(strerror(ret)));
        }
        ret = pthread_cond_init(&cond_, &attr);
        pthread_condattr_destroy(&attr);
        if(ret != 0) {
            throw std::runtime_error("pthread_cond_init failed: " + std::string(strerror(ret)));
        }
    }

    ~POSIXConditionVariable() {
        int ret = pthread_cond_destroy(&cond_);
        if(ret != 0) {
        }
    }

    void wait(pthread_mutex_t* mutex) override {
        int ret = pthread_cond_wait(&cond_, mutex);
        if(ret != 0) {
            throw std::runtime_error("pthread_cond_wait failed: " + std::string(strerror(ret)));
        }
    }

    void notify_one() override {
        int ret = pthread_cond_signal(&cond_);
        if(ret != 0) {
            throw std::runtime_error("pthread_cond_signal failed: " + std::string(strerror(ret)));
        }
    }

    void notify_all() override {
        int ret = pthread_cond_broadcast(&cond_);
        if(ret != 0) {
            throw std::runtime_error("pthread_cond_broadcast failed: " + std::string(strerror(ret)));
        }
    }

private:
    pthread_cond_t cond_;
};

class POSIXMutex final : public BaseSHMMutex {
public:
    POSIXMutex() {
        pthread_mutexattr_t attr;
        if(pthread_mutexattr_init(&attr) != 0) {
            throw std::runtime_error("pthread_mutexattr_init failed");
        }
        if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
            pthread_mutexattr_destroy(&attr);
            throw std::runtime_error("pthread_mutexattr_setpshared failed");
        }
        if(pthread_mutex_init(&mutex_, &attr) != 0) {
            pthread_mutexattr_destroy(&attr);
            throw std::runtime_error("pthread_mutex_init failed");
        }
        pthread_mutexattr_destroy(&attr);
    }

    ~POSIXMutex() {
        pthread_mutex_destroy(&mutex_);
    }

    void Lock() override {
        if(pthread_mutex_lock(&mutex_) != 0) {
            throw std::runtime_error("pthread_mutex_lock failed");
        }
    }

    void Unlock() override {
        if(pthread_mutex_unlock(&mutex_) != 0) {
            throw std::runtime_error("pthread_mutex_unlock failed");
        }
    }

    pthread_mutex_t* get() { return &mutex_; }

private:
    pthread_mutex_t mutex_;
};

// POSIX shared memory area implementation.
template<typename T>
class POSIXSharedMemory final : public BaseSHMArea<T> {
public:
    POSIXSharedMemory(std::string name, size_t size_of_area)
        : BaseSHMArea<T>(name, size_of_area), shm_fd_(-1), shm_addr_(nullptr) {
        if(this->name_.front() != '/') {
            this->name_ = "/" + this->name_;
        }
    }

    std::remove_pointer_t<T>* AttachSHM() override {
        shm_fd_ = shm_open(this->name_.c_str(), O_CREAT | O_RDWR, 0666);
        if(shm_fd_ == -1) {
            throw std::runtime_error("shm_open failed: " + std::string(strerror(errno)));
        }
        if(ftruncate(shm_fd_, this->size_of_shm_area_) == -1) {
            throw std::runtime_error("ftruncate failed: " + std::string(strerror(errno)));
        }
        shm_addr_ = mmap(nullptr, this->size_of_shm_area_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
        if(shm_addr_ == MAP_FAILED) {
            throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
        }
        return static_cast<std::remove_pointer_t<T>*>(shm_addr_);
    }

    // TODO: we cannot call this multiple times
    std::remove_pointer_t<T>* AttachSHMRange(off_t offset, size_t length) {
        if(shm_fd_ == -1) {
            shm_fd_ = shm_open(this->name_.c_str(), O_CREAT | O_RDWR, 0666);
            if(shm_fd_ == -1) {
                throw std::runtime_error("shm_open failed: " + std::string(strerror(errno)));
            }
            if(ftruncate(shm_fd_, this->size_of_shm_area_) == -1) {
                throw std::runtime_error("ftruncate failed: " + std::string(strerror(errno)));
            }
        }
        void* addr = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, offset);
        if(addr == MAP_FAILED) {
            throw std::runtime_error("mmap range failed: " + std::string(strerror(errno)));
        }
        return static_cast<std::remove_pointer_t<T>*>(addr);
    }

    std::remove_pointer_t<T>* GetSHMAddr() override {
        return static_cast<std::remove_pointer_t<T>*>(shm_addr_);
    }

    void DeattachSHM() override {
        if(shm_addr_ != nullptr) {
            munmap(shm_addr_, this->size_of_shm_area_);
            shm_addr_ = nullptr;
        }
        if(shm_fd_ != -1) {
            close(shm_fd_);
            shm_fd_ = -1;
        }
    }

    std::shared_ptr<BaseSHMMutex> GetLock() override {
        return std::make_shared<POSIXMutex>();
    }

    void RemoveSHM() {
        if(shm_unlink(this->name_.c_str()) == -1) {
            throw std::runtime_error("shm_unlink failed: " + std::string(strerror(errno)));
        }
    }

private:
    int shm_fd_;
    void* shm_addr_;
};

} // namespace posix
} // namespace shm

#endif
