# 编译: make exp3 -j (若改了 rocksdb 源码，则还需要: make static_lib)

key_size=20
val_size=64
init_scale=64
test_scale=10

rm -rf test_*

for dist in books fb osm wiki logn uniform; do
#    echo "[${dist} 测试1] 只读吞吐率 | 99尾延迟 | zipf点查询平均延迟 | 随机加载构建时间"
#    ./exp3 -p test_read_only_${dist} -n ${init_scale} -m ${test_scale} -w rand -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_read_only.txt
#    rm -r test_read_only_${dist}

    echo "[${dist} 测试5] 顺序加载构建时间"
    ./exp3 -p test_load_seq_${dist} -n ${init_scale} -m 0 -w seq -d ${dist} -k ${key_size} -v ${val_size} -o ../results/${dist}_exp_load_seq.txt
    rm -r test_load_seq_${dist}
done
