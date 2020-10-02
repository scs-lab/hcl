/*
 * Copyright (C) 2019  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Keith Bateman
 * <kbateman@hawk.iit.edu>, Xian-He Sun <sun@iit.edu>
 *
 * This file is part of HCL
 * 
 * HCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_HCL_PRIORITY_QUEUE_PRIORITY_QUEUE_H_
#define INCLUDE_HCL_PRIORITY_QUEUE_PRIORITY_QUEUE_H_

/**
 * Include Headers
 */
#include <hcl/communication/rpc_lib.h>
#include <hcl/communication/rpc_factory.h>
#include <hcl/common/singleton.h>
#include <hcl/common/debug.h>
#include <hcl/common/typedefs.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#ifdef HCL_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#endif

/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <queue>
#include <string>
#include <memory>
#include <vector>
#include <hcl/common/container.h>

/** Namespaces Uses **/
namespace bip = boost::interprocess;

/** Global Typedefs **/

namespace hcl {
/**
 * This is a Distributed priority_queue Class. It uses shared memory + RPC + MPI
 * to achieve the data structure.
 *
 * @tparam MappedType, the value of the priority_queue
 */
template<typename MappedType, typename Compare = std::less<MappedType>>
class priority_queue:public container {
  private:
    /** Class Typedefs for ease of use **/
    typedef bip::allocator<MappedType, bip::managed_mapped_file::segment_manager>
    ShmemAllocator;
    typedef std::priority_queue<MappedType, std::vector<MappedType, ShmemAllocator>, Compare> Queue;

    /** Class attributes**/
    Queue *queue;
  public:
    ~priority_queue();

    void construct_shared_memory() override;

    void open_shared_memory() override;

    void bind_functions() override;

    explicit priority_queue(CharStruct name_ = "TEST_PRIORITY_QUEUE", uint16_t port=HCL_CONF->RPC_PORT);
    Queue * data(){
        if(server_on_node || is_server) return queue;
        else nullptr;
    }
    bool LocalPush(MappedType &data);
    std::pair<bool, MappedType> LocalPop();
    std::pair<bool, MappedType> LocalTop();
    size_t LocalSize();

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE(LocalPush, (data), MappedType &data)
    THALLIUM_DEFINE1(LocalPop)
    THALLIUM_DEFINE1(LocalTop)
    THALLIUM_DEFINE1(LocalSize)
#endif

    bool Push(MappedType &data, uint16_t &key_int);
    std::pair<bool, MappedType> Pop(uint16_t &key_int);
    std::pair<bool, MappedType> Top(uint16_t &key_int);
    size_t Size(uint16_t &key_int);
};

#include "priority_queue.cpp"

}  // namespace hcl

#endif  // INCLUDE_HCL_PRIORITY_QUEUE_PRIORITY_QUEUE_H_
