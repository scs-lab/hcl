#!/bin/bash

THALLIUM_VERSION=0.8.3
MERCURY_VERSION=2.0.0
MPICH_VERSION=3.2.1
RPCLIB_VERSION=2.2.1
BOOST_VERSION=1.74.0
GCC_VERSION=9.3.0

GCC_SPEC="gcc@${GCC_VERSION}"
MPICH_SPEC="mpich@${MPICH_VERSION}%${GCC_SPEC}"
MERCURY_SPEC="mercury@${MERCURY_VERSION}%${GCC_SPEC}"
THALLIUM_SPEC="mochi-thallium~cereal@${THALLIUM_VERSION}%${GCC_SPEC}"
RPCLIB_SPEC="rpclib@${RPCLIB_VERSION}%${GCC_SPEC}"
BOOST_SPEC="boost@${BOOST_VERSION}%${GCC_SPEC}"

LOCAL=local
INSTALL_DIR="${HOME}/${LOCAL}/install"
SPACK_DIR=${HOME}/${LOCAL}/spack
rm -r ${INSTALL_DIR}
mkdir -p ${INSTALL_DIR}

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

spack view symlink ${INSTALL_DIR} ${GCC_SPEC} ${MPICH_SPEC} ${THALLIUM_SPEC} ${RPCLIB_SPEC} ${BOOST_SPEC}


mkdir build
pushd build

CXXFLAGS="-I${INSTALL_DIR}/include -fsanitize=address -O1 -fno-omit-frame-pointer -g" \
LDFLAGS="-L${INSTALL_DIR}/lib"                                       \
CMAKE_C_COMPILER=${INSTALL_DIR}/bin/gcc                              \
CMAKE_CXX_COMPILER=${INSTALL_DIR}/bin/g++

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
