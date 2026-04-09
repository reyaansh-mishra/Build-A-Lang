#pragma once
#include "AST.hpp"
#include "Arena.hpp"
#include "lexer.hpp"
#include <cstddef>
#include <vector>

class Parser {
    public:
        explicit Parser(std::vector<Token_s> tokens, Arena &arena) : m_arena(arena), m_Tokens(std::move(tokens)), m_pos(0) {};
        ASTNode* parse();
        void debug_walk();

    private:
        Arena &m_arena;

        std::vector<Token_s> m_Tokens;
        size_t m_pos;

        // Ret current token
        const Token_s &Peek() const;
        const Token_s &Peek(size_t variation) const;

        // Ret current token and advance forward
        const Token_s &Advance();

        // Ret true if at end
        bool IsThisTheEnd() const;
        void Log(const std::string& action) const;

        // "Consume" a token
        Token_s Consume(Tokens type, const std::string &message);

        ASTNode* parseStatement();
        ASTNode* parseExpression();
        ASTNode* parsePrimary();
        ASTNode* parseTerm();
        BlockNode *parseBlock();
};