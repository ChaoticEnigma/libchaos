/*******************************************************************************
**                                  LibChaos                                  **
**                                 zrandom.cpp                                **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zrandom.h"
#include "zstring.h"
#include "zerror.h"
#include "zlog.h"

#include <assert.h>

#if PLATFORM == WINDOWS
    #include <windows.h>
    #define RAND_DEV "ZRANDOM_GENERATOR"
#elif PLATFORM == MACOSX
    // OSX's /dev/random is equivalent to UNIX's /dev/urandom
    #define RAND_DEV "/dev/random"
#else
    // Use /dev/urandom for reliability
    #define RAND_DEV "/dev/urandom"
#endif

namespace LibChaos {

ZRandom::ZRandom(){
#if PLATFORM == WINDOWS
    _cryptprov = new HCRYPTPROV;
    ZString keyset = RAND_DEV;
    // Try to use existing named keyset
    BOOL ret = CryptAcquireContextA((HCRYPTPROV*)_cryptprov, keyset.cc(), NULL, PROV_RSA_FULL, 0);
    if(ret == FALSE){
        if(GetLastError() == 0x80090016){
            // Create new named keyset
            ret = CryptAcquireContextA((HCRYPTPROV*)&_cryptprov, keyset.cc(), NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
            if(ret == FALSE){
                ELOG(ZError::getSystemError());
            }
        } else {
            ELOG(ZError::getSystemError());
        }
    }
#else
    // Use OS filesystem random device
    _devrandom = fopen(RAND_DEV, "r");
    if(_devrandom == NULL)
        ELOG("Failed to open " RAND_DEV ":" << ZError::getSystemError());
#endif
}

ZRandom::~ZRandom(){
#if PLATFORM == WINDOWS
    delete (HCRYPTPROV*)_cryptprov;
#else
    fclose(_devrandom);
    _devrandom = NULL;
#endif
}

#define RAND_INV_RANGE(r) ((zu64) (ZU64_MAX) / (r))

zu64 ZRandom::genzu(zu64 min, zu64 max){
    if(min > max)
        throw ZException("Unusable random range");
    const zu64 range = max - min;
    if(range == 0)
        return min;

    ZBinary buff = generate(8);

    zu64 rand;
    do {
        rand = ZBinary::deczu64(buff.raw());
    } while(rand >= range * RAND_INV_RANGE(range));
    rand /= RAND_INV_RANGE(range);

    // TODO: Imperfect random distribution
//    rand = rand % (max - min) + min;
    return rand;
}

ZBinary ZRandom::generate(zu64 size){
    ZBuffer buffer(size);
#if PLATFORM == WINDOWS
    BOOL ret = CryptGenRandom(*(HCRYPTPROV*)_cryptprov, size, buffer.raw());
    if(ret == FALSE)
        ELOG(ZError::getSystemError());
#else
    if(_devrandom == NULL)
        throw ZException("Random device is not open");
    else
        fread(buffer.raw(), 1, size, _devrandom);
#endif
    return buffer;
}

bool ZRandom::chance(double probability){
    probability = MAX(probability, 0.0f);
    probability = MIN(probability, 1.0f);
    if(probability == 0.0f)
        return false;
    zu64 rand = genzu();
    return ((double)rand / (double)ZU64_MAX) <= probability;
}

}
