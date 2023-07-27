# make static_lib -j   make -j

# echo "[测试] 只读情形吞吐率、p99延迟、平均延迟(包含rand和zipf) | 范围查询吞吐率 | 随机加载构建时间"
# ./exp1 -p test_read_rand_only -n 100 -m 20 -t 1 -w rand -r all -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv -k 16 -v 64 -g -l 500 -o exp_test_read_rand_only.result

echo "[测试] 读多写少吞吐率"
./exp1 -p test_read_rand_more -n 20 -m 100 -t 0.9 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv -k 16 -v 64 -o exp_test_read_rand_more.result

echo "[测试] 读写均衡吞吐率"
./exp1 -p test_read_rand_midd -n 20 -m 100 -t 0.5 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv -k 16 -v 64 -o exp_test_read_rand_midd.result

echo "[测试] 写多读少吞吐率"
./exp1 -p test_read_rand_less -n 20 -m 100 -t 0.1 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv -k 16 -v 64 -o exp_test_read_rand_less.result

# echo "[测试] 顺序加载构建时间"
# ./exp1 -p test_load_seq -n 100 -m 0 -w seq -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv -k 16 -v 64 -o exp_test_load_seq.result
