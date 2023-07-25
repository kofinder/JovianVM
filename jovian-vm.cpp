
#include <string>

#include "./src/JovianVM.h"

int main(int argc, char const *argv[]) {

    std::string program = R"(
        printf "Value: %d\n" 42
    )";

    JovianVM vm;

    vm.execute(program);

    return 42;
}
