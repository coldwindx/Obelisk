PROJECT(OBELISK)

INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./log)
INCLUDE_DIRECTORIES(./config)
INCLUDE_DIRECTORIES(./coroutine)
INCLUDE_DIRECTORIES(/usr/include)

SET(SRC_LIST    
    sample/main.cpp 
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
    src/socket.cpp
    src/bytearray.cpp
    src/http/http.cpp
    src/http/http11_parser.cc
    src/http/httpclient_parser.cc
    src/http/http_parser.cpp
    src/http/tcp_server.cpp
    src/http/http_session.cpp
    src/stream.cpp
    src/streams/socket_stream.cpp)
SET(LIBS pthread yaml-cpp dl)

LINK_DIRECTORIES(/usr/local/lib64)

ADD_EXECUTABLE(main.out ${SRC_LIST})
TARGET_LINK_LIBRARIES(main.out ${LIBS})
