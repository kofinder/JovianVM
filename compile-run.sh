# Compile main:
clang++ -o jovian-vm `llvm-config-14 --cxxflags --ldflags --system-libs --libs core` main.cpp -fexceptions

# Run main:
./jovian-vm -f test.eva

# Execute generated IR:
lli-14 ./out.ll

# Optimize the output:
# opt-14 ./out.ll -O3 -S -o ./out-opt.ll

# Compile ./out.ll with GC:
#
# Note: to install GC_malloc:
#
#   brew install libgc
#
clang++ -O3 -I/usr/local/include/gc/ ./out.ll /usr/local/lib/libgc.a -o ./out

# Run the compiled program:
./out

# Print result:
echo $?

printf "\n"
