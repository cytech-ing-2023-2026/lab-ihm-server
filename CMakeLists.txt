cmake_minimum_required(VERSION 3.14)

project(lab-ihm-server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

add_executable(lab-ihm-server
  model.cpp
  main.cpp

  model.hpp
)

target_include_directories(lab-ihm-server PRIVATE ${SODIUM_INCLUDE_DIRS})
target_link_libraries(lab-ihm-server Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql PkgConfig::SODIUM)

include(GNUInstallDirs)
install(TARGETS lab-ihm-server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
