set(SUBSYS_NAME gpu_gmr)
set(SUBSYS_PATH gpu/gmr)
set(SUBSYS_DESC "Point cloud GPU gmr library")
set(SUBSYS_DEPS common gpu_containers gpu_utils gpu_octree)

set(build TRUE)
PCL_SUBSYS_OPTION(build "${SUBSYS_NAME}" "${SUBSYS_DESC}" ON)
PCL_SUBSYS_DEPEND(build NAME ${SUBSYS_NAME} DEPS ${SUBSYS_DEPS})
PCL_SET_SUBSYS_INCLUDE_DIR("${SUBSYS_NAME}" "${SUBSYS_PATH}")
mark_as_advanced("BUILD_${SUBSYS_NAME}")

PCL_ADD_DOC("${SUBSYS_NAME}")

if(NOT build)
  return()
endif()

set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --diag-suppress=20012")


set(srcs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gpu/gmr/src/segmentation/gpu_extract_clusters.cpp"
)

set(incs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gpu/gmr/include/pcl/gpu/gmr/segmentation/gpu_extract_clusters.h"
)

set(impl_incs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gpu/gmr/include/pcl/gpu/gmr/segmentation/impl/gpu_extract_clusters.hpp"
)

set(LIB_NAME "pcl_${SUBSYS_NAME}")
include_directories("${CMAKE_SOURCE_DIR}/../gmrpcl/gpu/gmr/include"${CUDA_INCLUDE_DIRS})

PCL_ADD_LIBRARY(${LIB_NAME} COMPONENT ${SUBSYS_NAME} SOURCES ${srcs} ${incs} ${impl_incs})
target_link_libraries("${LIB_NAME}" pcl_common pcl_gpu_octree pcl_gpu_utils pcl_gpu_containers ${CUDA_LIBRARIES})
PCL_MAKE_PKGCONFIG(${LIB_NAME} COMPONENT ${SUBSYS_NAME} DESC ${SUBSYS_DESC} PCL_DEPS ${SUBSYS_DEPS})

# Install include files
PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_PATH}" ${incs})
PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_PATH}/impl" ${impl_incs})
