#ifndef MESSAGECRYPTER_H
#define MESSAGECRYPTER_H

#include "message.h"
#include "../uint256.h"
#include "../sync.h"
#include "../key.h"

#include <vector>
#include <string>
#include <set>

#include <boost/thread/recursive_mutex.hpp>

class MessageCrypter {
private:
    unsigned char chKey[32] = {'0'};
    unsigned char chIV[16] = {'0'};
    bool fKeySet;
public:

    MessageCrypter() {
        // Try to keep the key data out of swap (and be a bit over-careful to keep the IV that we don't even use out of swap)
        // Note that this does nothing about suspend-to-disk (which will put all our key data on disk)
        // Note as well that at no point in this program is any attempt made to prevent stealing of keys by reading the memory of the running process.
        LockedPageManager::Instance().LockRange(&chKey[0], sizeof chKey);
        LockedPageManager::Instance().LockRange(&chIV[0], sizeof chIV);
        fKeySet = false;
    }

    ~MessageCrypter() {
        // clean key
        memset(&chKey, 0, sizeof chKey);
        memset(&chIV, 0, sizeof chIV);
        fKeySet = false;

        LockedPageManager::Instance().UnlockRange(&chKey[0], sizeof chKey);
        LockedPageManager::Instance().UnlockRange(&chIV[0], sizeof chIV);
    }

    bool SetKey(const std::vector<unsigned char> &vchNewKey, unsigned char *chNewIV);

    bool Encrypt(unsigned char *chPlaintext, uint32_t nPlain, std::vector<unsigned char> &vchCiphertext);

    bool Decrypt(unsigned char *chCiphertext, uint32_t nCipher, std::vector<unsigned char> &vchPlaintext);
};

#endif // MESSAGECRYPTER_H