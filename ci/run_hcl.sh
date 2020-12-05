#!/bin/bash

LOCAL=local
INSTALL_DIR="${HOME}/${LOCAL}"
SPACK_DIR=${INSTALL_DIR}/spack
INSTALL_DIR=${SPACK_DIR}/var/spack/environments/hcl/.spack-env/view
mkdir build
pushd build

if [ "${HCL_ENABLE_RPCLIB}" = "ON" ]; then
    pushd test
    # TODO(chogan): Run tests via ctest and enable Thallium tests
    echo "Testing ordered map"
    mpiexec -n 2 ./map_test || exit 1
    echo "Testing multimap"
    mpiexec -n 2 ./multimap_test || exit 1
    echo "Testing priority queue"
    mpiexec -n 2 ./priority_queue_test || exit 1
    echo "Testing queue"
    mpiexec -n 2 ./queue_test || exit 1
    echo "Testing set"
    mpiexec -n 2 ./set_test || exit 1
    echo "Testing unordered map test"
    mpiexec -n 2 ./unordered_map_test || exit
    popd

    make install || exit 1
fi

#if [ "${HCL_ENABLE_THALLIUM_TCP}" = "ON" ]; then
#    pushd test
#    # TODO(chogan): Run tests via ctest and enable Thallium tests
#    echo "Testing ordered map"
#    mpiexec -n 2 ./map_test || exit 1
#    echo "Testing multimap"
#    mpiexec -n 2 ./multimap_test || exit 1
#    echo "Testing priority queue"
#    mpiexec -n 2 ./priority_queue_test || exit 1
#    echo "Testing queue"
#    mpiexec -n 2 ./queue_test || exit 1
#    echo "Testing set"
#    mpiexec -n 2 ./set_test || exit 1
#    echo "Testing unordered map int test"
#    mpiexec -n 2 ./unordered_map_int_test || exit
#    echo "Testing unordered map string test"
#    mpiexec -n 2 ./unordered_map_string_test || exit 1
#    popd
#
#    make install || exit 1
#fi
popd
