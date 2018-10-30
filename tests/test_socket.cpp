#include "tests.h"
#include "zfile.h"
#include "zdatagramsocket.h"
#include "zstreamsocket.h"
#include "zconnection.h"
#include "zmultiplex.h"
//#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "zerror.h"

namespace LibChaosTest {

static bool run = true;

void stopHandler(ZError::zerror_signal sig){
    run = false;
    TASSERT(false);
}

void udp_client(){
    LOG("=== UDP Socket Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZDatagramSocket sock;
    if(!sock.open()){
        ELOG("Socket Open Fail");
        TASSERT(false);
    }
    LOG("Sending...");

    ZAddress addr("127.0.0.1", 8998);

    ZString dat = "Hello World out There! ";
    zu64 count = 0;

    for(zu64 i = 0; run && i < 5000; ++i){
        ZString str = dat + ZString::ItoS(i);
        ZBinary data((const unsigned char *)str.cc(), str.size());
        if(sock.send(addr, data)){
            LOG("to " << addr.debugStr() << " (" << data.size() << "): \"" << data << "\"");
            ZAddress sender;
            ZBinary recvdata;
            if(sock.receive(sender, recvdata)){
                LOG("from " << sender.str() << " (" << recvdata.size() << "): \"" << recvdata << "\"");
                count++;
            } else {
                continue;
            }
        } else {
            LOG("failed to send to " << addr.debugStr());
        }
        ZThread::usleep(500000);
    }

    TLOG("Sent " << count);
    sock.close();
}

void udp_server(){
    LOG("=== UDP Socket Server Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZDatagramSocket sock;
    ZAddress bind(8998);
    DLOG(bind.debugStr());
    if(!sock.open()){
        ELOG("Socket Open Fail");
        TASSERT(false);
    }
    if(!sock.bind(bind)){
        ELOG("Socket Bind Fail");
        TASSERT(false);
    }

    //sock.setBlocking(false);

    //int out;
    //int len = sizeof(out);
    //getsockopt(sock.getHandle(), SOL_SOCKET, SO_REUSEADDR, &out, (socklen_t*)&len);
    //LOG(out);

    LOG("Listening...");
    //sock.listen(receivedGram);

    zu64 count = 0;

    while(run){
        ZAddress sender;
        ZBinary data;
        if(sock.receive(sender, data)){
            LOG("from " << sender.str() << " (" << data.size() << "): \"" << data << "\"");
            count++;
            ZAddress addr = sender;
            if(sock.send(addr, data)){
                LOG("to " << addr.debugStr() << " (" << data.size() << "): \"" << data << "\"");
            } else {
                LOG("failed to send to " << addr.str());
            }
        } else {
            LOG("error receiving message: " << ZError::getSystemError());
            continue;
        }
    }

    TLOG("Received " << count);
    sock.close();
}

#define TCP_PORT    8998
#define TCP_SIZE    1024
#define TCP_DAT1    "ping"
#define TCP_DAT2    "pong"

void tcp_client(){
    LOG("=== TCP Socket Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZAddress addr("::1", TCP_PORT);
    //ZAddress addr("127.0.0.1", TCP_PORT);

    ZConnection conn;
    LOG("Connecting to " << addr.str());
    if(!conn.connect(addr)){
        ELOG("Socket Connect Fail");
        TASSERT(false);
    }
    LOG("connected " << conn.peer().str());

    ZSocket::socket_error err;

    ZString str = TCP_DAT1;
    ZBinary snddata(str);
    err = conn.write(snddata);
    if(err != ZSocket::OK){
        ELOG(ZSocket::errorStr(err));
        TASSERT(false);
    }
    LOG("write (" << snddata.size() << "): \"" << snddata.printable().asChar() << "\"");

    ZBinary data(TCP_SIZE);
    do {
        err = conn.read(data);
    } while(err == ZSocket::DONE);
    if(err != ZSocket::OK){
        ELOG(ZSocket::errorStr(err));
        TASSERT(false);
    }
    ZString recv = ZString(data.printable().asChar());
    LOG("read (" << data.size() << "): \"" << recv << "\"");

    TASSERT(recv == TCP_DAT2);
}

void tcp_server(){
    LOG("=== TCP Socket Server Test...");
    ZError::registerInterruptHandler(stopHandler);
    ZError::registerSignalHandler(ZError::TERMINATE, stopHandler);

    ZStreamSocket sock;
    ZAddress bind(TCP_PORT);
    LOG(bind.debugStr());
    if(!sock.listen(bind)){
        ELOG("Socket Listen Fail");
        TASSERT(false);
    }

    LOG("Listening...");

    bool ok = false;
    while(run){
        ZPointer<ZConnection> client;
        sock.accept(client);

        LOG("accept " << client->peer().debugStr());

        ZSocket::socket_error err;
        ZBinary data(TCP_SIZE);
        do {
            err = client->read(data);
        } while(err == ZSocket::DONE);
        if(err != ZSocket::OK){
            ELOG(ZSocket::errorStr(err));
            TASSERT(false);
        }
        ZString recv = ZString(data.printable().asChar());
        LOG("read (" << data.size() << "): \"" << recv << "\"");

        if(recv == TCP_DAT1){
            ZString str = TCP_DAT2;
            ZBinary snddata(str);
            err = client->write(snddata);
            if(err != ZSocket::OK){
                ELOG(ZSocket::errorStr(err));
                TASSERT(false);
            }
            LOG("write (" << snddata.size() << "): \"" << ZString(snddata.printable().asChar()) << "\"");
            ok = true;
            break;
        }
    }
    TASSERT(ok);
}

void multiplex_tcp_server(){
    ZPointer<ZStreamSocket> lsocket = new ZStreamSocket;
    ZMultiplex events;
//    events.add(lsocket);
    while(events.wait()){
        for(zsize i = 0; i < events.count(); ++i){

        }
    }
}

ZArray<Test> socket_tests(){
    return {
        { "udp-client",             udp_client,             false, {} },
        { "udp-server",             udp_server,             false, {} },
        { "tcp-client",             tcp_client,             false, {} },
        { "tcp-server",             tcp_server,             false, {} },
        { "multiplex-tcp-server",   multiplex_tcp_server,   false, {} },
    };
}

}
