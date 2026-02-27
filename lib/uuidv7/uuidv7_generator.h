#ifndef UUIDV7_GENERATOR_H
#define UUIDV7_GENERATOR_H

#include <string>
#include <random>
#include <mutex>
#include "../id_generator.h"

class UuidV7Generator : public IdGenerator {
private:
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<uint64_t> dis;
    std::mutex mtx;

    uint64_t current_time_millis();

public:
    UuidV7Generator();
    std::string next_id_string() override;
};

#endif // UUIDV7_GENERATOR_H
