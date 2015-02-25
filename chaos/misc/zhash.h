#ifndef ZHASH
#define ZHASH

#include "ztypes.h"
#include "xxhash.h"

namespace LibChaos {

class ZHashBase {
public:
    ZHashBase(zu64 hashval) : _hash(hashval){}
    zu64 hash() const { return _hash; }
protected:
    zu64 _hash;
};

template <typename T> class ZHash : public ZHashBase {
public:
    ZHash(T &data) : ZHashBase(0){}
};

// zu
template <> class ZHash<zu8> : public ZHashBase {
public:
    ZHash(zu8 data) : ZHashBase(data){}
};
template <> class ZHash<zu16> : public ZHashBase {
public:
    ZHash(zu16 data) : ZHashBase(data){}
};
template <> class ZHash<zu32> : public ZHashBase {
public:
    ZHash(zu32 data) : ZHashBase(data){}
};
template <> class ZHash<zu64> : public ZHashBase {
public:
    ZHash(zu64 data) : ZHashBase(data){}
};

// zs
template <> class ZHash<zs8> : public ZHashBase {
public:
    ZHash(zs8 data) : ZHashBase((zu64)data){}
};
template <> class ZHash<zs16> : public ZHashBase {
public:
    ZHash(zs16 data) : ZHashBase((zu64)data){}
};
template <> class ZHash<zs32> : public ZHashBase {
public:
    ZHash(zs32 data) : ZHashBase((zu64)data){}
};
template <> class ZHash<zs64> : public ZHashBase {
public:
    ZHash(zs64 data) : ZHashBase((zu64)data){}
};

// signed
template <> class ZHash<char> : public ZHashBase {
public:
    ZHash(char data) : ZHashBase((zu64)data){}
};
//template <> class ZHash<short> : public ZHashBase {
//public:
//    ZHash(short data) : ZHashBase((zu64)data){}
//};
//template <> class ZHash<int> : public ZHashBase {
//public:
//    ZHash(int data) : ZHashBase((zu64)data){}
//};
template <> class ZHash<long> : public ZHashBase {
public:
    ZHash(long data) : ZHashBase((zu64)data){}
};
//template <> class ZHash<long long> : public ZHashBase {
//public:
//    ZHash(long long data) : ZHashBase((zu64)data){}
//};

// unsigned
//template <> class ZHash<unsigned char> : public ZHashBase {
//public:
//    ZHash(unsigned char data) : ZHashBase((zu64)data){}
//};
//template <> class ZHash<unsigned short> : public ZHashBase {
//public:
//    ZHash(unsigned short data) : ZHashBase((zu64)data){}
//};
//template <> class ZHash<unsigned int> : public ZHashBase {
//public:
//    ZHash(unsigned int data) : ZHashBase((zu64)data){}
//};
template <> class ZHash<unsigned long> : public ZHashBase {
public:
    ZHash(unsigned long data) : ZHashBase((zu64)data){}
};
//template <> class ZHash<unsigned long long> : public ZHashBase {
//public:
//    ZHash(unsigned long long data) : ZHashBase((zu64)data){}
//};

}

#endif // ZHASH
