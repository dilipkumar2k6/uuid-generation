#include "dual_buffer.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

using namespace std;

DualBufferGenerator::DualBufferGenerator() : current_pos(0), is_running(true), fetch_needed(false) {
    conn = mysql_init(NULL);
    if (conn == NULL) {
        throw runtime_error("mysql_init() failed");
    }
    connect();
    
    // Fetch the initial segment synchronously
    if (!fetch_segment(0)) {
        throw runtime_error("Failed to fetch initial ID segment from database");
    }
    
    // Start the background fetcher thread
    fetch_thread = thread(&DualBufferGenerator::background_fetcher, this);
}

DualBufferGenerator::~DualBufferGenerator() {
    is_running = false;
    cv_fetch.notify_one();
    if (fetch_thread.joinable()) {
        fetch_thread.join();
    }
    if (conn) {
        mysql_close(conn);
    }
}

void DualBufferGenerator::connect() {
    const char* host = getenv("DB_HOST") ? getenv("DB_HOST") : "mysql-dual-buffer";
    const char* user = getenv("DB_USER") ? getenv("DB_USER") : "root";
    const char* pass = getenv("DB_PASS") ? getenv("DB_PASS") : "root";
    const char* dbname = getenv("DB_NAME") ? getenv("DB_NAME") : "uuid_db";
    int port = getenv("DB_PORT") ? atoi(getenv("DB_PORT")) : 3306;

    if (mysql_real_connect(conn, host, user, pass, dbname, port, NULL, 0) == NULL) {
        cerr << "mysql_real_connect() failed: " << mysql_error(conn) << endl;
    }
}

bool DualBufferGenerator::fetch_segment(int index) {
    lock_guard<mutex> lock(db_mtx);
    
    // Simple reconnect logic if connection dropped
    if (mysql_ping(conn)) {
        mysql_close(conn);
        conn = mysql_init(NULL);
        connect();
    }
    
    mysql_query(conn, "START TRANSACTION");
    
    if (mysql_query(conn, "UPDATE id_segments SET max_id = max_id + step WHERE biz_tag = 'default'")) {
        cerr << "UPDATE failed: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    
    if (mysql_query(conn, "SELECT max_id, step FROM id_segments WHERE biz_tag = 'default'")) {
        cerr << "SELECT failed: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    bool success = false;
    uint64_t max_id = 0;
    uint64_t step = 0;
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row) {
            max_id = stoull(row[0]);
            step = stoull(row[1]);
            success = true;
        }
        mysql_free_result(result);
    }
    
    mysql_query(conn, "COMMIT");
    
    if (success) {
        lock_guard<mutex> mtx_lock(mtx);
        segments[index].max_id = max_id;
        segments[index].current_id = max_id - step + 1;
        segments[index].step = step;
        segments[index].is_ready = true;
    }
    
    return success;
}

void DualBufferGenerator::background_fetcher() {
    while (is_running) {
        unique_lock<mutex> lock(mtx);
        // Wait until a fetch is needed or we are shutting down
        cv_fetch.wait(lock, [this] { return fetch_needed.load() || !is_running.load(); });
        
        if (!is_running) break;
        
        int next_pos = 1 - current_pos;
        lock.unlock(); // Unlock while fetching from DB (slow operation)
        
        bool success = fetch_segment(next_pos);
        
        lock.lock();
        if (success) {
            fetch_needed = false;
            cv_consume.notify_all(); // Notify consumers that the next segment is ready
        } else {
            // If fetch failed, sleep briefly and retry (in a real system, add backoff)
            lock.unlock();
            this_thread::sleep_for(chrono::milliseconds(100));
            lock.lock();
            // fetch_needed remains true, so it will retry on next loop iteration
        }
    }
}

uint64_t DualBufferGenerator::next_id() {
    unique_lock<mutex> lock(mtx);
    
    while (true) {
        Segment& current_seg = segments[current_pos];
        
        if (current_seg.current_id <= current_seg.max_id) {
            uint64_t id = current_seg.current_id++;
            
            // Calculate remaining IDs in the current segment
            uint64_t remaining = current_seg.max_id - current_seg.current_id + 1;
            
            // If less than 20% remaining, trigger background fetch for the next segment
            uint64_t threshold = current_seg.step * 0.2;
            if (remaining <= threshold && !segments[1 - current_pos].is_ready && !fetch_needed) {
                fetch_needed = true;
                cv_fetch.notify_one();
            }
            
            return id;
        } else {
            // Current segment exhausted, try to swap to the next one
            int next_pos = 1 - current_pos;
            if (segments[next_pos].is_ready) {
                current_seg.is_ready = false;
                current_pos = next_pos;
            } else {
                // Next segment is not ready yet! This means the background thread is too slow or failed.
                // We must wait for it to finish fetching.
                if (!fetch_needed) {
                    fetch_needed = true;
                    cv_fetch.notify_one();
                }
                cv_consume.wait(lock, [this, next_pos] { return segments[next_pos].is_ready; });
            }
        }
    }
}
