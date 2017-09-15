#include "message_db.h"

ChatDb &ChatDb::instance() {
    static ChatDb db;
    return db;
}

bool ChatDb::load(const std::string &address, std::vector<Message> &messages) {
    messages.clear();

    {
        LOCK(criticalSection_);
//        if (!Read(address, messages)) {
//            return false;
//        }
    }
    // TODO crypto
    return true;
}

bool ChatDb::save(const std::string &address, const std::vector<Message> &messages) {
    // TODO crypto

    LOCK(criticalSection_);
//    return Write(address, messages);
}

bool ChatDb::erase(const std::string &address) {
    LOCK(criticalSection_);
//    return Erase(address);
}

std::string ChatDb::undelivered_("undelivered");

bool ChatDb::loadUndelivered(UndeliveredMap &messages) {
    messages.clear();
    {
        LOCK(criticalSection_);
/*
        if (!Read(undelivered_, messages))
        {
            return false;
        }
*/
    }
    return true;
}

bool ChatDb::saveUndelivered(const UndeliveredMap &messages) {
    LOCK(criticalSection_);
//    return Write(undelivered_, messages);

    return false;
}

bool ChatDb::loadAddresses(std::vector<std::string> &addresses) {
    addresses.clear();

    auto cur = GetCursor();
    if (!cur) {
        return false;
    }

    bool success = true;
    while (success) {
        CDataStream key(SER_DISK, CLIENT_VERSION);

        CDataStream value(SER_DISK, CLIENT_VERSION);

        int ret = ReadAtCursor(cur, key, value, DB_NEXT);

        if (ret == DB_NOTFOUND) {
            break;
        } else if (ret != 0) {
            success = false;
            break;
        }

        std::string addr;
        key >> addr;

        if (addr == undelivered_) {
            continue;
        }

        addresses.push_back(addr);
    }

    cur->close();

    return success;


}
