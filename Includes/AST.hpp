#pragma once
#include <string_view>
#include <vector>
#include <string>

struct ASTNode {
    virtual std::string dump() const = 0;
};

struct IntegerNode : public ASTNode {
    int val;
    explicit IntegerNode(int v) : val(v) {}
    std::string dump() const override { return std::to_string(val); }
};

struct IdentifierNode : public ASTNode {
    std::string_view name; // Optimization: No heap allocation for name
    explicit IdentifierNode(std::string_view n) : name(n) {}
    std::string dump() const override { return "ID(" + std::string(name) + ")"; }
};

struct BinaryExprNode : public ASTNode {
    std::string_view op;
    ASTNode* lhs; // Optimization: Raw pointer (Arena-managed)
    ASTNode* rhs;

    BinaryExprNode(std::string_view o, ASTNode* l, ASTNode* r) 
        : op(o), lhs(l), rhs(r) {}

    std::string dump() const override {
        return "(" + lhs->dump() + " " + std::string(op) + " " + rhs->dump() + ")";
    }
};

struct UnaryExprNode : public ASTNode {
    std::string_view op;
    ASTNode* expr;
    UnaryExprNode(std::string_view o, ASTNode* e) : op(o), expr(e) {}
    std::string dump() const override { return "(" + std::string(op) + expr->dump() + ")"; }
};

struct VarDeclarationNode : public ASTNode {
    std::string_view name;
    ASTNode* init_val;
    VarDeclarationNode(std::string_view n, ASTNode* val) 
        : name(n), init_val(val) {}
    std::string dump() const override {
        return "VAR_DECL(" + std::string(name) + " = " + (init_val ? init_val->dump() : "null") + ")";
    }
};

struct PrintNode : public ASTNode {
    ASTNode* expr;
    explicit PrintNode(ASTNode* e) : expr(e) {};
    std::string dump() const override { return "PRINT(" + expr->dump() + ")"; }
};

struct ReturnNode : public ASTNode {
    ASTNode* expr;
    explicit ReturnNode(ASTNode* e) : expr(e) {}
    std::string dump() const override { return "RETURN(" + (expr ? expr->dump() : "void") + ")"; }
};

struct BlockNode : public ASTNode {
    std::vector<ASTNode*> statements;
    std::string dump() const override {
        std::string out = "BLOCK {\n";
        for (auto s : statements) out += "    " + s->dump() + "\n";
        out += "  }";
        return out;
    }
};

struct IfStatementNode : public ASTNode {
    ASTNode* condition;
    BlockNode* then_block;
    BlockNode* else_block; 
    IfStatementNode(ASTNode* cond, BlockNode* t, BlockNode* e = nullptr)
        : condition(cond), then_block(t), else_block(e) {}
    std::string dump() const override {
        return "IF(" + condition->dump() + ") " + then_block->dump() + (else_block ? " ELSE " + else_block->dump() : "");
    }
};

struct ProgramNode : public ASTNode {
    std::vector<ASTNode*> statements;
    std::string dump() const override {
        std::string out = "--- PROGRAM AST ---\n";
        for (auto s : statements) out += "  " + s->dump() + "\n";
        return out;
    }
};

struct AssignmentNode : public ASTNode {
    std::string_view name;
    ASTNode *expr;

    AssignmentNode(std::string_view n, ASTNode* e) : name(n), expr(e) {}

    std::string dump() const override {
        return "ASSIGN(" + std::string(name) + " = " + expr->dump() + ")";
    }
};