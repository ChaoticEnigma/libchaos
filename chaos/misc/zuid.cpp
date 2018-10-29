/*******************************************************************************
**                                  LibChaos                                  **
**                                  zuid.cpp                                  **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zuid.h"
#include "zlog.h"
#include "zrandom.h"
#include "zmutex.h"
#include "zlock.h"

#include <time.h>

#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_CYGWIN
    #define ZUID_WINAPI
#endif

#ifdef ZUID_WINAPI
    #include <winsock2.h>
    #include <windows.h>
    #include <iptypes.h>
    #include <iphlpapi.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_MACOSX || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_FREEBSD
        #include <net/if_dl.h>
    #else
        #include <linux/if_packet.h>
        #include <net/ethernet.h>
    #endif
#endif

namespace LibChaos {

//! Prevent more than one uuid from being generated at one time.
ZMutex zuidlock;
//! Make sure previous timestamp it
zu64 prevtime;
zu16 prevclock = 0;
ZBinary prevmac;

//! Cache lock.
ZMutex cachelock;
//! Cached MAC address.
ZBinary cachemac;

ZUID::ZUID(){
    // Nil UUID
    for(zu8 i = 0; i < ZUID_SIZE; ++i){
        _id_octets[i] = 0;
    }
}

/*! Generates an RFC 4122 compliant UUID of \a type.
 *  If type is not a supported type id, ZUID will be initialized to nil uuid.
 */
ZUID::ZUID(uuidtype type, ZUID namespce, ZString name){
    switch(type){
        case UNINIT:
            // Uninitialized, for internal use.
            return;

        case TIME: {
            // Version 1 UUID: Time-Clock-MAC
            zuidlock.lock();

            // Get the time
            zu64 utctime = getTimestamp();
            zu32 utchi = (utctime >> 32);
            zu32 utclo = (utctime & 0xFFFFFFFF);

            // Write time
            _id_octets[0] = (zu8)((utclo >> 24) & 0xFF); // time_lo
            _id_octets[1] = (zu8)((utclo >> 16) & 0xFF);
            _id_octets[2] = (zu8)((utclo >> 8)  & 0xFF);
            _id_octets[3] = (zu8) (utclo        & 0xFF);
            _id_octets[4] = (zu8)((utchi >> 8)  & 0xFF); // time_med
            _id_octets[5] = (zu8) (utchi        & 0xFF);
            _id_octets[6] = (zu8)((utchi >> 24) & 0xFF); // time_hi
            _id_octets[7] = (zu8)((utchi >> 16) & 0xFF);

            zu16 clock;
            // Get MAC address, allow caching
            ZBinary mac = getMACAddress(true);

            // Check previous clock value.
            if(prevclock == 0){
                // Initialize clock with random value.
                ZRandom randgen;
                ZBinary randomdat = randgen.generate(2);
                clock = (randomdat[0] << 8) | randomdat[1];
            } else {
                // Increment clock if time is time has gone backwards or mac has changed.
                clock = prevclock;
                if(prevtime >= utctime || prevmac != mac){
                    ++clock;
                }
            }

            // Write clock
            _id_octets[8] = (zu8)((clock >> 8)  & 0xFF);
            _id_octets[9] = (zu8) (clock        & 0xFF);

            // Write MAC
            mac.read(_id_octets + 10, 6);

            // Save these values
            prevtime = utctime;
            prevclock = clock;
            prevmac = mac;

            zuidlock.unlock();
            break;
        }

        case RANDOM: {
            // Version 4 UUID: Random
            ZRandom randgen;
            ZBinary random = randgen.generate(ZUID_SIZE);
            random.read(_id_octets, ZUID_SIZE);
            break;
        }

#ifdef ZHASH_HAS_MD5
        case NAME_MD5: {
            // Version 3 UUID: Namespace-Name-MD5
            nameHashSet(ZHash<ZBinary, ZHashBase::MD5>(nameHashData(namespce, name)).hash());
            break;
        }
#endif

#ifdef ZHASH_HAS_SHA1
        case NAME_SHA: {
            // Version 5 UUID: Namespace-Name-SHA
            nameHashSet(ZHash<ZBinary, ZHashBase::SHA1>(nameHashData(namespce, name)).hash());
            break;
        }
#endif

        case NIL:
        default:
            // Nil UUID
            for(zu8 i = 0; i < ZUID_SIZE; ++i){
                _id_octets[i] = 0;
            }
            return;
    }

    // Write version
    _id_octets[6] &= 0x0F;
    _id_octets[6] |= ((int)type << 4);  // Insert UUID version

    // Write variant
    _id_octets[8] &= 0x3F;
    _id_octets[8] |= 0x80;  // Insert UUID variant "10x"
}

ZUID::ZUID(ZString str){
    str.replace(" ", "");
    str.replace("-", "");
    str.replace(":", "");
    if(str.size() == 32){
        for(zu64 i = 0; i < 32; ++i){
            if(!ZString::charIsHexadecimal(str[i])){
                for(zu8 j = 0; j < ZUID_SIZE; ++j)
                    _id_octets[j] = 0;
                return;
            }
        }
        for(zu64 i = 0; i < ZUID_SIZE; ++i)
            _id_octets[i] = (zu8)ZString::substr(str, i*2, 2).toUint(16);
    } else {
        for(zu8 i = 0; i < ZUID_SIZE; ++i)
            _id_octets[i] = 0;
    }
}

int ZUID::compare(const ZUID &uid){
    return ::memcmp(_id_octets, uid._id_octets, ZUID_SIZE);
}

bool ZUID::operator==(const ZUID &uid){
    return compare(uid) == 0;
}

bool ZUID::operator<(const ZUID &uid){
    return compare(uid) < 0;
}

ZUID &ZUID::fromRaw(zbyte *bytes){
    ::memcpy(_id_octets, bytes, ZUID_SIZE);
    return *this;
}

ZUID::uuidtype ZUID::getType() const {
    zu8 type = _id_octets[6] & 0xF0 >> 4;
    switch(type){
        case 1:
            return TIME;
#ifdef LIBCHAOS_HAS_CRYPTO
        case 3:
            return NAME_MD5;
        case 5:
            return NAME_SHA;
#endif
        case 4:
            return RANDOM;
        default:
            return UNKNOWN;
    }
}

ZString ZUID::str(ZString delim) const {
    ZString uid;
    for(zu8 i = 0; i < ZUID_SIZE; ++i)
        uid += ZString::ItoS(_id_octets[i], 16, 2);
    if(!delim.isEmpty()){
        uid.insert(8, delim);
        uid.insert(8 + 1 + 4, delim);
        uid.insert(8 + 1 + 4 + 1 + 4, delim);
        uid.insert(8 + 1 + 4 + 1 + 4 + 1 + 4, delim);
    }
    return uid;
}

ZBinary ZUID::bin() const {
    return ZBinary(_id_octets, ZUID_SIZE);
}

zu64 ZUID::getTimestamp(){
#ifdef ZUID_WINAPI
    SYSTEMTIME systime;
    GetSystemTime(&systime);
    FILETIME filetime;
    SystemTimeToFileTime(&systime, &filetime);
    // Time in 100-nanosecond intervals
    zu64 mstime = ((zu64)filetime.dwHighDateTime << 32) | filetime.dwLowDateTime;
    // Microsoft UTC starts January 1, 1601, 00:00:00.0000000
    // We need UTC since October 15, 1582, 00:00:00.0000000
    // Add 18 years + 17 days in Oct + 30 days in Nov + 31 days in Dec + 5 leap days, to seconds, to 100 nanosecond interval
    zu64 utctime = mstime + ((zu64)((18*365)+17+30+31+5) * (60*60*24) * (1000*1000*10));
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    // Time in microseconds
    zu64 tvtime = (zu64)((tv.tv_sec * 1000 * 1000) + tv.tv_usec);
    // POSIX UTC start January 1, 1970, 00:00:00.000000
    // We need UTC since October 15, 1582, 00:00:00.0000000
    // Add 387 years + 17 days in Oct + 30 days in Nov + 31 days in Dec + 94 leap days, to seconds, to 100 nanosecond interval
    zu64 utctime = (tvtime * 10) + ((zu64)((387*365)+17+30+31+5) * (60*60*24) * (1000*1000*10));
#endif
    return utctime;
}

ZList<ZBinary> ZUID::getMACAddresses(){
    ZList<ZBinary> maclist;

#ifdef ZUID_WINAPI

    // Win32 API

    ULONG bufLen = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO *adapterInfo = (IP_ADAPTER_INFO *)new zu8[bufLen];

    // Get number of adapters and list of adapter info
    DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
    if(status == ERROR_BUFFER_OVERFLOW){
        // Make larger list for adapters
        delete[] adapterInfo;
        adapterInfo = (IP_ADAPTER_INFO *)new zu8[bufLen];
        status = GetAdaptersInfo(adapterInfo, &bufLen);
    }

    if(status == NO_ERROR){
        // Get first acceptable MAC from list
        IP_ADAPTER_INFO *adapterInfoList = adapterInfo;
        while(adapterInfoList != NULL){
            if(validMAC(adapterInfoList->Address)){
                maclist.push(ZBinary(adapterInfoList->Address, 6));
            }
            adapterInfoList = adapterInfoList->Next;
        }
    }
    delete[] adapterInfo;

#else

    // getifaddrs

    ifaddrs *iflist = NULL;
    // Get list of interfaces and addresses
    int r = getifaddrs(&iflist);
    if(r == 0 && iflist != NULL){
        ifaddrs *current = iflist;
        // Walk linked list
        while(current != NULL){
            // Look for an interface with a hardware address
#if LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_FREEBSD || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_MACOSX
            if((current->ifa_addr != NULL) && (current->ifa_addr->sa_family == AF_LINK)){
                struct sockaddr_dl *sockdl = (struct sockaddr_dl *)current->ifa_addr;
                const uint8_t *mac = reinterpret_cast<const uint8_t*>(LLADDR(sockdl));
#else
            if((current->ifa_addr != NULL) && (current->ifa_addr->sa_family == AF_PACKET)){
                struct sockaddr_ll *sockll = (struct sockaddr_ll *)current->ifa_addr;
                const uint8_t *mac = sockll->sll_addr;
#endif
                if(validMAC(mac)){
                    maclist.push(ZBinary(mac, 6));
                }
            }
            current = current->ifa_next;
        }
    }
    freeifaddrs(iflist);

#endif

    return maclist;
}

ZBinary ZUID::getMACAddress(bool cache){
    // Get cached MAC address
    if(cache){
        ZLock lock(cachelock);
        if(cachemac.size())
            return cachemac;
    }

#ifdef ZUID_WINAPI

    // Win32 API

    PIP_ADAPTER_INFO adapterInfo;
    ULONG bufLen = sizeof(IP_ADAPTER_INFO);
    adapterInfo = new IP_ADAPTER_INFO[1];

    // Get number of adapters and list of adapter info
    DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
    if(status == ERROR_BUFFER_OVERFLOW){
        // Make larger list for adapters
        delete[] adapterInfo;
        adapterInfo = new IP_ADAPTER_INFO[bufLen];
        status = GetAdaptersInfo(adapterInfo, &bufLen);
    }

    if(status == NO_ERROR){
        // Get first acceptable MAC from list
        PIP_ADAPTER_INFO adapterInfoList = adapterInfo;
        while(adapterInfoList != NULL){
            if(validMAC(adapterInfoList->Address)){
                ZBinary bin(adapterInfoList->Address, 6);
                delete[] adapterInfo;
                cachelock.lock();
                cachemac = bin;
                cachelock.unlock();
                return bin;
            }
            adapterInfoList = adapterInfoList->Next;
        }
    }
    delete[] adapterInfo;

#elif LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_MACOSX || LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_FREEBSD

    // getifaddrs

    ifaddrs *iflist = NULL;
    // Get list of interfaces and addresses
    int r = getifaddrs(&iflist);
    if(r == 0 && iflist != NULL){
        ifaddrs *current = iflist;
        // Walk linked list
        while(current != NULL){
            // Look for an interface with a hardware address
            if((current->ifa_addr != NULL) && (current->ifa_addr->sa_family == AF_LINK)){
                sockaddr_dl *sockdl = (sockaddr_dl *)current->ifa_addr;
                uint8_t *mac = reinterpret_cast<uint8_t*>(LLADDR(sockdl));
                // Get first valid MAC
                if(validMAC(mac)){
                    ZBinary bin(mac, 6);
                    cachelock.lock();
                    cachemac = bin;
                    cachelock.unlock();
                    return bin;
                }
            }
            current = current->ifa_next;
        }
    }

#else

    // POSIX socket/ioctl

    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    unsigned char mac_address[6];

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sock != -1){
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if(ioctl(sock, SIOCGIFCONF, &ifc) != -1){
            struct ifreq *it = ifc.ifc_req;
            const struct ifreq *end = it + ((unsigned long)ifc.ifc_len / sizeof(struct ifreq));
            for(; it != end; ++it){
                strcpy(ifr.ifr_name, it->ifr_name);
                DLOG(ifr.ifr_name);
                // Get interface flags
                if(ioctl(sock, SIOCGIFFLAGS, &ifr) == 0){
                    // Skip loopback interface
                    if(!(ifr.ifr_flags & IFF_LOOPBACK)){
                        // Get hardware address
                        if(ioctl(sock, SIOCGIFHWADDR, &ifr) == 0){
                            memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
                            if(validMAC(mac_address)){
                                ZBinary bin(mac_address, 6);
                                cachelock.lock();
                                cachemac = bin;
                                cachelock.unlock();
                                return bin;
                            } else {
                                DLOG("in2valid mac");
                            }
                        } else {
                            DLOG("failed to get mac");
                        }
                    } else {
                        DLOG("skip loopback");
                    }
                } else {
                    // Try next interface
                    DLOG("failed to get if flags");
                }
            }
        } else {
            // ioctl failed
            DLOG("failed to get if config");
        }
    } else {
        // socket failed
        DLOG("failed to open socket");
    }

#endif

    // Otherwise, generate random 6 bytes
    ZRandom rand;
    ZBinary addr = rand.generate(6);
    addr[0] |= 0x02; // Mark as locally administered to avoid collision with real IEEE 802 MAC Addresses
    cachelock.lock();
    cachemac = addr;
    cachelock.unlock();
    return addr;
}

bool ZUID::validMAC(const zoctet *addr){
    if(addr == NULL) return false;
    // Zero address
    if(addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0 && addr[4] == 0 && addr[5] == 0) return false;
    // Locally administered address
    if(addr[0] & 0x02) return false;
    return true;
}

ZBinary ZUID::nameHashData(ZUID namesp, ZString name){
    ZBinary ns = namesp.bin();
    ZBinary data;
    data.writebeu32(ns.readleu32());
    data.writebeu16(ns.readleu16());
    data.writebeu16(ns.readleu16());
    data.write(ns.getSub(8, 8));
    data.write(name.bytes(), name.size());
    return data;
}

void ZUID::nameHashSet(ZBinary hash){
    _id_octets[0] = hash[3];    // time-low
    _id_octets[1] = hash[2];
    _id_octets[2] = hash[1];
    _id_octets[3] = hash[0];

    _id_octets[4] = hash[5];    // time-mid
    _id_octets[5] = hash[4];

    _id_octets[6] = hash[7];    // time-high-and-version
    _id_octets[7] = hash[6];

    _id_octets[8]  = hash[8];
    _id_octets[9]  = hash[9];

    _id_octets[10] = hash[10];  // node
    _id_octets[11] = hash[11];
    _id_octets[12] = hash[12];
    _id_octets[13] = hash[13];
    _id_octets[14] = hash[14];
    _id_octets[15] = hash[15];
}

}
