#ifndef XSI_SHM_AREA_H
#define XSI_SHM_AREA_H

#include "shm/base_shm_area.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

namespace shm {
namespace xsi {

const std::string mutex_prefix = "_mutex";

union sem_status {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};

class XSIConditionVariable final : BaseSHMConditionVariable {
public:
    XSIConditionVariable() {
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

    ~XSIConditionVariable() {
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

class XSIMutex final : public shm::BaseSHMMutex {
public:
    explicit XSIMutex(std::string name) {
        key_t key = ftok(name.c_str(), 'M');
        if(key == -1) {
            throw std::runtime_error("[XSIMutex] ftok failed: " + std::string(strerror(errno)));
        }
        semid_ = semget(key, 1, IPC_CREAT | 0666);
        if(semid_ == -1) {
            throw std::runtime_error("[XSIMutex] semget failed: " + std::string(strerror(errno)));
        }
        sem_status arg;
        arg.val = 1;
        if(semctl(semid_, 0, SETVAL, arg) == -1) {
            throw std::runtime_error("[XSIMutex] semctl SETVAL failed: " + std::string(strerror(errno)));
        }
    }

    ~XSIMutex() {
        try {
            Unlock();
        } catch(...) {
        }
    }

    void Lock() override {
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = SEM_UNDO;
        if(semop(semid_, &sb, 1) == -1) {
            throw std::runtime_error("[XSIMutex] semop Lock failed: " + std::string(strerror(errno)));
        }
    }

    void Unlock() override {
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = SEM_UNDO;
        if(semop(semid_, &sb, 1) == -1) {
            throw std::runtime_error("[XSIMutex] semop Unlock failed: " + std::string(strerror(errno)));
        }
    }

private:
    int semid_;
};

template<typename T>
class XSISharedMemory final : public shm::BaseSHMArea<T> {
public:
    XSISharedMemory(std::string name, size_t size_of_area)
        : shm::BaseSHMArea<T>(name, size_of_area)
        , shmid_(-1)
        , shm_addr_(nullptr) {
        mu_ = std::make_shared<XSIMutex>(this->name_ + mutex_prefix);
    }

    std::remove_pointer_t<T>* AttachSHM() override {
        key_t key = ftok(this->name_.c_str(), 'S');
        if(key == -1) {
            throw std::runtime_error("[AttachSHM] ftok failed: " + std::string(strerror(errno)));
        }
        shmid_ = shmget(key, this->size_of_shm_area_, IPC_CREAT | 0666);
        if(shmid_ == -1) {
            throw std::runtime_error("[AttachSHM] shmget failed: " + std::string(strerror(errno)));
        }
        shm_addr_ = shmat(shmid_, nullptr, 0);
        if(shm_addr_ == reinterpret_cast<void*>(-1)) {
            throw std::runtime_error("[AttachSHM] shmat failed: " + std::string(strerror(errno)));
        }
        return static_cast<std::remove_pointer_t<T>*>(shm_addr_);
    }

    std::remove_pointer_t<T>* GetSHMAddr() override {
        return static_cast<std::remove_pointer_t<T>*>(shm_addr_);
    }

    void DeattachSHM() override {
        if(shm_addr_ != nullptr) {
            if(shmdt(shm_addr_) == -1) {
                throw std::runtime_error("[DeattachSHM] shmdt failed: " + std::string(strerror(errno)));
            }
            shm_addr_ = nullptr;
        }
    }

    std::shared_ptr<BaseSHMMutex> GetLock() override {
        return mu_;
    }

private:
    int shmid_;
    void* shm_addr_;
    std::shared_ptr<XSIMutex> mu_;
};

} // namespace xsi
} // namespace shm

#endif
