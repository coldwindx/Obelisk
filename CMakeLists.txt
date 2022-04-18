PROJECT(OBELISK)

SET(SRC_LIST    
    main.cpp log/log_formatter.cpp 
    log/logger.cpp config/config.cpp
    thread/thread.cpp)
SET(LIBS pthread yaml-cpp)

LINK_DIRECTORIES(/usr/local/lib64)

ADD_EXECUTABLE(main.out ${SRC_LIST})
TARGET_LINK_LIBRARIES(main.out ${LIBS})