add_library(libSceMove SHARED EXCLUDE_FROM_ALL Export.cpp)
target_include_directories(libSceMove PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceMove)
