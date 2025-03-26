cmake_minimum_required(VERSION 3.16)
project(jstest-qt VERSION 0.1.1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${X11_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/main.h
    src/joystick.cpp
    src/joystick.h
    src/joystick_description.h
    src/widgets/axis_widget.cpp
    src/widgets/axis_widget.h
    src/widgets/button_widget.cpp
    src/widgets/button_widget.h
    src/widgets/rudder_widget.cpp
    src/widgets/rudder_widget.h
    src/widgets/throttle_widget.cpp
    src/widgets/throttle_widget.h
    src/dialogs/joystick_list_dialog.cpp
    src/dialogs/joystick_list_dialog.h
    src/dialogs/joystick_test_dialog.cpp
    src/dialogs/joystick_test_dialog.h
    src/dialogs/joystick_map_dialog.cpp
    src/dialogs/joystick_map_dialog.h
    src/dialogs/joystick_calibration_dialog.cpp
    src/dialogs/joystick_calibration_dialog.h
    src/dialogs/calibrate_maximum_dialog.cpp
    src/dialogs/calibrate_maximum_dialog.h
    src/utils/evdev_helper.cpp
    src/utils/evdev_helper.h
    resources.qrc
)

# Create the executable
add_executable(jstest-qt ${SOURCES})

# Link libraries
target_link_libraries(jstest-qt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${CMAKE_THREAD_LIBS_INIT}
    ${X11_LIBRARIES}
)

# Install rules
install(TARGETS jstest-qt
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

install(FILES
    ${CMAKE_BINARY_DIR}/jstest-qt
    RENAME jstest-qt
    PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY resources/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
    PATTERN "*~" EXCLUDE)

install(FILES
    jstest-qt.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES
    resources/generic.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME jstest-qt.png)
