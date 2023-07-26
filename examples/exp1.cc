#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/compression_type.h"
#include "rocksdb/filter_policy.h"
#include "cxxopts.hpp"

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
uint32_t key_size, value_size;

// Zipf generator, inspired by
// https://github.com/brianfrankcooper/YCSB/blob/master/core/src/main/java/site/ycsb/generator/ScrambledZipfianGenerator.java
// https://github.com/brianfrankcooper/YCSB/blob/master/core/src/main/java/site/ycsb/generator/ZipfianGenerator.java

class ScrambledZipfianGenerator {
 public:
  static constexpr double ZETAN = 26.46902820178302;
  static constexpr double ZIPFIAN_CONSTANT = 0.99;

  int num_keys_;
  double alpha_;
  double eta_;
  std::mt19937_64 gen_;
  std::uniform_real_distribution<double> dis_;

  explicit ScrambledZipfianGenerator(int num_keys)
      : num_keys_(num_keys), gen_(std::random_device{}()), dis_(0, 1) {
    double zeta2theta = zeta(2);
    alpha_ = 1. / (1. - ZIPFIAN_CONSTANT);
    eta_ = (1 - std::pow(2. / num_keys_, 1 - ZIPFIAN_CONSTANT)) /
           (1 - zeta2theta / ZETAN);
  }

  int nextValue() {
    double u = dis_(gen_);
    double uz = u * ZETAN;

    int ret;
    if (uz < 1.0) {
      ret = 0;
    } else if (uz < 1.0 + std::pow(0.5, ZIPFIAN_CONSTANT)) {
      ret = 1;
    } else {
      ret = (int)(num_keys_ * std::pow(eta_ * u - eta_ + 1, alpha_));
    }

    ret = fnv1a(ret) % num_keys_;
    return ret;
  }

  double zeta(long n) {
    double sum = 0.0;
    for (long i = 0; i < n; i++) {
      sum += 1 / std::pow(i + 1, ZIPFIAN_CONSTANT);
    }
    return sum;
  }

  // FNV hash from https://create.stephan-brumme.com/fnv-hash/
  static const uint32_t PRIME = 0x01000193;  //   16777619
  static const uint32_t SEED = 0x811C9DC5;   // 2166136261
  /// hash a single byte
  inline uint32_t fnv1a(unsigned char oneByte, uint32_t hash = SEED) {
    return (oneByte ^ hash) * PRIME;
  }
  /// hash a 32 bit integer (four bytes)
  inline uint32_t fnv1a(int fourBytes, uint32_t hash = SEED) {
    const unsigned char* ptr = (const unsigned char*)&fourBytes;
    hash = fnv1a(*ptr++, hash);
    hash = fnv1a(*ptr++, hash);
    hash = fnv1a(*ptr++, hash);
    return fnv1a(*ptr, hash);
  }
};

string generate_key(const string& key) {
  string result = string(key_size - key.length(), '0') + key;
  return std::move(result);
}

string generate_value(uint64_t value) {
  string value_string = to_string(value);
  string result = string(value_size - value_string.length(), '0') + value_string;
  return std::move(result);
}

int main(int argc, char** argv) {
  auto now = std::chrono::system_clock::now();
  std::time_t current_time = std::chrono::system_clock::to_time_t(now);
  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));
  std::cout << "[START TIME] " << buffer << std::endl;

  // READ FROM ARGS
  std::string db_path, write_type, read_type, dataset_path, exp_log_file;
  double read_ratio, num_data, num_ops;
  uint32_t range_length;
  bool range;

  cxxopts::Options commandline_options("leader tree test", "Testing leader tree.");
  commandline_options.add_options()
    ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
    ("p,db_path", "path of the database", cxxopts::value<std::string>(db_path)->default_value(""))
    ("n,number_data", "data number (million)", cxxopts::value<double>(num_data)->default_value("0"))
    ("m,number_ops", "operation number (million)", cxxopts::value<double>(num_ops)->default_value("0"))
    ("t,read_ratio", "read ratio of the test case", cxxopts::value<double>(read_ratio)->default_value("0"))
    ("w,write_type", "write rand or seq", cxxopts::value<std::string>(write_type)->default_value(""))
    ("r,read_type", "read rand or zipf", cxxopts::value<std::string>(read_type)->default_value(""))
    ("d,dataset_path", "path of the dataset", cxxopts::value<std::string>(dataset_path)->default_value(""))
    ("k, key_size", "byte size of the key", cxxopts::value<uint32_t>(key_size)->default_value("0"))
    ("v, value_size", "byte size of the value", cxxopts::value<uint32_t>(value_size)->default_value("0"))
    ("g, range_query", "use range query", cxxopts::value<bool>(range)->default_value("false"))
    ("l, range_length", "length of range query", cxxopts::value<uint32_t>(range_length)->default_value("0"))
    ("o, output_file", "path of result output file", cxxopts::value<std::string>(exp_log_file)->default_value(""));
  auto result = commandline_options.parse(argc, argv);
  if (result.count("help")) {
    printf("%s", commandline_options.help().c_str());
    exit(0);
  }
  std::ofstream exp_log(exp_log_file.data());

  // REMOVE OLD DATABASE
  system(("rm -rf " + db_path).data());
  system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");

  // CONFIG RocksDB
  DB* db;
  Options options;
  options.level_compaction_dynamic_level_bytes = true;
  options.target_file_size_base = 2 * 1024 * 1024;
  options.paranoid_checks = false;
  options.max_open_files = 65536;
  options.max_background_jobs = 1;
  options.write_buffer_size = 4 * 1024 * 1024;
  options.compression = rocksdb::kNoCompression;

//   options.IncreaseParallelism();
//   options.OptimizeLevelStyleCompaction();
  options.create_if_missing = true;
  ReadOptions read_options = ReadOptions();
  WriteOptions write_options = WriteOptions();
  write_options.sync = false;

  // OPEN OR BUILD DATABASE
  const uint64_t kNum = num_data * M;
  const uint64_t kOps = num_ops * M;
  std::cout << "[1/7] opening database..." << std::endl;
  Status status = DB::Open(options, db_path, &db);

  // READ DATA
  std::ifstream in(dataset_path);
  if (in.fail()) {
    std::cout << "dataset not found" << std::endl;
    exit(0);
  }
  std::cout << "[2/7] reading data..." << std::endl;
  std::vector<std::string> keys;
  keys.resize(kNum);
  for (uint64_t i = 0; i < kNum; i += 1) {
    in >> keys[i];
  }
  std::vector<std::string> insert_keys_after_bulkload;
  insert_keys_after_bulkload.resize(kOps);
  for (uint64_t i = 0; i < kOps; i += 1) {
    in >> insert_keys_after_bulkload[i];
  }
  std::cout << "[3/7] shuffling input data..." << std::endl;
  srand((unsigned int)time(nullptr));
  if (write_type == "rand") {
    std::random_shuffle(keys.begin(), keys.end());
    std::random_shuffle(insert_keys_after_bulkload.begin(), insert_keys_after_bulkload.end());
  } // else `write_typ` is seq, do nothing

  // TEST PUT()
  double bulkload_time = 0;
  std::cout << "[4/7] bulkloading data... " << std::endl;
  for (int i = 0; i < kNum; i += 1) {
    printf("%d\r", i);
    string key = generate_key(keys[i]);
    string val = generate_value(atoi(keys[i].c_str()));
    auto st_time = high_resolution_clock::now();
    status = db->Put(write_options, key, val);
    auto end_time = high_resolution_clock::now();
    bulkload_time += duration_cast<nanoseconds>(end_time - st_time).count();
  }
  std::cout << "bulkload time : " << static_cast<uint64_t>(bulkload_time) / 1e9 << " s" << std::endl;
  exp_log << "bulkload time : " << static_cast<uint64_t>(bulkload_time) / 1e9 << " s" << std::endl;

  std::cout << "[5/7] shuffling query data..." << std::endl;
  std::random_shuffle(keys.begin(), keys.end());

  // TEST CASE
  std::cout << "[6/7] testing... " << std::endl;
  double ops_time = 0;
  std::vector<double> latencies;
  ScrambledZipfianGenerator zipf_gen(keys.size());

  if (range) {
    std::cout << "range querying..." << std::endl;
    auto st_time = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    const uint32_t kRangeOps = 100000;
    for (uint64_t i = 0; i < kRangeOps; i += 1) {
      printf("%ld\r", i);
      // RANGE QUERY
      auto iter = db->NewIterator(read_options);
      string key = generate_key(keys[i]);
      uint32_t cnt = 0;
      st_time = high_resolution_clock::now();
      for (iter->Seek(key); iter->Valid() && cnt < range_length; iter->Next()) {
        cnt ++;
      }
      end_time = high_resolution_clock::now();
      auto duration = duration_cast<nanoseconds>(end_time - st_time).count();
      latencies.push_back(duration);
      ops_time += duration;
    }
    std::sort(latencies.begin(), latencies.end());
    std::cout << "\n";
    std::cout << "average range query time : " << ops_time / kRangeOps << " ns" << std::endl;
    exp_log << "average range query time : " << ops_time / kRangeOps << " ns" << std::endl;
    std::cout << "average range query throughput : " << kRangeOps / (ops_time / 1e9) << " ops" << std::endl;
    exp_log << "average range query throughput : " << kRangeOps / (ops_time / 1e9) << " ops" << std::endl;
    double p99_latency = latencies[static_cast<uint32_t>(kRangeOps * 0.99)];
    std::cout << "average p99 range query time : " << p99_latency << " ns" << std::endl;
    exp_log << "average p99 range query time : " << p99_latency << " ns" << std::endl;
    latencies.clear();
    ops_time = 0;
  }

  // remember that when `read_type` is set to `all`, the workload is read-only
  if (read_type == "all") {
    std::cout << "rand and zipf querying..." << std::endl;
    auto st_time = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    // TEST random read
    for (uint64_t i = 0; i < kOps; i += 1) {
      printf("%ld\r", i);
      std::string key = generate_key(keys[i]);
      string val;
      st_time = high_resolution_clock::now();
      status = db->Get(read_options, key, &val);
      end_time = high_resolution_clock::now();
      auto duration = duration_cast<nanoseconds>(end_time - st_time).count();
      latencies.push_back(duration);
      ops_time += duration;
    }
    std::sort(latencies.begin(), latencies.end());
    std::cout << "\n";
    std::cout << "average random point query time : " << ops_time / kOps << " ns" << std::endl;
    exp_log << "average random point query time : " << ops_time / kOps << " ns" << std::endl;
    std::cout << "average random point query throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    exp_log << "average random point query throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    double p99_latency = latencies[static_cast<uint32_t>(kOps * 0.99)];
    std::cout << "average p99 random point query time : " << p99_latency << " ns" << std::endl;
    exp_log << "average p99 random point query time : " << p99_latency << " ns" << std::endl;
    latencies.clear();
    ops_time = 0;

    // TEST zipfian read
    for (uint64_t i = 0; i < kOps; i += 1) {
      printf("%ld\r", i);
      std::string key = generate_key(keys[zipf_gen.nextValue()]);
      string val;
      st_time = high_resolution_clock::now();
      status = db->Get(read_options, key, &val);
      end_time = high_resolution_clock::now();
      auto duration = duration_cast<nanoseconds>(end_time - st_time).count();
      latencies.push_back(duration);
      ops_time += duration;
    }
    std::sort(latencies.begin(), latencies.end());
    std::cout << "\n";
    std::cout << "average zipfian query time : " << ops_time / kOps << " ns" << std::endl;
    exp_log << "average zipfian query time : " << ops_time / kOps << " ns" << std::endl;
    std::cout << "average zipfian query throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    exp_log << "average zipfian query throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    p99_latency = latencies[static_cast<uint32_t>(kOps * 0.99)];
    std::cout << "average p99 zipfian query time : " << p99_latency << " ns" << std::endl;
    exp_log << "average p99 zipfian query time : " << p99_latency << " ns" << std::endl;
    latencies.clear();
    ops_time = 0;

  } else if (read_type == "rand" || read_type == "zipf") {
    std::cout << "read and write querying..." << std::endl;
    const uint8_t change_threshold = 10 * read_ratio;
    auto st_time = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    for (uint64_t i = 0; i < kOps; i += 1) {
      printf("%ld\r", i);
      // the ops range is [1, 10], split by `change_threshold`
      uint8_t ops = rand() % 10 + 1;
      // POINT QUERY
      if (ops <= change_threshold) {
        string key;
        if (read_type == "rand") {
          key = generate_key(keys[i]);
        } else if (read_type == "zipf") {
          int idx = zipf_gen.nextValue();
          key = generate_key(keys[idx]);
        }
        string val;
        st_time = high_resolution_clock::now();
        status = db->Get(read_options, key, &val);
        end_time = high_resolution_clock::now();
      }
      // WRITE
      else {
        string key = generate_key(insert_keys_after_bulkload[i]);
        string val = generate_value(atoi(insert_keys_after_bulkload[i].c_str()));
        st_time = high_resolution_clock::now();
        status = db->Put(write_options, key, val);
        end_time = high_resolution_clock::now();
      }
      auto duration = duration_cast<nanoseconds>(end_time - st_time).count();
      latencies.push_back(duration);
      ops_time += duration;
    }
    std::sort(latencies.begin(), latencies.end());
    std::cout << "\n";
    std::cout << "average operate time : " << ops_time / kOps << " ns" << std::endl;
    exp_log << "average operate time : " << ops_time / kOps << " ns" << std::endl;
    std::cout << "average operate throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    exp_log << "average operate throughput : " << kOps / (ops_time / 1e9) << " ops" << std::endl;
    double p99_latency = latencies[static_cast<uint32_t>(kOps * 0.99)];
    std::cout << "average p99 operate time : " << p99_latency << " ns" << std::endl;
    exp_log << "average p99 operate time : " << p99_latency << " ns" << std::endl;
    latencies.clear();
    ops_time = 0;
  } // if `none`, do nothing

  std::cout << "[7/7] closing database..." << std::endl;
  delete db;
  in.close();
  exp_log.close();
  now = std::chrono::system_clock::now();
  current_time = std::chrono::system_clock::to_time_t(now);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));
  std::cout << "[END TIME] " << buffer << std::endl;
  return 0;
}