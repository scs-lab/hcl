/*
 * Copyright (C) 2019  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Keith Bateman
 * <kbateman@hawk.iit.edu>, Xian-He Sun <sun@iit.edu>
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

#ifndef INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_
#define INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_

#include <basket/common/constants.h>
#include <basket/common/data_structures.h>
#include <basket/common/debug.h>
#include <basket/common/macros.h>
#include <basket/common/singleton.h>
#include <basket/common/typedefs.h>
#include <mpi.h>

/** RPC Lib Headers**/
#ifdef BASKET_ENABLE_RPCLIB
#include <rpc/server.h>
#include <rpc/client.h>
#include <rpc/rpc_error.h>
#endif
/** Thallium Headers **/
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
#include <thallium.hpp>
#include <thallium/serialization/serialize.hpp>
#include <thallium/serialization/buffer_input_archive.hpp>
#include <thallium/serialization/buffer_output_archive.hpp>
#include <thallium/serialization/stl/array.hpp>
#include <thallium/serialization/stl/complex.hpp>
#include <thallium/serialization/stl/deque.hpp>
#include <thallium/serialization/stl/forward_list.hpp>
#include <thallium/serialization/stl/list.hpp>
#include <thallium/serialization/stl/map.hpp>
#include <thallium/serialization/stl/multimap.hpp>
#include <thallium/serialization/stl/multiset.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/set.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/tuple.hpp>
#include <thallium/serialization/stl/unordered_map.hpp>
#include <thallium/serialization/stl/unordered_multimap.hpp>
#include <thallium/serialization/stl/unordered_multiset.hpp>
#include <thallium/serialization/stl/unordered_set.hpp>
#include <thallium/serialization/stl/vector.hpp>
#endif

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <future>

namespace bip = boost::interprocess;
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
namespace tl = thallium;
#endif

class RPC {
private:
    uint16_t server_port;
    std::string name;
#ifdef BASKET_ENABLE_RPCLIB
    std::shared_ptr<rpc::server> rpclib_server;
    // We can't use a std::vector<rpc::client> for these since rpc::client is neither copy
    // nor move constructible. See https://github.com/rpclib/rpclib/issues/128
    std::vector<std::shared_ptr<rpc::client>> rpclib_clients;
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    std::shared_ptr<tl::engine> thallium_engine;
    CharStruct engine_init_str;
    std::vector<tl::endpoint> thallium_endpoints;
    void init_engine_and_endpoints(CharStruct conf);
    /*std::promise<void> thallium_exit_signal;

      void runThalliumServer(std::future<void> futureObj){

      while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){}
      thallium_engine->wait_for_finalize();
      }*/

#endif
    std::vector<CharStruct> server_list;
  public:
    ~RPC();

    RPC() : server_list(),
             server_port(BASKET_CONF->RPC_PORT) {
    AutoTrace trace = AutoTrace("RPC");

    server_list = BASKET_CONF->LoadServers();

    /* if current rank is a server */
    if (BASKET_CONF->IS_SERVER) {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
        case RPCLIB: {
            rpclib_server = std::make_shared<rpc::server>(server_port+BASKET_CONF->MY_SERVER);
            rpclib_server->suppress_exceptions(false);
	break;
      }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
        case THALLIUM_TCP: {
	engine_init_str = BASKET_CONF->TCP_CONF + "://" +
	  BASKET_CONF->SERVER_LIST[BASKET_CONF->MY_SERVER] +
	  ":" +
	  std::to_string(server_port + BASKET_CONF->MY_SERVER);
	break;
      }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
      case THALLIUM_ROCE: {
	  engine_init_str = BASKET_CONF->VERBS_CONF + "://" +
	    BASKET_CONF->VERBS_DOMAIN + "://" +
	    BASKET_CONF->SERVER_LIST[BASKET_CONF->MY_SERVER] +
	    ":" +
	    std::to_string(server_port+BASKET_CONF->MY_SERVER);
	  break;
	}
#endif
        }
    } else {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {

                break;
            }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
            case THALLIUM_TCP: {
                init_engine_and_endpoints(BASKET_CONF->TCP_CONF);
                break;
            }
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
            case THALLIUM_ROCE: {
                init_engine_and_endpoints(BASKET_CONF->VERBS_CONF);
                break;
            }
#endif
        }
    }
    for (std::vector<rpc::client>::size_type i = 0; i < server_list.size(); ++i) {
        rpclib_clients.push_back(std::make_unique<rpc::client>(server_list[i].c_str(), server_port + i));
    }
    run(BASKET_CONF->RPC_THREADS);
}


    template <typename F>
    void bind(CharStruct str, F func);

    void run(size_t workers = RPC_THREADS);

#ifdef BASKET_ENABLE_THALLIUM_ROCE
    template<typename MappedType>
    MappedType prep_rdma_server(tl::endpoint endpoint, tl::bulk &bulk_handle);

    template<typename MappedType>
    tl::bulk prep_rdma_client(MappedType &data);
#endif
    /**
     * Response should be RPCLIB_MSGPACK::object_handle for rpclib and
     * tl::packed_response for thallium/mercury
     */
    template <typename Response, typename... Args>
    Response call(uint16_t server_index,
                  CharStruct const &func_name,
                  Args... args);
    /**
     * Response should be RPCLIB_MSGPACK::object_handle for rpclib and
     * tl::packed_response for thallium/mercury
     */
    template <typename Response, typename... Args>
    Response call(CharStruct &server,
                  uint16_t &port,
                  CharStruct const &func_name,
                  Args... args);
    template <typename Response, typename... Args>
    Response callWithTimeout(uint16_t server_index,
                  int timeout_ms,
                  CharStruct const &func_name,
                  Args... args);
    template <typename Response, typename... Args>
    std::future<Response> async_call(
            uint16_t server_index, CharStruct const &func_name, Args... args);
    template <typename Response, typename... Args>
    std::future<Response> async_call(CharStruct &server,
            uint16_t &port, CharStruct const &func_name, Args... args);

};

#include "rpc_lib.cpp"

#endif  // INCLUDE_BASKET_COMMUNICATION_RPC_LIB_H_
