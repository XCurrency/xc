#include "stored_pub_keys_db.h"

// static
StoredPubKeysDb &StoredPubKeysDb::instance() {
    static StoredPubKeysDb db;
    return db;
}

bool StoredPubKeysDb::load(const std::string &address, CPubKey &key) {

    LOCK(criticalSection_);
    return Read(address, key);
}

bool StoredPubKeysDb::store(const std::string &address, const CPubKey &key) {

    LOCK(criticalSection_);
    return Write(address, key);
}
