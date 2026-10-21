add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL Export.cpp Export.Discovery.cpp ThreadQueries.cpp Pthread.cpp Aio.cpp Semaphore.cpp UserContext.cpp Signals.cpp Timers.cpp KernelSemaphores.cpp)
target_include_directories(${LIB_NAME} PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(${LIB_NAME})

target_sources(${LIB_NAME} PRIVATE Wait.cpp Uuid.cpp)
