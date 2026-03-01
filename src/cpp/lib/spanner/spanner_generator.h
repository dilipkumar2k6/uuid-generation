#ifndef SPANNER_GENERATOR_H
#define SPANNER_GENERATOR_H

#include <cstdint>
#include <mutex>
#include <string>

#include "../id_generator.h"

class SpannerGenerator : public IdGenerator {
 private:
  std::string spanner_endpoint;
  std::string project_id;
  std::string instance_id;
  std::string database_id;
  std::string session_name;
  std::mutex mtx;

  std::string http_post(const std::string& url, const std::string& data);
  void create_session();

 public:
  SpannerGenerator();
  ~SpannerGenerator();
  uint64_t next_id() override;
};

#endif  // SPANNER_GENERATOR_H
