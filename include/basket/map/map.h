/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of Basket
 * 
 * Basket is free software: you can redistribute it and/or modify
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

#ifndef INCLUDE_BASKET_MAP_MAP_H_
#define INCLUDE_BASKET_MAP_MAP_H_

/**
 * Include Headers
 */

#include <basket/communication/rpc_lib.h>
#include <basket/common/singleton.h>
#include <basket/common/debug.h>
/** MPI Headers**/
#include <mpi.h>
/** RPC Lib Headers**/
#include <rpc/server.h>
#include <rpc/client.h>
/** Boost Headers **/
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
/** Standard C++ Headers**/
#include <iostream>
#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <map>
#include <vector>

namespace basket {
/**
 * This is a Distributed Map Class. It uses shared memory + RPC + MPI to
 * achieve the data structure.
 *
 * @tparam MappedType, the value of the Map
 */

template<typename KeyType, typename MappedType, typename Compare =
         std::less<KeyType>>
class map {
  private:
    std::hash<KeyType> keyHash;
    /** Class Typedefs for ease of use **/
    typedef std::pair<const KeyType, MappedType> ValueType;
    typedef boost::interprocess::allocator<
        ValueType, boost::interprocess::managed_shared_memory::segment_manager>
    ShmemAllocator;
    typedef boost::interprocess::map<KeyType, MappedType, Compare, ShmemAllocator>
    MyMap;
    /** Class attributes**/
    int comm_size, my_rank, num_servers;
    uint16_t  my_server;
    std::shared_ptr<RPC> rpc;
    really_long memory_allocated;
    bool is_server;
    boost::interprocess::managed_shared_memory segment;
    std::string name, func_prefix;
    MyMap *mymap;
    boost::interprocess::interprocess_mutex* mutex;
    bool server_on_node;

    bool LocalPut(KeyType key, MappedType data);
    std::pair<bool, MappedType> LocalGet(KeyType key);
    std::pair<bool, MappedType> LocalErase(KeyType key);
    std::vector<std::pair<KeyType, MappedType>> LocalGetAllDataInServer();
    std::vector<std::pair<KeyType, MappedType>> LocalContainsInServer(KeyType key);

  public:
    ~map();

    map();
    explicit map(std::string name_, bool is_server_,
                 uint16_t my_server_, int num_servers_,
                 bool server_on_node_);
    bool Put(KeyType key, MappedType data);
    std::pair<bool, MappedType> Get(KeyType key);

    std::pair<bool, MappedType> Erase(KeyType key);
    std::vector<std::pair<KeyType, MappedType>> Contains(KeyType key);

    std::vector<std::pair<KeyType, MappedType>> GetAllData();

    std::vector<std::pair<KeyType, MappedType>> ContainsInServer(KeyType key);
    std::vector<std::pair<KeyType, MappedType>> GetAllDataInServer();
};

#include "map.cpp"

}  // namespace basket

#endif  // INCLUDE_BASKET_MAP_MAP_H_
