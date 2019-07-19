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

template<typename MappedType>
queue<MappedType>::~queue() {
    if (is_server) bip::shared_memory_object::remove(name.c_str());
}

template<typename MappedType>
queue<MappedType>::queue(std::string name_,
                         bool is_server_,
                         uint16_t my_server_,
                         int num_servers_,
                         bool server_on_node_)
        : is_server(is_server_), my_server(my_server_), num_servers(num_servers_),
          comm_size(1), my_rank(0), memory_allocated(1024ULL * 1024ULL * 128ULL),
          name(name_), segment(),
          my_queue(), func_prefix(name_),
          server_on_node(server_on_node_) {
    AutoTrace trace = AutoTrace("basket::queue(local)", name_,
                                is_server_, my_server_, num_servers_);
    /* Initialize MPI rank and size of world */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* create per server name for shared memory. Needed if multiple servers are
       spawned on one node*/
    this->name += "_" + std::to_string(my_server);
    rpc = Singleton<RPC>::GetInstance("RPC_SERVER_LIST", is_server_, my_server_,
                                      num_servers_);
    if (is_server) {
        /* Delete existing instance of shared memory space*/
        bip::shared_memory_object::remove(name.c_str());
        /* allocate new shared memory space */
        segment = bip::managed_shared_memory(bip::create_only, name.c_str(),
                                             memory_allocated);
        ShmemAllocator alloc_inst(segment.get_segment_manager());
        /* Construct queue in the shared memory space. */
        my_queue = segment.construct<Queue>("Queue")(alloc_inst);
        mutex = segment.construct<bip::interprocess_mutex>("mtx")();
        /* Create a RPC server and map the methods to it. */
        std::function<bool(MappedType, uint16_t)> pushFunc(
            std::bind(&basket::queue<MappedType>::LocalPush, this,
                      std::placeholders::_1, std::placeholders::_2));
        std::function<std::pair<bool, MappedType>(uint16_t)> popFunc(std::bind(
            &queue::LocalPop, this, std::placeholders::_1));
        std::function<size_t(uint16_t)> sizeFunc(std::bind(
            &queue::LocalSize, this, std::placeholders::_1));
        std::function<bool(uint16_t)> waitForElementFunc(std::bind(
            &queue::LocalWaitForElement, this,
            std::placeholders::_1));
        rpc->bind(func_prefix+"_Push", pushFunc);
        rpc->bind(func_prefix+"_Pop", popFunc);
        rpc->bind(func_prefix+"_WaitForElement", waitForElementFunc);
        rpc->bind(func_prefix+"_Size", sizeFunc);
    }
    /* Make clients wait until all servers reach here*/
    MPI_Barrier(MPI_COMM_WORLD);
    /* Map the clients to their respective memory pools */
    if (!is_server) {
        segment = bip::managed_shared_memory(bip::open_only, name.c_str());
        std::pair<Queue*, bip::managed_shared_memory::size_type> res;
        res = segment.find<Queue> ("Queue");
        my_queue = res.first;
        std::pair<bip::interprocess_mutex *,
                  bip::managed_shared_memory::size_type> res2;
        res2 = segment.find<bip::interprocess_mutex>("mtx");
        mutex = res2.first;
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Push the data into the local queue.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename MappedType>
bool queue<MappedType>::LocalPush(MappedType data,
                                  uint16_t key_int) {
    AutoTrace trace = AutoTrace("basket::queue::Push(local)", data,
                                key_int);
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    my_queue->push_back(std::move(data));
    return true;
}

/**
 * Push the data into the queue. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename MappedType>
bool queue<MappedType>::Push(MappedType data,
                             uint16_t key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalPush(data, key_int);
    } else {
        AutoTrace trace = AutoTrace("basket::queue::Push(remote)", data,
                                    key_int);
        return rpc->call(key_int, func_prefix+"_Push", data).template as<bool>();
    }
}

/**
 * Get the local data from the queue.
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType>
std::pair<bool, MappedType>
queue<MappedType>::LocalPop(uint16_t key_int) {
    AutoTrace trace = AutoTrace("basket::queue::Pop(local)",
                                key_int);
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    if (my_queue->size() > 0) {
        MappedType value = my_queue->front();
        my_queue->pop_front();
        return std::pair<bool, MappedType>(true, value);
    }
    return std::pair<bool, MappedType>(false, MappedType());
}

/**
 * Get the data from the queue. Uses key_int to decide the server to hash it
 * to,
 * @param key_int, key_int to know which server
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename MappedType>
std::pair<bool, MappedType>
queue<MappedType>::Pop(uint16_t key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalPop(key_int);
    } else {
        AutoTrace trace = AutoTrace("basket::queue::Pop(remote)",
                                    key_int);
        return rpc->call(key_int, func_prefix+"_Pop").template
                as<std::pair<bool, MappedType>>();
    }
}

template<typename MappedType>
bool queue<MappedType>::LocalWaitForElement(uint16_t key_int) {
    AutoTrace trace = AutoTrace(
        "basket::queue::WaitForElement(local)", key_int);
    int count = 0;
    while (my_queue->size() == 0) {
        usleep(10);
        if (count == 0) printf("Server %d, No Events in Queue\n", key_int);
        count++;
    }
    return true;
}

template<typename MappedType>
bool queue<MappedType>::WaitForElement(uint16_t key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalWaitForElement(key_int);
    } else {
        AutoTrace trace = AutoTrace(
            "basket::queue::WaitForElement(remote)", key_int);
        return rpc->call(key_int, func_prefix+"_WaitForElement").template
                as<bool>();
    }
}

/**
 * Get the size of the local queue.
 * @param key_int, key_int to know which server
 * @return return a size of the queue
 */
template<typename MappedType>
size_t queue<MappedType>::LocalSize(uint16_t key_int) {
    AutoTrace trace = AutoTrace("basket::queue::Size(local)",
                                key_int);
    bip::scoped_lock<bip::interprocess_mutex> lock(*mutex);
    size_t value = my_queue->size();
    return value;
}

/**
 * Get the size of the queue. Uses key_int to decide the server to hash it to,
 * @param key_int, key_int to know which server
 * @return return a size of the queue
 */
template<typename MappedType>
size_t queue<MappedType>::Size(uint16_t key_int) {
    if (key_int == my_server && server_on_node) {
        return LocalSize(key_int);
    } else {
        AutoTrace trace = AutoTrace("basket::queue::Size(remote)",
                                    key_int);
        return rpc->call(key_int, func_prefix+"_Size").template as<size_t>();
    }
}
// template class queue<int>;
