#include "messagecrypter.h"

#include <openssl/aes.h>
#include <openssl/evp.h>

#include <boost/date_time/posix_time/posix_time.hpp>


bool MessageCrypter::SetKey(const std::vector<unsigned char> &vchNewKey, unsigned char *chNewIV) {
    // -- for EVP_aes_256_cbc() key must be 256 bit, iv must be 128 bit.

    memcpy(&chKey[0], &vchNewKey[0], sizeof(chKey));

    memcpy(&chIV, chNewIV, sizeof(chIV));

    fKeySet = true;

    return true;
}

bool MessageCrypter::Encrypt(unsigned char *chPlaintext, uint32_t nPlain, std::vector<unsigned char> &vchCiphertext) {
    if (!fKeySet) {
        return false;
    }

    // -- max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE - 1 bytes
    int nLen = nPlain;

    int nCLen = nLen + AES_BLOCK_SIZE, nFLen = 0;
    vchCiphertext = std::vector<unsigned char>(nCLen);

    EVP_CIPHER_CTX ctx;

    bool fOk = true;

    EVP_CIPHER_CTX_init(&ctx);
    if (fOk) {
        fOk = EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), nullptr, &chKey[0], &chIV[0]);
    }
    if (fOk) {
        fOk = EVP_EncryptUpdate(&ctx, &vchCiphertext[0], &nCLen, chPlaintext, nLen);
    }
    if (fOk) {
        fOk = EVP_EncryptFinal_ex(&ctx, (&vchCiphertext[0]) + nCLen, &nFLen);
    }
    EVP_CIPHER_CTX_cleanup(&ctx);

    if (!fOk) {
        return false;
    }

    vchCiphertext.resize(nCLen + nFLen);

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool MessageCrypter::Decrypt(unsigned char *chCiphertext, uint32_t nCipher,
                             std::vector<unsigned char> &vchPlaintext) {
    if (!fKeySet) {
        return false;
    }

    // plaintext will always be equal to or lesser than length of ciphertext
    int nPLen = nCipher, nFLen = 0;

    vchPlaintext.resize(nCipher);

    EVP_CIPHER_CTX ctx;

    bool fOk = true;

    EVP_CIPHER_CTX_init(&ctx);
    if (fOk) {
        fOk = EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, &chKey[0], &chIV[0]);
    }
    if (fOk) {
        fOk = EVP_DecryptUpdate(&ctx, &vchPlaintext[0], &nPLen, &chCiphertext[0], nCipher);
    }
    if (fOk) {
        fOk = EVP_DecryptFinal_ex(&ctx, (&vchPlaintext[0]) + nPLen, &nFLen);
    }
    EVP_CIPHER_CTX_cleanup(&ctx);

    if (!fOk) {
        return false;
    }

    vchPlaintext.resize(nPLen + nFLen);

    return true;
}
