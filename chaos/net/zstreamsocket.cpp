/*******************************************************************************
**                                  LibChaos                                  **
**                              zstreamsocket.cpp                             **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zstreamsocket.h"
#include "zconnection.h"

namespace LibChaos {

ZStreamSocket::ZStreamSocket() : ZSocket(ZSocket::STREAM){

}
ZStreamSocket::ZStreamSocket(zsocktype fd) : ZSocket(ZSocket::STREAM, fd){

}

bool ZStreamSocket::listen(ZAddress bindaddr){
    if(!ZSocket::bind(bindaddr))
        return false;
    if(!ZSocket::listen())
        return false;
    return true;
}

ZSocket::socket_error ZStreamSocket::accept(ZPointer<ZConnection> &conn){
    zsocktype connfd;
    ZAddress connaddr;
    socket_error ret = ZSocket::accept(connfd, connaddr);
    if(ret == OK){
        conn = new ZConnection(connfd, connaddr);
    } else {
        conn = nullptr;
    }
    return ret;
}

}
