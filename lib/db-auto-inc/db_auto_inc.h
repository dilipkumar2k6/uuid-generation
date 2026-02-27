#ifndef DB_AUTO_INC_H
#define DB_AUTO_INC_H

#include <string>
#include <mutex>
#include <mysql/mysql.h>
#include "../id_generator.h"

/**
 * Database Auto-Increment ID Generator
 * 
 * Uses a Multi-Master MySQL setup (Flickr Ticket Server pattern)
 * to generate unique 64-bit IDs using the AUTO_INCREMENT feature.
 */
class DbAutoIncGenerator : public IdGenerator {
private:
    MYSQL *conn;
    std::mutex mtx;

    void connect();

public:
    DbAutoIncGenerator();
    ~DbAutoIncGenerator();
    
    uint64_t next_id() override;
};

#endif // DB_AUTO_INC_H
