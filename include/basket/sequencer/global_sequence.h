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

#ifndef INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
#define INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_

#include <basket/communication/rpc_lib.h>
#include <basket/communication/rpc_factory.h>
#include <basket/common/singleton.h>
#include <stdint-gcc.h>
#include <mpi.h>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <utility>
#include <memory>
#include <string>
#include <boost/interprocess/managed_mapped_file.hpp>

namespace bip = boost::interprocess;

namespace basket {
class global_sequence {
  private:
    uint64_t* value;
    bool is_server;
    bip::interprocess_mutex* mutex;
    int my_rank, comm_size, num_servers;
    uint16_t my_server;
    really_long memory_allocated;
    bip::managed_mapped_file segment;
    std::string name, func_prefix;
    std::shared_ptr<RPC> rpc;
    bool server_on_node;
    CharStruct backed_file;

  public:
    ~global_sequence() {
        if (is_server) bip::file_mapping::remove(backed_file.c_str());
    }
    global_sequence(std::string name_ = "TEST_GLOBAL_SEQUENCE", uint16_t port=BASKET_CONF->RPC_PORT)
            : is_server(BASKET_CONF->IS_SERVER), my_server(BASKET_CONF->MY_SERVER),
              num_servers(BASKET_CONF->NUM_SERVERS),
              comm_size(1), my_rank(0), memory_allocated(BASKET_CONF->MEMORY_ALLOCATED),
              backed_file(BASKET_CONF->BACKED_FILE_DIR + PATH_SEPARATOR + name_),
              name(name_), segment(),
              func_prefix(name_),
              server_on_node(BASKET_CONF->SERVER_ON_NODE) {
        AutoTrace trace = AutoTrace("basket::global_sequence");
        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        name = name+"_"+std::to_string(my_server);
        rpc = Singleton<RPCFactory>::GetInstance()->GetRPC(port);
        if (is_server) {
            switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
                case RPCLIB: {
                std::function<uint64_t(void)> getNextSequence(std::bind(
                    &basket::global_sequence::LocalGetNextSequence, this));
                rpc->bind(func_prefix+"_GetNextSequence", getNextSequence);
                break;
            }
#endif
#ifdef BASKET_ENABLE_THALLIUM_TCP
                case THALLIUM_TCP:
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
                    case THALLIUM_ROCE:
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
                {
                    std::function<void(const tl::request &)> getNextSequence(std::bind(
                            &basket::global_sequence::ThalliumLocalGetNextSequence, this,
                            std::placeholders::_1));
                    rpc->bind(func_prefix+"_GetNextSequence", getNextSequence);
                    break;
                }
#endif
            }

            boost::interprocess::file_mapping::remove(backed_file.c_str());
            segment = bip::managed_mapped_file(bip::create_only, backed_file.c_str(), 65536);
            value = segment.construct<uint64_t>(name.c_str())(0);
            mutex = segment.construct<boost::interprocess::interprocess_mutex>(
                    "mtx")();
        }else if (!is_server && server_on_node) {
            segment = bip::managed_mapped_file(bip::open_only, backed_file.c_str());
            std::pair<uint64_t*, bip::managed_mapped_file::size_type> res;
            res = segment.find<uint64_t> (name.c_str());
            value = res.first;
            std::pair<bip::interprocess_mutex *,
                    bip::managed_mapped_file::size_type> res2;
            res2 = segment.find<bip::interprocess_mutex>("mtx");
            mutex = res2.first;
        }
    }
    uint64_t * data(){
        if(server_on_node || is_server) return value;
        else nullptr;
    }
    void lock(){
        if(server_on_node || is_server) mutex->lock();
    }

    void unlock(){
        if(server_on_node || is_server) mutex->unlock();
    }
    uint64_t GetNextSequence(){
        if (server_on_node) {
            return LocalGetNextSequence();
        }
        else {
            auto my_server_i = my_server;
            return RPC_CALL_WRAPPER1("_GetNextSequence", my_server_i, uint64_t);
        }
    }
    uint64_t GetNextSequenceServer(uint16_t &server){
        if (my_server == server && server_on_node) {
            return LocalGetNextSequence();
        }
        else {
            return RPC_CALL_WRAPPER1("_GetNextSequence", server, uint64_t);
        }
    }

    uint64_t LocalGetNextSequence() {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
                lock(*mutex);
        return ++*value;
    }

#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    THALLIUM_DEFINE1(LocalGetNextSequence)
#endif

};

}  // namespace basket

#endif  // INCLUDE_BASKET_SEQUENCER_GLOBAL_SEQUENCE_H_
