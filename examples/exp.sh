# 编译: make static_lib -j  make -j

# todo : 更多数据集?
for dist in books_200M_uint32 fb_200M_uint64 osm_cellids_200M_uint64 wiki_ts_200M_uint64; do
	echo "[测试1 数据集: ${dist}] 只读吞吐率 | 99尾延迟 | rand和zipf点查询平均延迟 | 范围查询吞吐率 | 随机加载构建时间"
	./exp1 -p test_read_rand_only -n 100 -m 20 -t 1 -w rand -r all -d /media/yjn/jiananyuan/DataSet/SOSD/${dist}.csv \
		   -k 16 -v 64 -g -l 500 -o ./results/exp_read_rand_only_${dist}.result
	rm -r test_read_rand_only

	echo "[测试2 数据集: ${dist}] 读多写少吞吐率"
	./exp1 -p test_read_rand_more -n 20 -m 100 -t 0.9 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/${dist}.csv \
		   -k 16 -v 64 -o ./results/exp_read_rand_more_${dist}.result
	rm -r test_read_rand_more

	echo "[测试3 数据集: ${dist}] 读写均衡吞吐率"
	./exp1 -p test_read_rand_midd -n 20 -m 100 -t 0.5 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/${dist}.csv \
		   -k 16 -v 64 -o ./results/exp_read_rand_midd_${dist}.result
	rm -r test_read_rand_midd

	echo "[测试4 数据集: ${dist}] 写多读少吞吐率"
	./exp1 -p test_read_rand_less -n 20 -m 100 -t 0.1 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/${dist}.csv \
	 	   -k 16 -v 64 -o ./results/exp_read_rand_less_${dist}.result
	rm -r test_read_rand_less

	echo "[测试5 数据集: ${dist}] 顺序加载构建时间"
	./exp1 -p test_load_seq -n 100 -m 0 -w seq -d /media/yjn/jiananyuan/DataSet/SOSD/${dist}.csv \
	 	   -k 16 -v 64 -o ./results/exp_load_seq_${dist}.result
	rm -r test_load_seq
done

# todo : 更多数据集?
for vs in 64 128 256 512 1024; do
	echo "[测试6 value_size: ${vs}] 不同value_size 对随机只读情形吞吐量的影响"
	./exp1 -p test_value_size -n 100 -m 20 -t 1 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv \
		   -k 16 -v ${vs} -g -l 500 -e 4 -o ./results/exp_value_size_${vs}.result
	rm -r test_value_size
done

# todo : 更多数据集?
for eb in 4 8 16 32 64; do
	echo "[测试7 error_bound: ${eb}] 不同error_bound 对索引大小和随机点查询吞吐量的影响"
	./exp1 -p test_error_bound -n 100 -m 20 -t 1 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv \ 
		   -k 16 -v 64 -g -l 500 -e ${eb} -o ./results/exp_error_bound_${eb}.result
	rm -r test_error_bound
done

# 使用此命令前, 需要cd 到YCSB-CPP目录, 删除旧生成文件, 执行命令: 
# make BIND_ROCKSDB=1 EXTRA_CXXFLAGS=-I/home/yjn/Desktop/VLDB/rocksdb EXTRA_LDFLAGS="-L/home/yjn/Desktop/VLDB/rocksdb/build -lsnappy"
cd ../../YCSB-cpp
for wlt in a b c d e f; do
	echo "[测试8 workload: ${wlt}] YCSB测试"
	sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
	./ycsb -load -db rocksdb -P workloads/workload${wlt} -P rocksdb/rocksdb.properties -s
	rm -r /tmp/ycsb-*
done

# 使用此命令前，需要增加cmake选项, 命令: cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG_SWITCH=ON -DINTERNAL_TIMER_SWITCH=ON
# echo "[测试9] 随机点查询时延breakdown"
# ./exp1 -p test_read_breakdown -n 100 -m 20 -t 1 -w rand -r rand -d /media/yjn/jiananyuan/DataSet/SOSD/books_200M_uint32.csv \
# 	   -k 16 -v 64 -e 4 -o ./results/exp_read_breakdown.result
# rm -r test_read_breakdown
