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

#include <basket/communication/rpc_lib.h>

RPC::~RPC() {
    if (BASKET_CONF->IS_SERVER) {
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
          // Twiddle thumbs
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
                // Mercury addresses in endpoints must be freed before finalizing Thallium
                thallium_endpoints.clear();
                thallium_engine->finalize();
                break;
            }
#endif
        }
    }
}


#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
void RPC::init_engine_and_endpoints(CharStruct protocol) {
    thallium_engine = basket::Singleton<tl::engine>::GetInstance(protocol.c_str(), MARGO_CLIENT_MODE);

    thallium_endpoints.reserve(server_list.size());
    for (std::vector<CharStruct>::size_type i = 0; i < server_list.size(); ++i) {
        // We use addr lookup because mercury addresses must be exactly 15 char
        char ip[16];
        struct hostent *he = gethostbyname(server_list[i].c_str());
        in_addr **addr_list = (struct in_addr **)he->h_addr_list;
        strcpy(ip, inet_ntoa(*addr_list[0]));

        CharStruct lookup_str = protocol + "://" + std::string(ip) + ":" +
            std::to_string(server_port + i);
        thallium_endpoints.push_back(thallium_engine->lookup(lookup_str.c_str()));
    }
}
#endif

void RPC::run(size_t workers) {
    AutoTrace trace = AutoTrace("RPC::run", workers);
    if (BASKET_CONF->IS_SERVER){
        switch (BASKET_CONF->RPC_IMPLEMENTATION) {
#ifdef BASKET_ENABLE_RPCLIB
            case RPCLIB: {
                    rpclib_server->async_run(workers);
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
		  thallium_engine = basket::Singleton<tl::engine>::GetInstance(engine_init_str.c_str(), THALLIUM_SERVER_MODE,true,BASKET_CONF->RPC_THREADS);
                    break;
                }
#endif
        }
    }
}
