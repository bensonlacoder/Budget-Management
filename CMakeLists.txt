cmake_minimum_required(VERSION 3.16)

project(DiningPointsBudget)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Retain SQL support from the course template for future persistence.
find_package(Qt6 COMPONENTS Widgets Sql Test REQUIRED)

# New model and test files are discovered automatically. UI files stay separate.
file(GLOB APP_SOURCES  CONFIGURE_DEPENDS src/*.cpp)
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

add_executable(app ${APP_SOURCES} UI/main.cpp UI/mainui.cpp UI/mainui.h UI/mainui.ui)
target_link_libraries(app PRIVATE Qt6::Widgets Qt6::Sql)
target_include_directories(app PRIVATE src UI)

# Course tests cover models; keep view and controller sources out of this target.
# UI/main.cpp belongs only to app because doctest supplies its own main().
add_executable(test_build ${TEST_SOURCES} ${APP_SOURCES})
target_link_libraries(test_build PRIVATE Qt6::Widgets Qt6::Sql)
target_include_directories(test_build PRIVATE src tests)

enable_testing()
add_test(NAME test_build COMMAND test_build)

# Exercise day-entry validation through the real Qt widgets.
add_executable(ui_tests tests/ui/remaining_days_test.cpp ${APP_SOURCES}
    UI/mainui.cpp UI/mainui.h UI/mainui.ui)
target_link_libraries(ui_tests PRIVATE Qt6::Widgets Qt6::Test)
target_include_directories(ui_tests PRIVATE src UI tests)
add_test(NAME ui_tests COMMAND ui_tests)
set_tests_properties(ui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
