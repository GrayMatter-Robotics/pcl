set(SUBSYS_NAME gmr)
set(SUBSYS_DESC "Point cloud GMR library")
set(SUBSYS_DEPS common geometry search sample_consensus kdtree octree features filters ml)

set(build TRUE)
PCL_SUBSYS_OPTION(build "${SUBSYS_NAME}" "${SUBSYS_DESC}" ON)
PCL_SUBSYS_DEPEND(build NAME ${SUBSYS_NAME} DEPS ${SUBSYS_DEPS} OPT_DEPS OpenMP)

PCL_ADD_DOC("${SUBSYS_NAME}")

if(NOT build)
  return()
endif()

set(srcs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gmr/src/extract_clusters_omp.cpp"
)

set(incs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gmr/include/pcl/gmr/extract_clusters_omp.h"
)

set(impl_incs
  "${CMAKE_SOURCE_DIR}/../gmrpcl/gmr/include/pcl/gmr/impl/extract_clusters_omp.hpp"
)

set(LIB_NAME "pcl_${SUBSYS_NAME}")
include_directories("${CMAKE_SOURCE_DIR}/../gmrpcl/gmr/include")

PCL_ADD_LIBRARY(${LIB_NAME} COMPONENT ${SUBSYS_NAME} SOURCES ${srcs} ${incs} ${impl_incs})
target_link_libraries("${LIB_NAME}" pcl_search pcl_sample_consensus pcl_filters pcl_ml pcl_features)
PCL_MAKE_PKGCONFIG(${LIB_NAME} COMPONENT ${SUBSYS_NAME} DESC ${SUBSYS_DESC} PCL_DEPS ${SUBSYS_DEPS})

# Install include files
PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_NAME}" ${incs})
PCL_ADD_INCLUDES("${SUBSYS_NAME}" "${SUBSYS_NAME}/impl" ${impl_incs})
