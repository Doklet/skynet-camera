include(${CMAKE_CURRENT_SOURCE_DIR}/FindErlang.cmake)

include_directories(
  SYSTEM
  ${Erlang_EI_INCLUDE_DIR}
  ${Erlang_EI_DIR}/src
)

link_directories(
  ${Boost_LIBRARY_DIRS}
  ${Erlang_EI_LIBRARY_DIR}
)

set(EIXX_LIBS
  ${Erlang_EI_LIBRARIES}
  ${OPENSSL_LIBRARIES}  # For MD5 support
  pthread
)

file(GLOB skynetCameraSources *.cpp)
file(GLOB skynetCameraIncludes *.h )

cuda_add_executable(skynet-camera ${skynetCameraSources})
target_link_libraries(skynet-camera nvcaffe_parser nvinfer jetson-inference pthread erl_interface ei)
