# Compile main:
clang++ -o jovian-vm `llvm-config-14 --cxxflags --ldflags --system-libs --libs core` jovian-vm.cpp -fexceptions

# Run main:
./jovian-vm

# Execute generated IR:
lli-14 ./out.ll

echo $?

printf "\n"
