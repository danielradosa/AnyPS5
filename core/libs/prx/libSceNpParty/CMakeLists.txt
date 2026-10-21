add_library(libSceNpParty SHARED EXCLUDE_FROM_ALL Export.cpp)
target_include_directories(libSceNpParty PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceNpParty)
