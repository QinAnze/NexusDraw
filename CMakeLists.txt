cmake_minimum_required(VERSION 3.16)
project(NexusDraw VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 (fall back to Qt5 if not found)
find_package(Qt6 COMPONENTS Widgets Network Core QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Network Core)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

set(SOURCES
    src/main.cpp
    # Core
    src/core/AppConfig.cpp
    src/core/AIManager.cpp
    src/core/DataManager.cpp
    src/core/CodeExecutor.cpp
    # UI
    src/ui/MainWindow.cpp
    src/ui/AIConfigDialog.cpp
    src/ui/ChatPanel.cpp
    src/ui/DataPreviewPanel.cpp
    src/ui/CodeViewPanel.cpp
    src/ui/PlotViewPanel.cpp
    src/ui/ExportDialog.cpp
    src/ui/TerminalPanel.cpp
    src/ui/FavoritesPanel.cpp
)

set(HEADERS
    src/core/AppConfig.h
    src/core/AIManager.h
    src/core/DataManager.h
    src/core/CodeExecutor.h
    src/ui/MainWindow.h
    src/ui/AIConfigDialog.h
    src/ui/ChatPanel.h
    src/ui/DataPreviewPanel.h
    src/ui/CodeViewPanel.h
    src/ui/PlotViewPanel.h
    src/ui/ExportDialog.h
    src/ui/TerminalPanel.h
    src/ui/FavoritesPanel.h
)

set(RESOURCES
    resources/resources.qrc
)

# Windows icon resource
if(WIN32)
    set(SOURCES ${SOURCES} resources/nexusdraw.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network Qt6::Core)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network Qt5::Core)
endif()

# Platform-specific settings
if(WIN32)
    # Hide console window (WIN32_EXECUTABLE causes MinGW linker error)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition -Wl,-subsystem,windows)
endif()

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/${PROJECT_NAME}/resources)
