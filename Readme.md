# Step 1: Automatic installation script
    - bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
    - wget https://apt.llvm.org/llvm.sh
    - wget https://apt.llvm.org/llvm.sh
    - sudo ./llvm.sh all

# Step 2: Add the following first/etc/apt/source.list
    - deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main
    - deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main

# Step 3: Then execute the following shell command
    - sudo apt update
    - sudo apt install clang-13

# Eva Run Command
    - chmod +x compille-run.sh
    
# Run Command
    - clang++ -S -emit-llvm test
    - clang++ -o test test.ll
    - echo $? # see output
    - lli-14 file.ll
    - clang++ -o test test.ll; ./test; echo $ # full command
    - llvm-as-14 test.ll # Assambly
    - llvm-dis-14 test.bc -o test2.l # Dis Assembly
    - clang++ -S test.ll
    - clang++ -o test test.s; ./test; echo $?

# REFRENCES
    - https://apt.llvm.org/
    - https://llvm.org/docs/LangRef.html





# bdw-gc setup
 - tar xvfz gc<version>.tar.gz
# cd gc<version>
# ./configure --enable-static=yes --enable-shared=no --enable-cplusplus
# make
# make check
# sudo make install