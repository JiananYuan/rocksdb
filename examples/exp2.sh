# 编译: cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG_SWITCH=ON

# todo : 更多数据集?
for dist in books_100M fb_100M osm_100M wiki_100M; do
	  echo "[${dist} 测试1] 只读吞吐率 | 99尾延迟 | zipf点查询平均延迟 | 随机加载构建时间"
    ./exp2 -p test_read_only -n 10 -m 2 -w rand -r zipf -d ${dist} -k 20 -v 64 -o ../results/${dist}_exp_read_only.txt
    rm -r test_read_only

    # todo : 以下三个用 YCSB 测试来替代?
    echo "[${dist} 测试2] 读多写少吞吐率"
    ./exp2 -p test_read_more -n 10 -m 2 -w rand -r read_heavy -d ${dist} -k 20 -v 64 -o ../results/${dist}_exp_read_rand_more.txt
    rm -r test_read_more

    echo "[${dist} 测试3] 读写均衡吞吐率"
    ./exp2 -p test_read_midd -n 10 -m 2 -w rand -r rw_balance -d ${dist} -k 20 -v 64 -o ../results/${dist}_exp_read_rand_midd.txt
    rm -r test_read_midd

    echo "[${dist} 测试4] 写多读少吞吐率"
    ./exp2 -p test_read_less -n 10 -m 2 -w rand -r write_heavy -d ${dist} -k 20 -v 64 -o ../results/${dist}_exp_read_rand_less.txt
    rm -r test_read_less

    echo "[${dist} 测试5] 顺序加载构建时间"
    ./exp2 -p test_load_seq -n 10 -m 0 -w seq -d ${dist} -k 20 -v 64 -o ../results/${dist}_exp_load_seq.txt
    rm -r test_load_seq
done

## todo : 范围查询?
#
## todo : 更多数据集?
#for vs in 64 128 256 512 1024; do
#    echo "[测试6 value_size: ${vs}] 不同value_size 对zipf只读情形吞吐量的影响"
#	  ./exp2 -p test_value_size -n 50 -m 10 -w rand -r zipf -d books_100M -k 20 -v ${vs} -o ../results/exp_value_size_${vs}.txt
#    rm -r test_value_size
#done
#
## todo : 更多数据集?
#for eb in 4 8 16 32 64; do
#    echo "[测试7 error_bound: ${eb}] 不同error_bound 对索引大小和zipf点查询吞吐量的影响"
#	   ./exp2 -p test_error_bound -n 50 -m 10 -w rand -r zipf -d books_100M -k 20 -v 64 -e ${eb} -o ../results/exp_error_bound_${eb}.txt
#    rm -r test_error_bound
#done
#
## 使用此命令前, 需要cd 到YCSB-CPP目录, 删除旧生成文件, 执行命令:
## make BIND_LEVELDB=1 EXTRA_CXXFLAGS=-I/home/yjn/Desktop/VLDB/leader-tree EXTRA_LDFLAGS="-L/home/yjn/Desktop/VLDB/leader-tree/build -lsnappy"
#cd ../../YCSB-cpp
#for wlt in a b c d e f; do
#    echo "[测试8 workload: ${wlt}] YCSB测试"
#    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
#    ./ycsb -load -db leveldb -P workloads/workload${wlt} -P leveldb/leveldb.properties -s
#    rm -r /tmp/ycsb-*
#done
#
## 使用此命令前，需要增加cmake选项, 命令: cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG_SWITCH=ON -DINTERNAL_TIMER_SWITCH=ON
# echo "[测试9] zipf点查询时延breakdown"
# ./exp2 -p test_read_breakdown -n 50 -m 10 -w rand -r zipf -d books_100M -k 20 -v 64 -o ../results/exp_read_breakdown.txt
# rm -r test_read_breakdown
