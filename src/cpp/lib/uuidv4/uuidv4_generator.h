#ifndef UUIDV4_GENERATOR_H
#define UUIDV4_GENERATOR_H

#include <mutex>
#include <random>
#include <string>

#include "../id_generator.h"

class UuidV4Generator : public IdGenerator {
 private:
  std::random_device rd;
  std::mt19937_64 gen;
  std::uniform_int_distribution<uint64_t> dis;
  std::mutex mtx;

 public:
  UuidV4Generator();
  std::string next_id_string() override;
};

#endif  // UUIDV4_GENERATOR_H
