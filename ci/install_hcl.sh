#!/bin/bash

INSTALL_DIR=/root/install
SPACK_DIR=/root/spack

mkdir build
pushd build

CXXFLAGS="-I${INSTALL_DIR}/include -fsanitize=address -O1 -fno-omit-frame-pointer -g" \
LDFLAGS="-L${INSTALL_DIR}/lib"                                       \
CMAKE_C_COMPILER=${INSTALL_DIR}/bin/gcc                              \
CMAKE_CXX_COMPILER=${INSTALL_DIR}/bin/g++

${INSTALL_DIR}/bin/cmake                                                      \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}                        \
    -DCMAKE_BUILD_RPATH="${INSTALL_DIR}/lib"                     \
    -DCMAKE_INSTALL_RPATH="${INSTALL_DIR}/lib"                   \
    -DCMAKE_BUILD_TYPE=Release                             \
    -DCMAKE_CXX_COMPILER=${INSTALL_DIR}/bin/g++                    \
    -DCMAKE_C_COMPILER=${INSTALL_DIR}/bin/gcc                      \
    -DHCL_ENABLE_RPCLIB=${HCL_ENABLE_RPCLIB}               \
    -DHCL_ENABLE_THALLIUM_TCP=${HCL_ENABLE_THALLIUM_TCP}   \
    -DHCL_ENABLE_THALLIUM_ROCE=${HCL_ENABLE_THALLIUM_ROCE} \
    -DBUILD_TEST=ON                                        \
    ..

cmake --build . -- -j 2 VERBOSE=1 || exit 1

popd
