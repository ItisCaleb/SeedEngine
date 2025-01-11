mkdir -p build
cd build && cmake .. -DTEST=ON
make -j4 && ./unit_test