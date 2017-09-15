#ifndef STORED_PUB_KEYS_DB_H
#define STORED_PUB_KEYS_DB_H


#include "../db.h"
#include "../pubkey.h"

class StoredPubKeysDb : public CDB {
protected:
    StoredPubKeysDb() : CDB("pubkeys.dat", "rw") {}

public:
    static StoredPubKeysDb &instance();

public:
    bool load(const std::string &address, CPubKey &key);

    bool store(const std::string &address, const CPubKey &key);

private:
    CCriticalSection criticalSection_;
};

#endif // STORED_PUB_KEYS_DB_H