#include <sstream>
#include <map>
#include "AST.hpp"

class Generator {
public:
    explicit Generator(ASTNode* root) : m_root(root) {}
    std::string generate();

private:
    ASTNode* m_root;
    std::stringstream m_output;
    std::map<std::string, int> m_vars; // Tracks where 'x' is on the stack
    int m_stack_ptr = 0;
    int m_label_count = 0;
    const std::string reg_map[6] = {"rdi", "rsi", "rdx", "r10", "r8", "r9"};

    // 1. Helper: Puts the result of any expression into RAX
    void genExpr(const ASTNode* expr);

    // 2. genStatement
    void genStatement(const ASTNode* stmt);

    void emit(std::string instr);
};