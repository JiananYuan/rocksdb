#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

// #include "leveldb/comparator.h"
// #include "leveldb/db.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/compression_type.h"
#include "rocksdb/filter_policy.h"
#include "cxxopts.hpp"
#include "var.h"

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
  // system("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches");
  // print now time
  auto now = std::chrono::system_clock::now();
  std::time_t current_time = std::chrono::system_clock::to_time_t(now);
  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));
  std::cout << "[START TIME] " << buffer << std::endl;

  // READ FROM ARGS
  std::string db_path, write_type, ds_name, exp_log_file;
  double num_data, num_ops;
  const string dir_path = "/home/yjn/Desktop/VLDB/Dataset/";
  const std::string kWrite = "W";
  const std::string kRead = "R";
  int start_type;
  bool range_query;

  cxxopts::Options commandline_options("leader tree test", "Testing leader tree.");
  commandline_options.add_options()
    ("h,help", "print help message", cxxopts::value<bool>()->default_value("false"))
    ("p,db_path", "path of the database", cxxopts::value<std::string>(db_path)->default_value("minitest"))
    ("n,number_data", "data number (million)", cxxopts::value<double>(num_data)->default_value("50"))
    ("m,number_ops", "operation number (million)", cxxopts::value<double>(num_ops)->default_value("10"))
    ("w,write_type", "write rand or seq", cxxopts::value<std::string>(write_type)->default_value("rand"))
    ("d,dataset_name", "name of the dataset", cxxopts::value<std::string>(ds_name)->default_value("books"))
    ("k, key_size", "byte size of the key", cxxopts::value<uint32_t>(key_size)->default_value("20"))
    ("v, value_size", "byte size of the value", cxxopts::value<uint32_t>(value_size)->default_value("64"))
    ("o, output_file", "path of result output file", cxxopts::value<std::string>(exp_log_file)->default_value("minilog"))
    ("r, range_query", "use range query", cxxopts::value<bool>(range_query)->default_value("false"))
    ("s, start_type", "start type id", cxxopts::value<int>(start_type)->default_value("0"));
  auto result = commandline_options.parse(argc, argv);
  if (result.count("help")) {
    printf("%s", commandline_options.help().c_str());
    exit(0);
  }
  std::ofstream exp_log(exp_log_file);
  const std::string ds_rand_path = dir_path + ds_name + "/" + ds_name + "_rand_data.csv";
  const std::string ds_seq_path = dir_path + ds_name + "/" + ds_name + "_seq_data.csv";
  const std::string ds_rand_query = dir_path + ds_name + "/" + ds_name + "_rand_query.csv";
  const std::string ds_zipf_query = dir_path + ds_name + "/" + ds_name + "_zipf_query.csv";
  const std::string ds_read_heavy = dir_path + ds_name + "/" + ds_name + "_read_heavy.csv";
  const std::string ds_rw_balance = dir_path + ds_name + "/" + ds_name + "_rw_balance.csv";
  const std::string ds_write_heavy = dir_path + ds_name + "/" + ds_name + "_write_heavy.csv";

  // REMOVE OLD DATABASE
  // system(("rm -rf " + db_path).data());

  // CONFIG LEADER-TREE
  DB* db;
  Options options;
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  options.create_if_missing = true;
  // options.level_compaction_dynamic_level_bytes = true;
  options.write_buffer_size = 64 * 1024 * 1024;
  options.target_file_size_base = 64 * 1024 * 1024;
  options.paranoid_checks = false;
  options.max_open_files = 65536;
  options.max_background_jobs = 1;
  options.compression = rocksdb::kNoCompression;

  ReadOptions read_options = ReadOptions();
  WriteOptions write_options = WriteOptions();
  write_options.sync = false;

  // OPEN OR BUILD DATABASE
  const uint64_t kNum = num_data * M;
  const uint64_t kOps = num_ops * M;
  std::cout << "[1/4] opening database..." << std::endl;
  Status status = DB::Open(options, db_path, &db);

  // READ DATA
  double bulkload_time = 0;
  int print_interval = kNum / 128;
  std::ifstream in;
  if (write_type == "rand")   in.open(ds_rand_path);
  if (write_type == "seq")    in.open(ds_seq_path);

  // TEST PUT()
  std::cout << "[2/4] bulkloading data... " << std::endl;
  for (int i = 0; i < kNum; i += 1) {
    if (i % print_interval == 0) {
      printf("\rprogress : %.2f%%", 100.0 * i / kNum);
      fflush(stdout);
    }
    std::string key;
    in >> key;
    key = generate_key(key);
    string val = generate_value(strtoull(key.c_str(), nullptr, 10));
    auto st_time = high_resolution_clock::now();
    status = db->Put(write_options, key, val);
    auto end_time = high_resolution_clock::now();
    bulkload_time += duration_cast<nanoseconds>(end_time - st_time).count();
  }
  std::cout << std::endl;
  std::cout << "bulkload time : " << static_cast<uint64_t>(bulkload_time) / 1e9 << " s" << std::endl;
  exp_log << "bulkload time : " << static_cast<uint64_t>(bulkload_time) / 1e9 << " s" << std::endl;
  in.close();
  delete db;
  options.max_background_jobs = 1;
  status = DB::Open(options, db_path, &db);

  // TEST CASE
  for (int ei = start_type; ei < 4; ei += 1) {
    switch (ei) {
      case 0:
        std::cout << "[3/4] testing-1 (write-more)... " << std::endl;
        in.open(ds_write_heavy);
        break;
      case 1:
        std::cout << "[3/4] testing-2 (rw_balance)... " << std::endl;
        in.open(ds_rw_balance);
        break;
      case 2:
        std::cout << "[3/4] testing-3 (read_more)... " << std::endl;
        in.open(ds_read_heavy);
        break;
      case 3:
        std::cout << "[3/4] testing-4 (read-only)... " << std::endl;
        in.open(ds_zipf_query);
        break;
    }
    double ops_time = 0;
    std::vector<double> latencies;

    // remember that when `read_type` is set to `rand` or `zipf`, the workload is read-only
    print_interval = kOps / 128;
    auto st_time = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    uint32_t num_real_ops = 0;
    bool exist_not_found = false;
    for (uint64_t i = 0; i < kOps; i += 1) {
      if (i % print_interval == 0) {
        printf("\rprogress : %.2f%%", 100.0 * i / kOps);
        fflush(stdout);
      }
      // if `read_type` equal to range, we perform `kNumRange` range queries
      std::string line;
      getline(in, line);
      std::stringstream ss(line);
      ss >> line;
      if (line == kWrite) {
        ss >> line;  // key
        line = generate_key(line);
        std::string val = generate_value(atoi(line.c_str()));
        st_time = high_resolution_clock::now();
        status = db->Put(write_options, line, val);
        end_time = high_resolution_clock::now();
      } else if (line == kRead) {
        ss >> line;  // key
        line = generate_key(line);
        if (range_query) {
          rocksdb::ReadOptions iter_option;
          iter_option.fill_cache = false;
          auto iter = db->NewIterator(iter_option);
          const int range_length = 100;
          uint32_t cnt = 0;
          st_time = high_resolution_clock::now();
          for (iter->Seek(line); iter->Valid() && cnt < range_length; iter->Next()) {
            cnt += 1;
          }
          end_time = high_resolution_clock::now();
          delete iter;
        } else {
          string val;
          st_time = high_resolution_clock::now();
          status = db->Get(read_options, line, &val);
          end_time = high_resolution_clock::now();
          if (status.IsNotFound())    exist_not_found = true;
        }
      }
      auto duration = duration_cast<nanoseconds>(end_time - st_time).count();
      latencies.push_back(duration);
      ops_time += duration;
      num_real_ops += 1;
    }
    if (kOps != 0) {
      std::sort(latencies.begin(), latencies.end());
      std::cout << "\n";
      if (exist_not_found)    std::cout << "exist not found" << std::endl;
      std::cout << "average operate time : " << ops_time / num_real_ops << " ns" << std::endl;
      exp_log << "average operate time : " << ops_time / num_real_ops << " ns" << std::endl;
      std::cout << "average operate throughput : " << num_real_ops / (ops_time / 1e9) << " ops" << std::endl;
      exp_log << "average operate throughput : " << num_real_ops / (ops_time / 1e9) << " ops" << std::endl;
      double p99_latency = latencies[static_cast<uint32_t>(num_real_ops * 0.99)];
      std::cout << "average p99 operate time : " << p99_latency << " ns" << std::endl;
      exp_log << "average p99 operate time : " << p99_latency << " ns" << std::endl;
    }
    if (in.is_open())   in.close();
  }
  
  std::cout << "[4/4] closing database..." << std::endl;
  std::cout << "index size: " << adgMod::idx_sz << std::endl;
  delete db;
  exp_log.close();
  now = std::chrono::system_clock::now();
  current_time = std::chrono::system_clock::to_time_t(now);
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));
  std::cout << "[END TIME] " << buffer << std::endl;
  return 0;
}
