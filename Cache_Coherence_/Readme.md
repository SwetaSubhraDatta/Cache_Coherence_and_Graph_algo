if using cmake

cd Cache_Caherence_

mkdir build
cd build
cmake ..

make

./Cache_Caherence_ <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file>


if using make::

cd Cache_Caherence_
make
././smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> 
