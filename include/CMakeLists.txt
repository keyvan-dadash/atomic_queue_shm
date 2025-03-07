CMAKE_MINIMUM_REQUIRED( VERSION 3.25 )

add_library(
    atomic_queue
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/atomic_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/atomic_queue_mutex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/barrier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/defs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/spinlock.h
)

add_library(
    shm
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/shm/base_shm_area.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shm/xsi_shm_area.h
)

target_include_directories(
    atomic_queue
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    shm
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
