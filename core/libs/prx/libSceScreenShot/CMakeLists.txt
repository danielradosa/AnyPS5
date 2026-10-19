add_library(libSceScreenShot SHARED EXCLUDE_FROM_ALL Export.cpp)
target_include_directories(libSceScreenShot PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceScreenShot)

if(BUILD_TESTING)
    add_test_executable(guest_screen_shot_tests ${LIBS_INCLUDE_DIR}/tests/GuestScreenShot.cpp)
    target_include_directories(guest_screen_shot_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_screen_shot_tests PRIVATE libSceScreenShot)
    add_test(NAME guest_screen_shot COMMAND guest_screen_shot_tests)
    if(WIN32)
        set_tests_properties(guest_screen_shot PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceScreenShot>")
    endif()
endif()
