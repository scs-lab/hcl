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
//
// Created by hariharan on 8/16/19.
//

#ifndef HCL_RPC_FACTORY_H
#define HCL_RPC_FACTORY_H

#include <unordered_map>
#include <memory>
#include "rpc_lib.h"

class RPCFactory{
private:
    std::unordered_map<uint16_t,std::shared_ptr<RPC>> rpcs;
public:
    RPCFactory():rpcs(){}
    std::shared_ptr<RPC> GetRPC(uint16_t server_port){
        auto iter = rpcs.find(server_port);
        if(iter!=rpcs.end()) return iter->second;
        auto temp = HCL_CONF->RPC_PORT;
        HCL_CONF->RPC_PORT=server_port;
        auto rpc = std::make_shared<RPC>();
        rpcs.emplace(server_port,rpc);
        HCL_CONF->RPC_PORT=temp;
        return rpc;
    }
};
#endif //HCL_RPC_FACTORY_H
