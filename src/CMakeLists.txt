CMAKE_MINIMUM_REQUIRED( VERSION 3.25 )

if ( ATOMIC_QUEUE_BUILD_EXAMPLES )
    add_executable(
        atomic_queue_example
        ${CMAKE_CURRENT_SOURCE_DIR}/example.cc
    )

    target_link_libraries(
        atomic_queue_example
        atomic_queue
    )
endif()

    add_executable(
        atomic_queue_example
        ${CMAKE_CURRENT_SOURCE_DIR}/example.cc
    )

    add_executable(
        atomic_queue_pro
        ${CMAKE_CURRENT_SOURCE_DIR}/pro_example.cc
    )

    add_executable(
        atomic_queue_cons
        ${CMAKE_CURRENT_SOURCE_DIR}/cons_example.cc
    )

    target_link_libraries(
        atomic_queue_example
        atomic_queue
    )

    target_link_libraries(
        atomic_queue_pro
        atomic_queue
        shm
    )

    target_link_libraries(
        atomic_queue_cons
        atomic_queue
        shm
    )

if ( ATOMIC_QUEUE_BUILD_TESTS )
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    add_executable(
        atomic_queue_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests.cc
    )

    target_link_libraries(
        atomic_queue_tests
        atomic_queue
        Boost::unit_test_framework
    )

    add_test(
        NAME atomic_queue_tests
        COMMAND atomic_queue_tests
    )
endif()
