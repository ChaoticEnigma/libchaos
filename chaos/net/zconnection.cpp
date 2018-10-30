/*******************************************************************************
**                                  LibChaos                                  **
**                               zconnection.cpp                              **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zconnection.h"
#include "zlog.h"

namespace LibChaos {

ZConnection::ZConnection() : ZStreamSocket(), buffer(nullptr){

}

ZConnection::ZConnection(zsocktype fd, ZAddress addr) : ZStreamSocket(fd), _peeraddr(addr), buffer(nullptr){

}

ZConnection::~ZConnection(){
    // Close the socket
    close();
    delete buffer;
}

bool ZConnection::connect(ZAddress addr){
    zsocktype connfd;
    return ZStreamSocket::connect(addr, connfd, _peeraddr);
}

ZAddress ZConnection::peer(){
    return _peeraddr;
}

}
