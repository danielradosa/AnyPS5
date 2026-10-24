add_library(libScePigletv2VSH SHARED EXCLUDE_FROM_ALL
        Export.cpp
        src/Context.cpp
        src/Egl.cpp
        src/Program.cpp
        src/Vulkan.cpp
)
target_include_directories(libScePigletv2VSH PRIVATE ${LIBS_INCLUDE_DIR} ${ANYPS5_VULKAN_HEADERS_INCLUDE_DIR})
target_link_libraries(libScePigletv2VSH PRIVATE libSceVideoOut libSceAgcDriver libc)
if(MINGW)
    target_link_libraries(libScePigletv2VSH PRIVATE $<TARGET_LINKER_FILE:libc> glslang SPIRV glslang-default-resource-limits SDL2-static stdc++)
else()
    target_link_libraries(libScePigletv2VSH PRIVATE glslang SPIRV glslang-default-resource-limits SDL2-static)
endif()
if(MINGW AND NOT ANYPS5_USE_SYSTEM_DEPS)
    set(glslangPoolAllocatorSource "${ANYPS5_GLSLANG_SOURCE_DIR}/glslang/MachineIndependent/PoolAlloc.cpp")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${glslangPoolAllocatorSource}")
    file(READ "${glslangPoolAllocatorSource}" glslangPoolAllocator)
    string(REPLACE "\r\n" "\n" glslangPoolAllocator "${glslangPoolAllocator}")
    string(SHA256 glslangPoolAllocatorDigest "${glslangPoolAllocator}")
    if(NOT glslangPoolAllocatorDigest STREQUAL "9eba9c5fa4f46b875568a6b5e9805a4fca5b53ba74aceed1337d1db3739b3a4d")
        message(FATAL_ERROR "glslang's PoolAlloc.cpp has changed (SHA-256 ${glslangPoolAllocatorDigest}); src/GlslangPoolAllocator.cpp compiles all of it with DISABLE_THREAD_SUPPORT, which makes every THREAD_LOCAL in it process-wide, so review it against the new file and then update the digest")
    endif()
    target_sources(libScePigletv2VSH PRIVATE src/GlslangPoolAllocator.cpp)
    set_source_files_properties(src/GlslangPoolAllocator.cpp PROPERTIES COMPILE_OPTIONS "-Wno-return-local-addr;-Werror=unused-function")
endif()
if(MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(libScePigletv2VSH PRIVATE LINKER:--exclude-libs,ALL)
endif()
configure_windows_unwind(libScePigletv2VSH)

if(BUILD_TESTING)
    add_test_executable(guest_piglet_tests ${LIBS_INCLUDE_DIR}/tests/GuestPiglet.cpp)
    target_include_directories(guest_piglet_tests PRIVATE ${LIBS_INCLUDE_DIR} ${ANYPS5_VULKAN_HEADERS_INCLUDE_DIR})
    target_link_libraries(guest_piglet_tests PRIVATE libScePigletv2VSH libScePrecompiledShaders libc)
    configure_windows_unwind(guest_piglet_tests)
    add_test(NAME guest_piglet COMMAND guest_piglet_tests)
    set_tests_properties(guest_piglet PROPERTIES TIMEOUT 30 SKIP_RETURN_CODE 77)
    if(WIN32)
        set_tests_properties(guest_piglet PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libScePigletv2VSH>")
    endif()
endif()
