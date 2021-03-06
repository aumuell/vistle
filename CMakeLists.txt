cmake_minimum_required(VERSION 2.8)
project(vistle_all)

if(POLICY CMP0042)
   # default to finding shlibs relative to @rpath on MacOS
   cmake_policy(SET CMP0042 NEW)
endif()

# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

# vim's YouCompleteMe can make use of this
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(VISTLE_BUILD_SHARED "Build all libraries as shared libraries instead of static" ON)
set(VISTLE_CPU_ARCH "corei7-avx" CACHE STRING "CPU architecture to optimize for (native, core-avx2, atom, ...)")

option(VISTLE_SANITIZE_UNDEFINED "Build with -fsanitize=undefined" OFF)
option(VISTLE_SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(VISTLE_SANITIZE_THREAD "Build with -fsanitize=thread" OFF)

option(VISTLE_USE_CUDA "Build with CUDA support" ON)

if (NOT "$ENV{VBUILD}" STREQUAL "")
   include("${PROJECT_SOURCE_DIR}/build/$ENV{VBUILD}.cmake")
endif()

if (VISTLE_BUILD_SHARED)
   set(VISTLE_LIB_TYPE SHARED)
else()
   set(VISTLE_LIB_TYPE STATIC)
   add_definitions(-DVISTLE_STATIC)
endif()
set(BUILD_SHARED_LIBS ${VISTLE_BUILD_SHARED})
if (NOT BUILD_SHARED_LIBS)
   set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

# allow for optimizations in Boost.MPI
# do not use for now: causes crash in broadcast on OS X
#add_definitions(-DBOOST_MPI_HOMOGENEOUS)

enable_testing()

if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
      FORCE
   )
   message("Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
   # disable rpath - only rely on LD_LIBRARY_PATH and the likes
   set(CMAKE_SKIP_RPATH TRUE)
else()
   add_definitions(-DEIGEN_INITIALIZE_MATRICES_BY_NAN)
   set(CMAKE_SKIP_RPATH FALSE)
endif()

#add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=short)
add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=int)

# use this instead of FIND_PACKAGE to prefer Package in $PACKAGE_HOME and $EXTERNLIBS/package
macro(vistle_find_package package)
   set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})

   set(pack "${package}")
   if (pack STREQUAL "PythonLibs")
      set(pack "Python")
   endif()

   string(TOUPPER ${pack} UPPER)
   string(TOLOWER ${pack} LOWER)
   if(MINGW)
      set(CMAKE_PREFIX_PATH ${MINGW_SYSROOT} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{EXTERNLIBS}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER}/bin ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS} ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{${UPPER}_HOME}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{${UPPER}_HOME} ${CMAKE_PREFIX_PATH})
   endif()
   #message("looking for package ${ARGV}")
   #message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
   find_package(${ARGV})

   set(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
endmacro(vistle_find_package PACKAGE)

if(WIN32)
   #set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
   #STRING(REGEX REPLACE "\\\\" "/" BOOST_ROOT ${BOOST_ROOT}) 
    #set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/vistle/cmake/Modules/windows;${CMAKE_MODULE_PATH}")
    #add_definitions(-DOMPI_IMPORTS)

    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)

    add_definitions(-DBOOST_ALL_NO_LIB)
    #add_definitions(-DBOOST_ALL_DYN_LINK)
    add_definitions(-DBOOST_MPI_DYN_LINK)
    add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
    add_definitions(-DBOOST_ZLIB_DYN_LINK)
    add_definitions(-DBOOST_IOSTREAMS_DYN_LINK)

    add_definitions(-D_WIN32_WINNT=0x0601) # windows 7
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4275 /wd4910 /EHsc")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4251 /wd4275 /wd4910 /EHsc")
endif(WIN32)


#vistle_find_package(IceT REQUIRED)
set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/3rdparty/IceT/src/include)
set(ICET_CORE_LIBS "IceTCore")
set(ICET_MPI_LIBS "IceTMPI")
set(ICET_FOUND TRUE)

if (VISTLE_USE_CUDA)
   vistle_find_package(CUDA)
   if ("${CUDA_VERSION}" VERSION_LESS 7.0)
      set (CUDA_HOST_COMPILER ${PROJECT_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)
   else()
      set(CUDA_PROPAGATE_HOST_FLAGS OFF)
      set(CUDA_NVCC_FLAGS "--std=c++11 ${CUDA_NVCC_FLAGS}")
   endif()
   if (APPLE)
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} \"-DBOOST_NOINLINE=__attribute__ ((noinline))\"")
   endif (APPLE)
endif()

set(ASAN_COMPILE_FLAGS "")
set(ASAN_LINK_FLAGS "")
if (VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=undefined")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=undefined")
endif()
if (VISTLE_SANITIZE_ADDRESS)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=address")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=address")
endif()
if (VISTLE_SANITIZE_THREAD)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=thread")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=thread")
endif()
if (VISTLE_SANITIZE_THREAD OR VISTLE_SANITIZE_ADDRESS OR VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fno-omit-frame-pointer")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -g3")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fPIC")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${ASAN_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${ASAN_LINK_FLAGS}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
   if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
   endif()
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

   if (DEFINED VISTLE_CPU_ARCH)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
   endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
   if (NOT APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
   endif()

   if (NOT ${BUILD_SHARED_LIBS})
      # for linking the libraries into plugins
      #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
      set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
   endif()

   if (NOT APPLE)
      if (DEFINED VISTLE_CPU_ARCH)
         set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
         set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
      endif()
   endif()

   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
      set(CUDA_HOST_COMPILER ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)

      if (NOT APPLE)
         set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfxsr")
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfxsr")
      endif()
   endif()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

vistle_find_package(MPI REQUIRED)

include_directories(${MPI_C_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
add_definitions(-DOMPI_SKIP_MPICXX)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

vistle_find_package(Boost 1.40 REQUIRED system)
add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)
if (Boost_VERSION VERSION_GREATER 106099)
   set(BOOST_MPI_DIR "boost-mpi")
elseif (Boost_VERSION VERSION_GREATER 105899)
   set(BOOST_MPI_DIR "boost-mpi-1.60")
elseif (Boost_VERSION VERSION_GREATER 105599)
   set(BOOST_MPI_DIR "boost-mpi-1.58")
else()
   set(BOOST_MPI_DIR "boost-mpi-1.55")
endif()
#message("Using Boost.MPI from ${BOOST_MPI_DIR} for Boost ${Boost_VERSION}")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if (LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()
set(INSTALL_LIB_DIR     lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

macro(vistle_add_library targetname)
    add_library(${targetname} ${ARGN})
    set_target_properties(${targetname} PROPERTIES FOLDER "Libraries")
    install(TARGETS ${targetname}
       LIBRARY DESTINATION ${INSTALL_LIB_DIR}
       ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
endmacro(vistle_add_library)


add_subdirectory(3rdparty)
add_subdirectory(vistle)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.2")
   include(FeatureSummary)
   feature_summary(WHAT ALL)
endif()                                                                                                                                                            
