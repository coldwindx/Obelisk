PROJECT(OBELISK)

INCLUDE_DIRECTORIES(/usr/include)
INCLUDE_DIRECTORIES(./)

SET(SRC_LIST    
    main.cpp 
    log/log_formatter.cpp 
    log/logger.cpp 
    log/log_initer.cpp
    config/config.cpp
    thread/thread.cpp
    coroutine/coroutine.cpp)
SET(LIBS pthread yaml-cpp)

LINK_DIRECTORIES(/usr/local/lib64)

ADD_EXECUTABLE(main.out ${SRC_LIST})
TARGET_LINK_LIBRARIES(main.out ${LIBS})