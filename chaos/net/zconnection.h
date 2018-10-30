/*******************************************************************************
**                                  LibChaos                                  **
**                                zconnection.h                               **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#ifndef ZCONNECTION_H
#define ZCONNECTION_H

#include "ztypes.h"
#include "zstreamsocket.h"
#include "zaddress.h"

namespace LibChaos {

/*! TCP connection abstraction of ZSocket.
 *  \ingroup Network
 */
class ZConnection : private ZStreamSocket {
public:
    //! Construct unopened connection.
    ZConnection();
    //! Construct from opened socket connected to addr.
    ZConnection(zsocktype fd, ZAddress addr);
    //! Destructor will close the connection socket.
    ~ZConnection();

    // Functions imported from ZSocket
    using ZStreamSocket::close;
    using ZStreamSocket::isOpen;

    using ZStreamSocket::read;
    using ZStreamSocket::write;

    using ZStreamSocket::setBlocking;
    using ZStreamSocket::getSocket;

    bool connect(ZAddress addr);

    //! Get the address of the peer.
    ZAddress peer();

private:
    ZAddress _peeraddr;
    unsigned char *buffer;
};

} // namespace LibChaos

#endif // ZCONNECTION_H
