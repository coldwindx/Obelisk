PROJECT(OBELISK)

INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./log)
INCLUDE_DIRECTORIES(./config)
INCLUDE_DIRECTORIES(./coroutine)
INCLUDE_DIRECTORIES(/usr/include)

SET(SRC_LIST    
    main.cpp 
    log/log_formatter.cpp 
    log/logger.cpp 
    log/log_initer.cpp
    config/config.cpp
    src/thread.cpp
    coroutine/coroutine.cpp
    src/scheduler.cpp
    coroutine/iomanager.cpp
    src/timer.cpp
    src/hook.cpp
    src/fd_manager.cpp
    src/address.cpp
    src/socket.cpp)
SET(LIBS pthread yaml-cpp dl)

LINK_DIRECTORIES(/usr/local/lib64)

ADD_EXECUTABLE(main.out ${SRC_LIST})
TARGET_LINK_LIBRARIES(main.out ${LIBS})