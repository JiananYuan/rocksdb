# 编译: make exp3 -j (若改了 rocksdb 源码，则还需要: make static_lib)

key_size=20
val_size=64
init_scale=64
test_scale=10

rm -rf test_*

for dist in books fb osm wiki logn uniform; do
    echo "[${dist} 测试1] 随机加载 | 吞吐率 | 99尾延迟 | zipf点查询平均延迟"
    ./exp3 -p test_read_only_${dist} -n ${init_scale} -m ${test_scale} -w rand -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_only.txt
    rm -r test_read_only_${dist}

    echo "[${dist} 测试2] 顺序加载"
    ./exp3 -p test_load_seq_${dist} -n ${init_scale} -m 0 -w seq -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_load_seq.txt
    rm -r test_load_seq_${dist}
done

#for vs in 128 256 512 1024; do
#    echo "[测试3 value_size: ${vs}] 不同value_size 对吞吐量的影响"
#    ./exp3 -p test_value_size -n ${init_scale} -m ${test_scale} -w rand -d books -k ${key_size} -v ${vs} -o ../results/${dist}_exp_value_size_${vs}.txt
#    rm -r test_value_size
#done

# 使用此命令前，需要增加cmake选项, 命令: cmake .. -DCMAKE_BUILD_TYPE=Release -DNDEBUG_SWITCH=ON -DINTERNAL_TIMER_SWITCH=ON
#echo "[测试4] zipf点查询时延breakdown"
#./exp3 -p test_read_breakdown -n ${init_scale} -m ${test_scale} -w rand -s 3 -d books -k ${key_size} -v ${val_size} -o ../results/exp_read_breakdown.txt
#rm -r test_read_breakdown
