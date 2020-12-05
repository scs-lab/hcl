#!/bin/bash

LOCAL=local
INSTALL_DIR="${HOME}/${LOCAL}"
SPACK_DIR=${INSTALL_DIR}/spack
INSTALL_DIR=${SPACK_DIR}/var/spack/environments/hcl/.spack-env/view
mkdir build
pushd build

CXXFLAGS="-I${INSTALL_DIR}/include -fsanitize=address -O1 -fno-omit-frame-pointer -g" \
LDFLAGS="-L${INSTALL_DIR}/lib"                                       \
cmake                                                      \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}                        \
    -DCMAKE_BUILD_RPATH="${INSTALL_DIR}/lib"                     \
    -DCMAKE_INSTALL_RPATH="${INSTALL_DIR}/lib"                   \
    -DCMAKE_BUILD_TYPE=Release                             \
    -DCMAKE_CXX_COMPILER=`which mpicxx`                    \
    -DCMAKE_C_COMPILER=`which mpicc`                       \
    -DHCL_ENABLE_RPCLIB=${HCL_ENABLE_RPCLIB}               \
    -DHCL_ENABLE_THALLIUM_TCP=${HCL_ENABLE_THALLIUM_TCP}   \
    -DHCL_ENABLE_THALLIUM_ROCE=${HCL_ENABLE_THALLIUM_ROCE} \
    -DBUILD_TEST=ON                                        \
    ..

cmake --build . -- -j 2 VERBOSE=1 || exit 1

popd
