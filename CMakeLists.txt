PROJECT(OBELISK)

INCLUDE(cmake/utils.cmake)

INCLUDE_DIRECTORIES(/usr/include)
INCLUDE_DIRECTORIES(./include)

SET(SRC_LIST    
    src/log/log_formatter.cpp 
    src/log/logger.cpp 
    src/log/log_initer.cpp
    src/config.cpp
    src/thread.cpp
    src/coroutine.cpp
    src/scheduler.cpp
    src/iomanager.cpp
    src/timer.cpp
    src/hook.cpp
    src/fd_manager.cpp
    src/address.cpp
    src/socket.cpp
    src/bytearray.cpp
    src/daemon.cpp
    src/env.cpp
    src/utils.cpp
    src/application.cpp
    src/module.cpp
    src/library.cpp
    src/http/http.cpp
    src/http/http_parser.cpp
    src/http/tcp_server.cpp
    src/http/http_session.cpp
    src/http/http_connection.cpp
    src/http/http_server.cpp
    src/http/servlet.cpp
    src/stream.cpp
    src/streams/socket_stream.cpp)

ragelmaker(src/http/http11_parser.rl SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/httpclient_parser.rl SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/uri.rl SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/http)

LINK_DIRECTORIES(/usr/local/lib64)
SET(LIBS pthread yaml-cpp dl)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
file(COPY ${PROJECT_SOURCE_DIR}/bin/conf DESTINATION ${PROJECT_SOURCE_DIR}/build)

ADD_LIBRARY(obelisk SHARED ${SRC_LIST})

ADD_EXECUTABLE(main.out "sample/main.cpp")
TARGET_LINK_LIBRARIES(main.out obelisk ${LIBS})
force_redefine_file_macro_for_sources(project_name)