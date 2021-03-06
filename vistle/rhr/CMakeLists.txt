vistle_find_package(OpenGL)
vistle_find_package(LibVncServer)
vistle_find_package(TBB)
vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)
vistle_find_package(SNAPPY)
vistle_find_package(ZFP)


#use_openmp()

if(JPEGTURBO_FOUND)
   if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
      message("vistle_rhr: turbojpeg w/o tjMcuWidth")
   else()
      add_definitions(-DHAVE_TURBOJPEG)
      include_directories(${JPEGTURBO_INCLUDE_DIR})
   endif()
endif()

if(SNAPPY_FOUND)
   add_definitions(-DHAVE_SNAPPY)
   include_directories(${SNAPPY_INCLUDE_DIR})
endif()

if(ZFP_FOUND)
   add_definitions(-DHAVE_ZFP)
   include_directories(${ZFP_INCLUDE_DIRS})
endif()

set(RHR_SOURCES
   depthquant.cpp
   rfbext.cpp
   )

set(RHR_HEADERS
   depthquant.h
   rfbext.h
   ReadBackCuda.h
   )

if(TBB_FOUND)
   set(RHR_SOURCES ${RHR_SOURCES} rhrserver.cpp)
   set(RHR_HEADERS ${RHR_HEADERS} rhrserver.h)
   include_directories(
        ${TBB_INCLUDE_DIRS}
   )
endif()

if(LIBVNCSERVER_FOUND AND TBB_FOUND AND FALSE)
   set(RHR_SOURCES ${RHR_SOURCES} vncserver.cpp)
   set(RHR_HEADERS ${RHR_HEADERS} vncserver.h)
   include_directories(
        ${LIBVNCSERVER_INCLUDE_DIR}
   )
endif()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)

set(RHR_CUDA_SOURCES
    ReadBackCuda.cu
)

if(CUDA_FOUND AND GLEW_FOUND AND OPENGL_FOUND AND VISTLE_USE_CUDA)
  set(USE_CUDA TRUE)

  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS} ${GLEW_INCLUDES} ${OPENGL_INCLUDES})
  cuda_compile(RHR_CUDA_OBJ ${RHR_CUDA_SOURCES} ${VISTLE_LIB_TYPE})
else()
  add_custom_target(RHR_CUDA SOURCES ${RHR_CUDA_SOURCES})
endif()

if (NOT "${TURBOJPEG_LIBRARY_DYNAMIC}" STREQUAL "")
  vistle_add_library(vistle_rhr ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})
else()
  vistle_add_library(vistle_rhr STATIC ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})
endif()

if(USE_CUDA)
   target_link_libraries(vistle_rhr ${CUDA_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

target_link_libraries(vistle_rhr
        vistle_util
        vistle_core
        ${Boost_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
)

if (JPEGTURBO_FOUND AND TURBOJPEG_LIBRARY_DYNAMIC)
   target_link_libraries(vistle_rhr ${TURBOJPEG_LIBRARIES})
endif()

if(TBB_FOUND)
   target_link_libraries(vistle_rhr ${TBB_LIBRARIES})
endif()


if(SNAPPY_FOUND)
   target_link_libraries(vistle_rhr ${SNAPPY_LIBRARIES})
endif()

if (ZFP_FOUND)
   target_link_libraries(vistle_rhr ${ZFP_LIBRARIES})
endif()

if(LIBVNCSERVER_FOUND)
   target_link_libraries(vistle_rhr ${LIBVNCSERVER_SERVER_LIBRARY})
endif()

add_executable(depthtest depthtest.cpp)
target_link_libraries(depthtest vistle_rhr)
