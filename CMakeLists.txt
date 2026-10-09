cmake_minimum_required(VERSION 3.16)

project(slowa VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(slowa STATIC
    solver.cpp
    solver.h
    trie.cpp
    trie.h
    utf8.h
    params.h
)

target_include_directories(slowa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(EMSCRIPTEN)
    # Build the WebAssembly module exposing the C API from wasm/bindings.cpp.
    add_executable(slowa_wasm wasm/bindings.cpp)
    target_link_libraries(slowa_wasm PRIVATE slowa)
    set_target_properties(slowa_wasm PROPERTIES OUTPUT_NAME slowa SUFFIX ".js")
    target_link_options(slowa_wasm PRIVATE
        -O3
        -sMODULARIZE=1
        -sEXPORT_NAME=createSlowa
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=web,worker,node
        -sEXPORTED_FUNCTIONS=_malloc,_free
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,HEAPU8
        -sSTACK_SIZE=1048576
    )
else()
    enable_testing()

    add_executable(slowa_tests tests.cpp)
    target_link_libraries(slowa_tests PRIVATE slowa)
    add_test(NAME slowa_tests COMMAND slowa_tests)

    add_executable(slowa_benchmark benchmark.cpp)
    target_link_libraries(slowa_benchmark PRIVATE slowa)
endif()
