#include "messagecrypter.h"


bool MessageCrypter::SetKey(const std::vector<unsigned char> &vchNewKey, unsigned char *chNewIV) {
    // -- for EVP_aes_256_cbc() key must be 256 bit, iv must be 128 bit.

    memcpy(&chKey[0], &vchNewKey[0], sizeof(chKey));

    memcpy(&chIV, chNewIV, sizeof(chIV));

    fKeySet = true;

    return true;
}

bool MessageCrypter::Encrypt(unsigned char *chPlaintext, uint32_t nPlain, std::vector<unsigned char> &vchCiphertext) {
    return false;
}

bool MessageCrypter::Decrypt(unsigned char *chCiphertext, uint32_t nCipher, std::vector<unsigned char> &vchPlaintext) {
    return false;
}
