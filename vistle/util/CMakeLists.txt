vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system filesystem)

set(util_SOURCES
   coRestraint.cpp
   exception.cpp
   findself.cpp
   hostname.cpp
   stopwatch.cpp
   tools.cpp
   spawnprocess.cpp
   directory.cpp
   sleep.cpp
   sysdep.cpp
)

set(util_HEADERS
   export.h
   coRestraint.h
   enum.h
   exception.h
   findself.h
   hostname.h
   math.h
   sysdep.h
   tools.h
   triple.h
   stopwatch.h
   valgrind.h
   vecstreambuf.h
   spawnprocess.h
   directory.h
   sleep.h
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "")
   message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
   add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

vistle_add_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_link_libraries(vistle_util
        ${Boost_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)
