#ifndef MESSAGE_DB_H
#define MESSAGE_DB_H

#include "../db.h"
#include "message.h"

#include <string>
#include <vector>
#include <map>

using UndeliveredMap = std::map<uint256, Message>;

class ChatDb : public CDB {
protected:
    ChatDb() : CDB("chat.dat", "rw") {}

public:
    static ChatDb &instance();

public:
    bool load(const std::string &address, std::vector<Message> &messages);

    bool save(const std::string &address, const std::vector<Message> &messages);

    bool erase(const std::string &address);

    bool loadUndelivered(UndeliveredMap &messages);

    bool saveUndelivered(const UndeliveredMap &messages);

    bool loadAddresses(std::vector<std::string> &addresses);

private:
    CCriticalSection criticalSection_;

    static std::string undelivered_;

};

#endif // MESSAGE_DB_H