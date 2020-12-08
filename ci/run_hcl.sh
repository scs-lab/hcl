#!/bin/bash

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
    mpiexec -n 2 ./unordered_map_test || exit 1
    echo "Testing unordered map string test"
    mpiexec -n 2 ./unordered_map_string_test || exit 1
    popd
fi

if [ "${HCL_ENABLE_THALLIUM_TCP}" = "ON" ]; then
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
    mpiexec -n 2 ./unordered_map_test || exit 1
    echo "Testing unordered map string test"
    mpiexec -n 2 ./unordered_map_string_test || exit 1
    popd
fi
popd
