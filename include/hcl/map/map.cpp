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

#ifndef INCLUDE_HCL_MAP_MAP_CPP_
#define INCLUDE_HCL_MAP_MAP_CPP_

/**
 * Put the data into the local map.
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool map<KeyType, MappedType, Compare>::LocalPut(KeyType &key,
                                                 MappedType &data) {
    AutoTrace trace = AutoTrace("hcl::map::Put(local)", key, data);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
    mymap->insert_or_assign(key, data);
    /*typename MyMap::iterator iterator = mymap->find(key);
      if (iterator != mymap->end()) {
      mymap->erase(iterator);
      }
      mymap->insert(std::pair<KeyType, MappedType>(key, data));*/
    return true;
}

/**
 * Put the data into the map. Uses key to decide the server to hash it to,
 * @param key, the key for put
 * @param data, the value for put
 * @return bool, true if Put was successful else false.
 */
template<typename KeyType, typename MappedType, typename Compare>
bool map<KeyType, MappedType, Compare>::Put(KeyType &key,
                                            MappedType &data) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = static_cast<uint16_t>(key_hash % num_servers);
    if (key_int == my_server && server_on_node) {
        return LocalPut(key, data);
    } else {
        AutoTrace trace = AutoTrace("hcl::map::Put(remote)", key, data);
        return RPC_CALL_WRAPPER("_Put", key_int, bool,
                                key, data);
    }
}

/**
 * Get the data in the local map.
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::LocalGet(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::map::Get(local)", key);
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
 * Get the data in the map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then
 * data was found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::Get(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalGet(key);
    } else {
        AutoTrace trace = AutoTrace("hcl::map::Get(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Get", key_int, ret_type,
                                key);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::LocalErase(KeyType &key) {
    AutoTrace trace = AutoTrace("hcl::map::Erase(local)", key);
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>
            lock(*mutex);
    size_t s = mymap->erase(key);
    return std::pair<bool, MappedType>(s > 0, MappedType());
}

template<typename KeyType, typename MappedType, typename Compare>
std::pair<bool, MappedType>
map<KeyType, MappedType, Compare>::Erase(KeyType &key) {
    size_t key_hash = keyHash(key);
    uint16_t key_int = key_hash % num_servers;
    if (key_int == my_server && server_on_node) {
        return LocalErase(key);
    } else {
        AutoTrace trace = AutoTrace("hcl::map::Erase(remote)", key);
        typedef std::pair<bool, MappedType> ret_type;
        return RPC_CALL_WRAPPER("_Erase", key_int, ret_type, key);
    }
}

/**
 * Get the data into the map. Uses key to decide the server to hash it to,
 * @param key, key to get
 * @return return a pair of bool and Value. If bool is true then data was
 * found and is present in value part else bool is set to false
 */
template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::Contains(KeyType &key_start,KeyType &key_end) {
    AutoTrace trace = AutoTrace("hcl::map::Contains", key_start,key_end);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = ContainsInServer(key_start,key_end);
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers; ++i) {
        if (i != my_server) {
            typedef std::vector<std::pair<KeyType, MappedType>> ret_type;
            auto server = RPC_CALL_WRAPPER("_Contains", i, ret_type, key_start,key_end);
            final_values.insert(final_values.end(), server.begin(), server.end());
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::GetAllData() {
    AutoTrace trace = AutoTrace("hcl::map::GetAllData");
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    auto current_server = GetAllDataInServer();
    final_values.insert(final_values.end(), current_server.begin(), current_server.end());
    for (int i = 0; i < num_servers ; ++i) {
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
map<KeyType, MappedType, Compare>::LocalContainsInServer(KeyType &key_start,KeyType &key_end) {
    AutoTrace trace = AutoTrace("hcl::map::ContainsInServer", key_start,key_end);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
        typename MyMap::iterator lower_bound;
        size_t size = mymap->size();
        if (size == 0) {
        } else if (size == 1) {
            lower_bound = mymap->begin();

            if(lower_bound->first > key_start)
                final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(lower_bound->first, lower_bound->second));
        } else {
            lower_bound = mymap->lower_bound(key_start);
            if (lower_bound == mymap->end()) return final_values;
            if (lower_bound != mymap->begin()) {
                --lower_bound;
                if (key_start > lower_bound->first) lower_bound++;
            }
            while (lower_bound != mymap->end()) {
                if (lower_bound->first > key_end) break;
                final_values.insert(final_values.end(), std::pair<KeyType, MappedType>(lower_bound->first, lower_bound->second));
                lower_bound++;
            }
        }
    }
    return final_values;
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::ContainsInServer(KeyType &key_start,KeyType &key_end) {
    if (server_on_node) {
        return LocalContainsInServer(key_start,key_end);
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER("_Contains", my_server_i, ret_type, key_start,key_end);
    }
}

template<typename KeyType, typename MappedType, typename Compare>
std::vector<std::pair<KeyType, MappedType>>
map<KeyType, MappedType, Compare>::LocalGetAllDataInServer() {
    AutoTrace trace = AutoTrace("hcl::map::GetAllDataInServer", NULL);
    auto final_values = std::vector<std::pair<KeyType, MappedType>>();
    {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mutex);
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
map<KeyType, MappedType, Compare>::GetAllDataInServer() {
    if (server_on_node) {
        return LocalGetAllDataInServer();
    }
    else {
        typedef std::vector<std::pair<KeyType, MappedType> > ret_type;
        auto my_server_i = my_server;
        return RPC_CALL_WRAPPER1("_GetAllData", my_server_i, ret_type);
   }
}




#endif  // INCLUDE_HCL_MAP_MAP_CPP_
