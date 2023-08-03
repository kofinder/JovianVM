#include <vector>
#include <llvm/IR/Value.h>

class CodeGenContext;

using namespace llvm;

namespace JovianVM {

    class Node {
        public:
        virtual ~Node() {}
        virtual Value* codeGen(CodeGenContext& context) { return NULL; }
    };

    class NStatementNode : public Node {};

    class NDeclarationNode: public Node {};

    class NExpressionNode : public Node { };

    class NTypeNode : public Node {};

    class NErrorNode : public Node {};

}