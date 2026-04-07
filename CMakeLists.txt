cmake_minimum_required(VERSION 3.16)

project(NetWatch VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(VCPKG_APPLOCAL_DEPS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Boost REQUIRED COMPONENTS system)

# Networking and Common Library
add_library(NetWatch_Networking STATIC
    networking/src/connection.cpp
    networking/src/client.cpp
    networking/src/server.cpp
    networking/src/protocol.cpp
)
target_include_directories(NetWatch_Networking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/networking/include
)
target_link_libraries(NetWatch_Networking PUBLIC Boost::system Threads::Threads)
if(WIN32)
    target_link_libraries(NetWatch_Networking PUBLIC ws2_32 mswsock)
endif()

# Agent Executable
add_executable(NetWatchAgent
    agent/main.cpp
    agent/src/system_monitor.cpp
    agent/src/process_monitor.cpp
)
target_include_directories(NetWatchAgent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/agent/include)
target_link_libraries(NetWatchAgent PRIVATE NetWatch_Networking)

# Dashboard Target (Main Application)
set(PROJECT_SOURCES
    dashboard/main.cpp
    dashboard/mainwindow.cpp
    dashboard/mainwindow.hpp
    dashboard/mainwindow.ui
    dashboard/widgets/process_table.cpp
    dashboard/widgets/process_table.hpp
    dashboard/widgets/device_card.hpp
    dashboard/widgets/device_card.cpp
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(NetWatch
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(NetWatch SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(NetWatch
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(NetWatch PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/dashboard 
    ${CMAKE_CURRENT_SOURCE_DIR}/dashboard/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_link_libraries(NetWatch PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Boost::system
    NetWatch_Networking
)
if(WIN32)
    target_link_libraries(NetWatch PRIVATE ws2_32)
endif()

if(QT_VERSION VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.NetWatch)
endif()
set_target_properties(NetWatch PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Testing
enable_testing()
add_executable(test_protocol tests/test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE NetWatch_Networking)
add_test(NAME ProtocolTest COMMAND test_protocol)

add_executable(test_connection tests/test_connection.cpp)
target_link_libraries(test_connection PRIVATE NetWatch_Networking)
if(WIN32)
    target_link_libraries(test_connection PRIVATE ws2_32 mswsock)
endif()
add_test(NAME ConnectionTest COMMAND test_connection)

include(GNUInstallDirs)
install(TARGETS NetWatch NetWatchAgent NetWatch_Networking
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NetWatch)
endif()
