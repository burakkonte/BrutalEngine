cmake_minimum_required(VERSION 3.20)
project(BrutalEngine LANGUAGES C CXX)

set(BRUTAL_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(BRUTAL_CONTENT_ROOT "${BRUTAL_SOURCE_ROOT}")

if(NOT EXISTS "${BRUTAL_CONTENT_ROOT}/third_party/CMakeLists.txt" AND
   EXISTS "${BRUTAL_SOURCE_ROOT}/brutal/third_party/CMakeLists.txt")
    set(BRUTAL_CONTENT_ROOT "${BRUTAL_SOURCE_ROOT}/brutal")
endif()

foreach(subdir third_party engine playground)
    if(NOT EXISTS "${BRUTAL_CONTENT_ROOT}/${subdir}/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing '${subdir}' in ${BRUTAL_CONTENT_ROOT}. "
            "Ensure the archive was fully extracted."
        )
    endif()
endforeach()

add_subdirectory("${BRUTAL_CONTENT_ROOT}/third_party" "${CMAKE_CURRENT_BINARY_DIR}/third_party")
add_subdirectory("${BRUTAL_CONTENT_ROOT}/engine" "${CMAKE_CURRENT_BINARY_DIR}/engine")
add_subdirectory("${BRUTAL_CONTENT_ROOT}/playground" "${CMAKE_CURRENT_BINARY_DIR}/playground")

if(MSVC)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT playground)
endif()