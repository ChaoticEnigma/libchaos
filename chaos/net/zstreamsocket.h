/*******************************************************************************
**                                  LibChaos                                  **
**                               zstreamsocket.h                              **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#ifndef ZSTREAMSOCKET_H
#define ZSTREAMSOCKET_H

#include "zsocket.h"
#include "zpointer.h"

namespace LibChaos {

class ZConnection;

/*! TCP interface for ZSocket.
 *  \ingroup Network
 */
class ZStreamSocket : private ZSocket {
public:
    ZStreamSocket();
    ZStreamSocket(zsocktype fd);

    // Functions imported from ZSocket
    using ZSocket::close;
    using ZSocket::isOpen;

    using ZSocket::read;
    using ZSocket::write;

    using ZSocket::setAllowRebind;
    using ZSocket::setBlocking;
    using ZSocket::getBoundAddress;
    using ZSocket::getSocket;

    using ZSocket::connect;

    //! Open, bind and listen on a stream socket.
    bool listen(ZAddress bindaddr);

    //! Accept a new connection on the socket.
    socket_error accept(ZPointer<ZConnection> &conn);

    void setReadBuffer(zu64 size);
};

} // namespace LibChaos

#endif // ZSTREAMSOCKET_H
