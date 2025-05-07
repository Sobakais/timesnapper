cmake_minimum_required(VERSION 3.10)
project(timesnap VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# add_executable(${PROJECT_NAME}
#     src/main.cpp
# )

add_executable(test
  src/test.cpp
)

target_link_libraries(test PRIVATE
  mpg123
  syn123
  out123
)
