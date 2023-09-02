# 编译: make exp2 -j (若改了 rocksdb 源码，则还需要: make static_lib)

key_size=20
val_size=64
init_scale=1
test_scale=0.2

for dist in books fb osm wiki; do
    echo "[${dist} 测试1] 只读吞吐率 | 99尾延迟 | zipf点查询平均延迟 | 随机加载构建时间"
    ./exp2 -p test_read_only -n ${init_scale} -m ${test_scale} -w rand -r zipf -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_only.txt
    # 复制当前数据库给测试2使用
    cp -r test_read_only test_read_more
    # 复制当前数据库给测试3使用
    cp -r test_read_only test_read_midd
    # 复制当前数据库给测试4使用
    cp -r test_read_only test_read_less
    if [ ${dist} == books ]; then
        # 复制当前数据库给测试6使用
        cp -r test_read_only test_range
        # 复制当前数据库给测试10使用
        cp -r test_read_only test_read_breakdown
    fi
    rm -r test_read_only

    echo "[${dist} 测试2] 读多写少吞吐率"
    ./exp2 -p test_read_more -n ${init_scale} -m ${test_scale} -w none -r read_heavy -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_rand_more.txt
    rm -r test_read_more

    echo "[${dist} 测试3] 读写均衡吞吐率"
    ./exp2 -p test_read_midd -n ${init_scale} -m ${test_scale} -w none -r rw_balance -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_rand_midd.txt
    rm -r test_read_midd

    echo "[${dist} 测试4] 写多读少吞吐率"
    ./exp2 -p test_read_less -n ${init_scale} -m ${test_scale} -w none -r write_heavy -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_rand_less.txt
    rm -r test_read_less

    echo "[${dist} 测试5] 顺序加载构建时间"
    ./exp2 -p test_load_seq -n ${init_scale} -m 0 -w seq -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_load_seq.txt
    rm -r test_load_seq
done

echo "[测试6] 范围查询时延 | 吞吐率"
./exp2 -p test_range -n ${init_scale} -m 1 -w none -r range -l 500 -d books -k ${key_size} -v ${val_size} -o ../results/exp_read_breakdown.txt
rm -r test_range

# for vs in 128 256 512 1024; do
#     echo "[测试7 value_size: ${vs}] 不同value_size 对zipf只读情形吞吐量的影响"
#     ./exp2 -p test_value_size -n ${init_scale} -m ${test_scale} -w rand -r zipf -d books -k ${key_size} -v ${vs} -o ../results/exp_value_size_${vs}.txt
#     rm -r test_value_size
# done

# # 使用此命令前, 需要cd 到YCSB-CPP目录, 删除旧生成文件, 执行命令:
# # make BIND_LEVELDB=1 EXTRA_CXXFLAGS=-I/home/yjn/Desktop/VLDB/leader-tree EXTRA_LDFLAGS="-L/home/yjn/Desktop/VLDB/leader-tree/build -lsnappy"
# cd ../../YCSB-cpp
# for wlt in a b c d e f; do
#    echo "[测试9 workload: ${wlt}] YCSB测试"
#    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
#    ./ycsb -load -db leveldb -P workloads/workload${wlt} -P leveldb/leveldb.properties -s
#    rm -r /tmp/ycsb-*
# done

# # 使用此命令前，需要增加cmake选项, 命令: cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG_SWITCH=ON -DINTERNAL_TIMER_SWITCH=ON
# echo "[测试10] zipf点查询时延breakdown"
# ./exp2 -p test_read_breakdown -n ${init_scale} -m ${test_scale} -w none -r zipf -d books -k ${key_size} -v ${val_size} -o ../results/exp_read_breakdown.txt
# rm -r test_read_breakdown
