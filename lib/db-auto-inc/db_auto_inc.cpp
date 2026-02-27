#include "db_auto_inc.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

using namespace std;

DbAutoIncGenerator::DbAutoIncGenerator() {
    conn = mysql_init(NULL);
    if (conn == NULL) {
        throw runtime_error("mysql_init() failed");
    }
    connect();
}

DbAutoIncGenerator::~DbAutoIncGenerator() {
    if (conn) {
        mysql_close(conn);
    }
}

void DbAutoIncGenerator::connect() {
    // Read connection details from environment variables or use defaults
    // In Kubernetes, the ProxySQL service is named 'proxysql'
    const char* host = getenv("DB_HOST") ? getenv("DB_HOST") : "proxysql";
    const char* user = getenv("DB_USER") ? getenv("DB_USER") : "root";
    const char* pass = getenv("DB_PASS") ? getenv("DB_PASS") : "root";
    const char* dbname = getenv("DB_NAME") ? getenv("DB_NAME") : "uuid_db";
    int port = getenv("DB_PORT") ? atoi(getenv("DB_PORT")) : 6033; // ProxySQL default port

    // Connect to ProxySQL (or directly to MySQL)
    unsigned int ssl_mode = SSL_MODE_DISABLED;
    mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode);
    if (mysql_real_connect(conn, host, user, pass, dbname, port, NULL, 0) == NULL) {
        cerr << "mysql_real_connect() failed: " << mysql_error(conn) << endl;
        // We don't throw here to allow retries during next_id()
    } else {
        // Ensure the tickets table exists
        const char* query = "CREATE TABLE IF NOT EXISTS tickets (id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, stub CHAR(1) NOT NULL UNIQUE) ENGINE=InnoDB";
        if (mysql_query(conn, query)) {
            cerr << "Failed to create table: " << mysql_error(conn) << endl;
        }
    }
}

uint64_t DbAutoIncGenerator::next_id() {
    // Lock to ensure thread-safe access to the single MySQL connection
    lock_guard<mutex> lock(mtx);
    
    // The REPLACE INTO statement updates the single row with stub='a',
    // forcing the AUTO_INCREMENT counter to increase.
    const char* query = "REPLACE INTO tickets (stub) VALUES ('a')";
    
    if (mysql_query(conn, query)) {
        cerr << "REPLACE INTO failed: " << mysql_error(conn) << endl;
        
        // Attempt to reconnect and retry once
        mysql_close(conn);
        conn = mysql_init(NULL);
        connect();
        
        if (mysql_query(conn, query)) {
            cerr << "Retry failed: " << mysql_error(conn) << endl;
            return 0; // Return 0 on failure
        }
    }
    
    // Retrieve the newly generated ID
    return mysql_insert_id(conn);
}
