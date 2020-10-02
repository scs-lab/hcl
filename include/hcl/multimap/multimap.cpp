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

#ifndef INCLUDE_HCL_MULTIMAP_MULTIMAP_CPP_
#define INCLUDE_HCL_MULTIMAP_MULTIMAP_CPP_

/* Constructor to deallocate the shared memory*/
template<typename KeyType, typename MappedType, typename Compare>
multimap<KeyType, MappedType, Compare>::~multimap() {
    this->container::~container();
}

template<typename KeyType, typename MappedType, typename Compare>
multimap<KeyType, MappedType,
         Compare>::multimap(CharStruct name_, uint16_t port)
                 : container(name_,port),mymap() {
    AutoTrace trace = AutoTrace("hcl::multimap");
    if (is_server) {
        construct_shared_memory();
        bind_functions();
    }else if (!is_server && server_on_node) {
        open_shared_memory();
    }
}

/**
 * Put the data into the local multimap.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool multimap<KeyType, MappedType, Compare>::LocalPut(KeyType &key,
                                                      MappedType &data) {
    AutoTrace trace = AutoTrace("hcl::multimap::Put(local)", key, data);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MyMap::iterator iterator = mymap->find(key);
    if (iterator != mymap->end()) {
        mymap->erase(iterator);
    }
    mymap->insert(std::pair<KeyType, MappedType>(key, data));
    return true;
}

/**
 * Put the data into the multimap. Uses key to decide the server to hash it
 * to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool multimap<KeyType, MappedType, Compare>::Put(KeyType &key,
                                                 MappedType &data) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (is_local(key_int)) {
        return LocalPut(key, data);
    } else {
        AutoTrace trace = AutoTrace("hcl::multimap::Put(remote)", key,
                                    data);
        return RPC_CALL_WRAPPER("_Put", key_int, bool,
                                key, data);
    }
}

/**
 * Get the data in the local multimap.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
multimap<KeyType, MappedType, Compare>::LocalGet(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::multimap::Get(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    typename MyMap::iterator iterator = mymap->find(key);
    if (iterator != mymap->end()) {
        return std::pair<bool, MappedType>(true, iterator->second);
    } else {
        return std::pair<bool, MappedType>(false, MappedType());
    }
}

/**
 * Get the data into the multimap. Uses key to decide the server to hash it
 * to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
multimap<KeyType, MappedType, Compare>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (is_local(key_int)) {
        return LocalGet(key);
    } else {
        AutoTrace trace = AutoTrace("hcl::multimap::Get(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Get", key_int, ret_type,
                                key);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
multimap<KeyType, MappedType, Compare>::LocalErase(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::multimap::Erase(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    size_t s = mymap->erase(key);
    return std::pair<bool, MappedType>(s > 0, MappedType());
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
multimap<KeyType, MappedType, Compare>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (is_local(key_int)) {
        return LocalErase(key);
    } else {
        AutoTrace trace = AutoTrace("hcl::multimap::Erase(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Erase", key_int, ret_type, key);
    }
}

/**
 * Get the data in the multimap. Uses key to decide the server to hash it
 * to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType, Compare>::Contains(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::multimap::Contains", key);
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = ContainsInServer(key);
    final_values.insert(final_values.end(), current_server.begin(),
                        current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
            typedef std::vector<std::pair<KeyType, MappedType>> ret_type;
            auto server = RPC_CALL_WRAPPER("_Contains", i, ret_type,
                                           key);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType, Compare>::GetAllData() {
    AutoTrace trace = AutoTrace("hcl::multimap::GetAllData");
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = GetAllDataInServer();
    final_values.insert(final_values.end(), current_server.begin(),
                        current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
            typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
            auto server = RPC_CALL_WRAPPER1("_GetAllData", i, ret_type);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType,
         Compare>::LocalContainsInServer(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::multimap::ContainsInServer", key);
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
                lock(*mutex);
        typename MyMap::iterator lower_bound;
        size_t size = mymap->size();
        if (size == 0) {
        } else if (size == 1) {
            lower_bound = mymap->begin();
            final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(
                lower_bound->first, lower_bound->second));
        } else {
            lower_bound = mymap->lower_bound(key);
            if (lower_bound == mymap->end()) return final_values;
            if (lower_bound != mymap->begin()) {
                --lower_bound;
                if (!key.Contains(lower_bound->first)) lower_bound++;
            }
            while (lower_bound != mymap->end()) {
                if (!(key.Contains(lower_bound->first) ||
                      lower_bound->first.Contains(key))) break;
                final_values.insert(final_values.end(), std::pair<KeyType,
                                    MappedType>(lower_bound->first,
                                                lower_bound->second));
                lower_bound++;
            }
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType,
         Compare>::ContainsInServer(KeyType &key) {
    if (is_local()) {
        return LocalContainsInServer(key);
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER("_Contains", my_server_i, ret_type,
                                key);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType, Compare>::LocalGetAllDataInServer() {
    AutoTrace trace = AutoTrace("hcl::multimap::GetAllDataInServer");
    std::vector<std::pair<KeyType, MappedType>> final_values =
            std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
                lock(*mutex);
        typename MyMap::iterator lower_bound;
        lower_bound = mymap->begin();
        while (lower_bound != mymap->end()) {
            final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(
                lower_bound->first, lower_bound->second));
            lower_bound++;
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
multimap<KeyType, MappedType, Compare>::GetAllDataInServer() {
    if (is_local()) {
        return LocalGetAllDataInServer();
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
void multimap<KeyType, MappedType, Compare>::construct_shared_memory() {
    ShmemAllocator alloc_inst(segment.get_segment_manager());
    /* Construct Multimap in the shared memory space. */
    mymap = segment.construct<MyMap>(name.c_str())(Compare(), alloc_inst);
}

template<typename KeyType, typename MappedType, typename Compare>
void multimap<KeyType, MappedType, Compare>::open_shared_memory() {
    std::pair<MyMap*, boost::interprocess:: managed_mapped_file::size_type>
            res;
    res = segment.find<MyMap>(name.c_str());
    mymap = res.first;
}

template<typename KeyType, typename MappedType, typename Compare>
void multimap<KeyType, MappedType, Compare>::bind_functions() {
    /* Create a RPC server and map the methods to it. */
    switch (HCL_CONF->RPC_IMPLEMENTATION) {
#ifdef HCL_ENABLE_RPCLIB
        case RPCLIB: {
            std::function<bool(KeyType &, MappedType &)> putFunc(
                    std::bind(&multimap<KeyType, MappedType, Compare>::LocalPut, this,
                              std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool, MappedType>(KeyType &)> getFunc(
                    std::bind(&multimap<KeyType, MappedType, Compare>::LocalGet, this,
                              std::placeholders::_1));
            std::function<std::pair<bool, MappedType>(KeyType &)> eraseFunc(
                    std::bind(&multimap<KeyType, MappedType, Compare>::LocalErase, this,
                              std::placeholders::_1));
            std::function<std::vector<std::pair<KeyType, MappedType>>(void)>
                    getAllDataInServerFunc(std::bind(
                    &multimap<KeyType, MappedType, Compare>::LocalGetAllDataInServer,
                    this));
            std::function<std::vector<std::pair<KeyType, MappedType>>(KeyType &)>
                    containsInServerFunc(std::bind(&multimap<KeyType, MappedType,
                                                           Compare>::LocalContainsInServer, this,
                                                   std::placeholders::_1));

            rpc->bind(func_prefix+"_Put", putFunc);
            rpc->bind(func_prefix+"_Get", getFunc);
            rpc->bind(func_prefix+"_Erase", eraseFunc);
            rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
            rpc->bind(func_prefix+"_Contains", containsInServerFunc);
            break;
        }
#endif
#ifdef HCL_ENABLE_THALLIUM_TCP
            case THALLIUM_TCP:
#endif
#ifdef HCL_ENABLE_THALLIUM_ROCE
            case THALLIUM_ROCE:
#endif
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
            {

                    std::function<void(const tl::request &, KeyType &, MappedType &)> putFunc(
                        std::bind(&multimap<KeyType, MappedType, Compare>::ThalliumLocalPut, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3));
                    std::function<void(const tl::request &, KeyType &)> getFunc(
                        std::bind(&multimap<KeyType, MappedType, Compare>::ThalliumLocalGet, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &, KeyType &)> eraseFunc(
                        std::bind(&multimap<KeyType, MappedType, Compare>::ThalliumLocalErase, this,
                                  std::placeholders::_1, std::placeholders::_2));
                    std::function<void(const tl::request &)>
                            getAllDataInServerFunc(std::bind(
                                &multimap<KeyType, MappedType, Compare>::ThalliumLocalGetAllDataInServer,
                                this, std::placeholders::_1));
                    std::function<void(const tl::request &, KeyType &)>
                            containsInServerFunc(std::bind(&multimap<KeyType, MappedType,
                                                           Compare>::ThalliumLocalContainsInServer, this,
                                                           std::placeholders::_1,
							   std::placeholders::_2));

                    rpc->bind(func_prefix+"_Put", putFunc);
                    rpc->bind(func_prefix+"_Get", getFunc);
                    rpc->bind(func_prefix+"_Erase", eraseFunc);
                    rpc->bind(func_prefix+"_GetAllData", getAllDataInServerFunc);
                    rpc->bind(func_prefix+"_Contains", containsInServerFunc);
                    break;
                }
#endif
    }
}

#endif  // INCLUDE_HCL_MULTIMAP_MULTIMAP_CPP_