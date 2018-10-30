#include "tests.h"
#include "zaddress.h"
#include "zconnection.h"
#include "zstreamsocket.h"

namespace LibChaosTest {

void http_get(){
    ZStreamSocket sock;

    ZString url = "http://httpbin.org/get";
    ArZ urp = url.split("://");
    TASSERT(urp.size() == 2);
    ZString scheme = urp[0];
    ArZ pap = urp[1].split("/");
    TASSERT(pap.size() > 0);
    ZString host = pap[0];
    ZPath path;
    if(pap.size() > 1){
        path = pap[1];
    }
    path.absolute() = true;

    LOG("Scheme " << scheme);
    LOG("Host " << host);
    LOG("Path " << path);

    ZAddress addr(host, 80);

    zsocktype connfd;
    ZAddress connaddr;
    LOG("Connecting to " << addr.str());
    if(!sock.connect(addr, connfd, connaddr)){
        ELOG("Socket Connect Fail");
        TASSERT(false);
    }
    ZConnection conn(connfd, connaddr);
    LOG("connected " << conn.peer().str());

//    ZThread::sleep(1);
//    TASSERT(false);

    ZString reqstr = "GET " + path.str() + " HTTP/1.1\nHost: " + host + "\n\n";
    ZBinary request(reqstr.bytes(), reqstr.size());
    ZSocket::socket_error err = conn.write(request);
    TASSERT(err == ZSocket::OK);
    LOG("write (" << request.size() << "): \"" << request << "\"");

    ZBinary reply;
    err = ZSocket::AGAIN;
    while(err == ZSocket::AGAIN){
        err = conn.read(reply);
        LOG("read (" << reply.size() << "): \"" << reply << "\"");
    }
    LOG(ZSocket::errorStr(err));
}

ZArray<Test> http_tests(){
    return {
        { "http_get", http_get, false, {} },
    };
}

}
