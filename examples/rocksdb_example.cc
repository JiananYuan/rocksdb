#include <cassert>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <algorithm>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::PinnableSlice;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;
using namespace std;
using namespace std::chrono;
const uint64_t M = 1000000;
const uint32_t key_size = 16;
const uint32_t value_size = 64;
const std::string DB_NAME = "testdb_rocksdb";

string generate_key(const string& key) {
  string result = string(key_size - key.length(), '0') + key;
  return std::move(result);
}

string generate_value(uint64_t value) {
  string value_string = to_string(value);
  string result = string(value_size - value_string.length(), '0') + value_string;
  return std::move(result);
}

int main() {
  /**
   * REMOVE OLD DATABASE (switch -A)
   */
  system(("rm -rf " + DB_NAME).data());

  /**
   * IMPORTANT CONFIG, DO NOT MODIFY
   */
  DB* db;

  Options options;
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  options.create_if_missing = true;

  /**
   * TEST OPEN()
   */
  const uint64_t kNum = 100 * M;
  Status status = DB::Open(options, DB_NAME, &db);

  /**
   * READ DATA
   */
  std::ifstream in("/media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv");
  if (in.fail()) {
    in.close();
    in.open("/home/jiananyuan/Desktop/lsim_dataset/SOSD/books_200M_uint32.csv");
    if (in.fail()) {
      std::cout << "dataset not found" << std::endl;
      return 0;
    }
    std::cout << "use 506 server dataset" << std::endl;
  } else {
    std::cout << "use 506 computer dataset" << std::endl;
  }
  std::vector<std::string> keys;
  keys.resize(kNum);
  for (uint64_t i = 0; i < kNum; i += 1) {
    in >> keys[i];
  }
  in.close();
//  auto rd = std::random_device {};
//  auto rng = std::default_random_engine { rd() };
//  std::shuffle(std::begin(keys), std::end(keys), rng);
  std::random_shuffle(keys.begin(), keys.end());

  /**
   * TEST PUT()
   */
  for (int i = 0; i < kNum; i += 1) {
    printf("%d\r", i);
    string key = generate_key(keys[i]);
    string val = generate_value(strtol(keys[i].c_str(), nullptr, 10));
    status = db->Put(WriteOptions(), key, val);
    assert(status.ok() && "Put failed");
  }

  std::random_shuffle(keys.begin(), keys.end());
  uint32_t count = 0;
  /**
   * TEST GET()
   */
  count = 0;
  double search_time = 0;
  ofstream log_file;
  for (uint64_t i = 0; i < 0.2 * kNum; i += 1) {
    printf("%ld\r", i);
    string key = generate_key(keys[i]);
    string val;
    auto st_time = system_clock::now();
    status = db->Get(ReadOptions(), key, &val);
    auto en_time = system_clock::now();
    search_time += duration<double, std::nano>(en_time - st_time).count();
    log_file << "Get " << key << " " << val << endl;
    count ++;
  }
  cout << count << endl;
  std::cout << "average point search time : " << search_time / count << " ns" << std::endl;
  std::cout << "average point search throughput : " << count / (search_time / 1e9) << " ops" << std::endl;

  delete db;
  log_file.close();
  return 0;
}
