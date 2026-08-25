cmake_minimum_required(VERSION 3.25)
project(sister_urt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SISTER_URT_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

function(sister_urt_strict target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  if(SISTER_URT_WARNINGS_AS_ERRORS)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

add_library(sister_urt_core
  src/types.cpp
  src/domain.cpp
  src/governance.cpp
)
target_include_directories(sister_urt_core PUBLIC include)
sister_urt_strict(sister_urt_core)

add_library(sister_urt_repository
  src/repository.cpp
)
target_include_directories(sister_urt_repository PUBLIC include)
target_link_libraries(sister_urt_repository PUBLIC sister_urt_core)
sister_urt_strict(sister_urt_repository)

add_library(sister_urt_http
  src/http.cpp
)
target_include_directories(sister_urt_http PUBLIC include)
target_link_libraries(sister_urt_http PUBLIC sister_urt_repository)
sister_urt_strict(sister_urt_http)

add_executable(sister-urt-cli src/main_cli.cpp)
target_link_libraries(sister-urt-cli PRIVATE sister_urt_repository)
sister_urt_strict(sister-urt-cli)

add_executable(sister-urt-http src/main_http.cpp)
target_link_libraries(sister-urt-http PRIVATE sister_urt_http)
sister_urt_strict(sister-urt-http)

enable_testing()

add_executable(sister_urt_domain_test tests/domain_test.cpp)
target_link_libraries(sister_urt_domain_test PRIVATE sister_urt_core)
sister_urt_strict(sister_urt_domain_test)
add_test(NAME sister_urt_domain_test COMMAND sister_urt_domain_test)

add_executable(sister_urt_governance_test tests/governance_test.cpp)
target_link_libraries(sister_urt_governance_test PRIVATE sister_urt_core)
sister_urt_strict(sister_urt_governance_test)
add_test(NAME sister_urt_governance_test COMMAND sister_urt_governance_test)

add_executable(sister_urt_http_test tests/http_test.cpp)
target_link_libraries(sister_urt_http_test PRIVATE sister_urt_http)
sister_urt_strict(sister_urt_http_test)
add_test(NAME sister_urt_http_test COMMAND sister_urt_http_test)
