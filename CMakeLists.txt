project(custom_array)

set(CMAKE_CXX_STANDARD 11)

set(
    ALL_SRCS
    main.cpp
    main_catch.cpp
    array.h
    catch.h
)

add_executable(
    custom_array
    ${ALL_SRCS}
)
