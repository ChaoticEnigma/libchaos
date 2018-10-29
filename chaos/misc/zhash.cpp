/*******************************************************************************
**                                  LibChaos                                  **
**                                  zhash.cpp                                 **
**                          See COPYRIGHT and LICENSE                         **
*******************************************************************************/
#include "zhash.h"
#include "zexception.h"
#include <functional>
#include "xxhash.h"
//#include "fnv.h"

#ifdef LIBCHAOS_HAS_CRYPTO
    #include <openssl/md5.h>
    #include <openssl/sha.h>
#elif LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS
    #include <windows.h>
    #include <wincrypt.h>
    #define MD5LEN  16
    #define SHA1LEN 20
#endif

typedef LibChaos::zu32 crc;

namespace LibChaos {

// //////////////////////////////////////////////////////////
// CRC-16
// //////////////////////////////////////////////////////////

// From http://mdfs.net/Info/Comp/Comms/CRC16.htm
// CRC-CCITT
#define CCITT_POLY 0x1021
zu16 ZHash16Base::crcHash16_hash(const zbyte *ptr, zu64 size, zu16 init) {
    zu32 crc = init;
    for(zu64 i = 0; i < size; ++i){                 // Step through bytes in memory
        crc ^= (zu16)(ptr[i] << 8);                 // Fetch byte from memory, XOR into CRC top byte
        for(int j = 0; j < 8; j++){                 // Prepare to rotate 8 bits
            crc = crc << 1;                         // rotate
            if(crc & 0x10000)                       // bit 15 was set (now bit 16)...
                crc = (crc ^ CCITT_POLY) & 0xFFFF;  // XOR with XMODEM polynomic and ensure CRC remains 16-bit value
        }
    }
    return (zu16)(crc & 0xFFFF);
}

// //////////////////////////////////////////////////////////
// CRC-32
// //////////////////////////////////////////////////////////

#define CRC32_POLYNOMIAL        0x04C11DB7
#define CRC32_INITIAL_REMAINDER 0xFFFFFFFF
#define CRC32_FINAL_XOR_VALUE   0xFFFFFFFF
//#define CHECK_VALUE         0xCBF43926

#define CRC32_WIDTH   32
#define CRC32_TOPBIT  ((LibChaos::zu32)1 << (CRC32_WIDTH - 1))

#define CRC32_REFLECT_DATA(X)       ((unsigned char)reflect(X, 8))
#define CRC32_REFLECT_REMAINDER(X)  ((crc)reflect(X, CRC32_WIDTH))

static zu32 reflect(zu32 data, zu8 bits){
    zu32 reflection = 0x00000000;
    // * Reflect the data about the center bit.
    for(zu8 bit = 0; bit < bits; ++bit){
        // * If the LSB bit is set, set the reflection of it.
        if(data & 0x01){
            reflection |= ((zu32)1 << (bits - bit - 1));
        }
        data = (data >> 1);
    }
    return reflection;
}

zu32 ZHash32Base::crcHash32_hash(const zbyte *data, zu64 size, zu32 remainder){
    // Convert the input remainder to the expected input
    remainder = (CRC32_REFLECT_REMAINDER(remainder) ^ CRC32_FINAL_XOR_VALUE);
    zu64 byte;
    unsigned char bit;

    // * Perform modulo-2 division, a byte at a time.
    for(byte = 0; byte < size; ++byte){
        // * Bring the next byte into the remainder.
        remainder ^= ((zu32)CRC32_REFLECT_DATA(data[byte]) << (CRC32_WIDTH - 8));
        // Perform modulo-2 division, a bit at a time.
        for(bit = 8; bit > 0; --bit){
            // * Try to divide the current data bit.
            if(remainder & CRC32_TOPBIT){
                remainder = (remainder << 1) ^ CRC32_POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }

    // * The final remainder is the CRC result.
    return (CRC32_REFLECT_REMAINDER(remainder) ^ CRC32_FINAL_XOR_VALUE);
}

// //////////////////////////////////////////////////////////
// Simple Hash
// //////////////////////////////////////////////////////////

zu64 ZHash64Base::simpleHash64_hash(const zbyte *data, zu64 size, zu64 seed){
    zu64 hash = seed;
    for(zu64 i = 0; i < size; ++i){
        hash = ((hash << 5) + hash) + data[i]; /* hash * 33 + c */
    }
    return hash;
}

// //////////////////////////////////////////////////////////
// FNV Hash
// //////////////////////////////////////////////////////////

zu64 ZHash64Base::fnvHash64_hash(const zbyte *data, zu64 size, zu64 hval){
    // FNV (Fowler/Noll/Vo) hash algorithm
    // The FNV hash algorithm is in the public domain
    // FNV-1a is recommended where possible by the developers

    // The FNV-1a hash algorithm
    const zbyte *bp = data;         /* start of buffer */
    const zbyte *be = bp + size;    /* beyond end of buffer */

    // FNV-1a hash each octet of the buffer
    while (bp < be) {
        // xor the bottom with the current octet
        hval ^= (zu64)*bp++;

        // multiply by the 64 bit FNV magic prime mod 2^64
        //hval *= FNV_64_PRIME;
        // OR
        // GGC OPTIMIZATIONs
        hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
    }
    return hval;

    //return fnv_64a_buf(data, size, hval);
}

// //////////////////////////////////////////////////////////
// xxHash
// //////////////////////////////////////////////////////////

zu64 ZHash64Base::xxHash64_hash(const zbyte *data, zu64 size){
    return XXH64(data, size, 0);
}
void *ZHash64Base::xxHash64_init(){
    return (void *)XXH64_createState();
}
void ZHash64Base::xxHash64_feed(void *state, const zbyte *data, zu64 size){
    XXH64_update((XXH64_state_t*)state, data, size);
}
zu64 ZHash64Base::xxHash64_done(void *state){
    zu64 hash = XXH64_digest((const XXH64_state_t*)state);
    XXH64_freeState((XXH64_state_t*)state);
    return hash;
}

#ifdef LIBCHAOS_HAS_CRYPTO

// //////////////////////////////////////////////////////////
// MD5
// //////////////////////////////////////////////////////////

ZBinary ZHashBigBase::md5_hash(const zbyte *data, zu64 size){
    ZBinary hash(MD5_DIGEST_LENGTH);
    ::MD5(data, size, hash.raw());
    return hash;
}

void *ZHashBigBase::md5_init(){
    MD5_CTX *context = new MD5_CTX;
    if(context)
        MD5_Init(context);
    return context;
}

void ZHashBigBase::md5_feed(void *context, const zbyte *data, zu64 size){
    if(context)
        MD5_Update((MD5_CTX *)context, data, size);
}

ZBinary ZHashBigBase::md5_finish(void *ctx){
    MD5_CTX *context = (MD5_CTX *)ctx;
    ZBinary hash(MD5_DIGEST_LENGTH);
    if(context)
        MD5_Final(hash.raw(), context);
    delete context;
    return hash;
}

// //////////////////////////////////////////////////////////
// SHA-1
// //////////////////////////////////////////////////////////

ZBinary ZHashBigBase::sha1_hash(const zbyte *data, zu64 size){
    ZBinary hash(SHA_DIGEST_LENGTH);
    ::SHA1(data, size, hash.raw());
    return hash;
}

void *ZHashBigBase::sha1_init(){
    SHA_CTX *context = new SHA_CTX;
    if(context)
        SHA1_Init(context);
    return context;
}

void ZHashBigBase::sha1_feed(void *context, const zbyte *data, zu64 size){
    if(context)
        SHA1_Update((SHA_CTX *)context, data, size);
}

ZBinary ZHashBigBase::sha1_finish(void *ctx){
    SHA_CTX *context = (SHA_CTX *)ctx;
    ZBinary hash(SHA_DIGEST_LENGTH);
    if(context)
        SHA1_Final(hash.raw(), context);
    delete context;
    return hash;
}

#elif LIBCHAOS_PLATFORM == LIBCHAOS_PLATFORM_WINDOWS

struct WinCryptHash {
    HCRYPTPROV prov;
    HCRYPTHASH hash;
};

// //////////////////////////////////////////////////////////
// MD5
// //////////////////////////////////////////////////////////

ZBinary ZHashBigBase::md5_hash(const zbyte *data, zu64 size){
    HCRYPTPROV hProv = 0;
    if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    HCRYPTHASH hHash = 0;
    if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)){
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    if(!CryptHashData(hHash, data, size, 0)){
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    DWORD mlen = MD5LEN;
    ZBinary hash(mlen);
    if(!CryptGetHashParam(hHash, HP_HASHVAL, hash.raw(), &mlen, 0)){
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return hash;
}

void *ZHashBigBase::md5_init(){
    WinCryptHash *ctx = new WinCryptHash;
    if(!CryptAcquireContext(&ctx->prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    if(!CryptCreateHash(ctx->prov, CALG_MD5, 0, 0, &ctx->hash)){
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    return ctx;
}

void ZHashBigBase::md5_feed(void *context, const zbyte *data, zu64 size){
    WinCryptHash *ctx = (WinCryptHash *)context;
    if(!CryptHashData(ctx->hash, data, size, 0)){
        CryptDestroyHash(ctx->hash);
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
}

ZBinary ZHashBigBase::md5_finish(void *context){
    WinCryptHash *ctx = (WinCryptHash *)context;
    DWORD mlen = MD5LEN;
    ZBinary hash(mlen);
    if(!CryptGetHashParam(ctx->hash, HP_HASHVAL, hash.raw(), &mlen, 0)){
        CryptDestroyHash(ctx->hash);
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    delete ctx;
    return hash;
}

// //////////////////////////////////////////////////////////
// SHA-1
// //////////////////////////////////////////////////////////

ZBinary ZHashBigBase::sha1_hash(const zbyte *data, zu64 size){
    HCRYPTPROV hProv = 0;
    if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
        throw ZException("ZHash sha1_hash: " + ZError::getSystemError(), __LINE__);
    }
    HCRYPTHASH hHash = 0;
    if(!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)){
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash sha1_hash: " + ZError::getSystemError(), __LINE__);
    }
    if(!CryptHashData(hHash, data, size, 0)){
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash sha1_hash: " + ZError::getSystemError(), __LINE__);
    }
    DWORD mlen = SHA1LEN;
    ZBinary hash(mlen);
    if(!CryptGetHashParam(hHash, HP_HASHVAL, hash.raw(), &mlen, 0)){
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        throw ZException("ZHash sha1_hash: " + ZError::getSystemError(), __LINE__);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return hash;
}

void *ZHashBigBase::sha1_init(){
    WinCryptHash *ctx = new WinCryptHash;
    if(!CryptAcquireContext(&ctx->prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    if(!CryptCreateHash(ctx->prov, CALG_SHA1, 0, 0, &ctx->hash)){
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    return ctx;
}

void ZHashBigBase::sha1_feed(void *context, const zbyte *data, zu64 size){
    WinCryptHash *ctx = (WinCryptHash *)context;
    if(!CryptHashData(ctx->hash, data, size, 0)){
        CryptDestroyHash(ctx->hash);
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
}

ZBinary ZHashBigBase::sha1_finish(void *context){
    WinCryptHash *ctx = (WinCryptHash *)context;
    DWORD mlen = SHA1LEN;
    ZBinary hash(mlen);
    if(!CryptGetHashParam(ctx->hash, HP_HASHVAL, hash.raw(), &mlen, 0)){
        CryptDestroyHash(ctx->hash);
        CryptReleaseContext(ctx->prov, 0);
        delete ctx;
        throw ZException("ZHash md5_hash: " + ZError::getSystemError(), __LINE__);
    }
    delete ctx;
    return hash;
}

#endif

}
