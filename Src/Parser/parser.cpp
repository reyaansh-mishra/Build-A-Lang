#include "parser.hpp"
#include "AST.hpp"
#include "lexer.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>


// ------------------------------------------------------------------------------ //
// ------------------------ END OF INCLUDES/#gs --------------------------------- //
// ------------------------------------------------------------------------------ //
// -------------------- START OF BASIC PARSER LOGIC ----------------------------- //
// ------------------------------------------------------------------------------ //

ASTNode* Parser::parse() {
    auto program = m_arena.alloc<ProgramNode>(); // Create the root
    
    while (!IsThisTheEnd() && Peek().TYPE != Tokens::Eof) {
        // 1. Capture the statement
        auto stmt = parseStatement(); 
        
        // 2. If it's not null, move it into the Program's vector
        if (stmt) {
            program->statements.push_back(std::move(stmt));
        }
    }
    
    return program; // Return the fully populated tree
}


bool Parser::IsThisTheEnd() const {
    if (m_pos >= m_Tokens.size()) {return true;}
    else {return false;}
};


const Token_s &Parser::Peek() const {

    // Safety check: If for some reason we overshot, always return the EOF token
    if (IsThisTheEnd()) { return m_Tokens.back(); };

    return m_Tokens[m_pos];
};

const Token_s &Parser::Peek(size_t variation) const {

    // Safety check: If for some reason we overshot, always return the EOF token
    if (m_pos + variation >= m_Tokens.size()) {
        return m_Tokens.back(); // Always return EOF if we look too far
    }

    return m_Tokens[m_pos + variation];
};

const Token_s &Parser::Advance() {
    if (IsThisTheEnd()) { return m_Tokens.back(); };

    size_t m_pos_temp = m_pos;
    m_pos++;

    return m_Tokens[m_pos_temp];
};


Token_s Parser::Consume(Tokens type, const std::string& err_message) {
    if (Peek().TYPE != type) {
        std::string foundValue = Peek().value;
        int foundType = (int)Peek().TYPE;
        int expectedType = (int)type;

        // Build a detailed error string
        std::string full_msg = "\n[PARSE ERROR]\n"
                               "Expected: Type " + std::to_string(expectedType) + " (Check your enum)\n"
                               "Found   : Type " + std::to_string(foundType) + " ('" + foundValue + "')\n"
                               "Context : " + err_message + "\n";
        
        throw std::runtime_error(full_msg);
    }
    return Advance();
}


void Parser::Log(const std::string& action) const {
    const Token_s& t = Peek();
    std::cout << "[Parser]" << action 
              << " | Current Token: " << t.value 
              << " (Type: " << static_cast<int>(t.TYPE) << ")"
              << " | Pos: " << m_pos << "\n";
}


void Parser::debug_walk() {
    while (!IsThisTheEnd()) {
        std::cout << "Parsing Statement starting with: " << Peek().value << "\n";
        parseStatement(); 
        std::cout << "Moving to next statement. Current pos: " << m_pos << "\n";
    }
}


// ------------------------------------------------------------------------------ //
// ------------------------ END OF BASIC PARSER LOGIC --------------------------- //
// ------------------------------------------------------------------------------ //
// ------------------------ START OF AST/ADV. LOGIC ----------------------------- //
// ------------------------------------------------------------------------------ //

ASTNode* Parser::parseStatement() {

    // The "Every End" Fix: Allow standalone semicolons
    if (Peek().TYPE == Tokens::Semicolon) {
        Advance(); 
        return nullptr; // Return nothing, parse() will just skip this null
    }

    if (Peek().TYPE == Tokens::Identifier && Peek(1).TYPE == Tokens::Equal) {
        Token_s ident = Advance(); // The name
        Advance();                // The '='
        auto* expr = parseExpression();
        Consume(Tokens::Semicolon, "Expected ';' after assignment");
        
        return m_arena.alloc<AssignmentNode>(m_arena.dup_string(ident.value), expr);
    }

    
    switch (Peek().TYPE) {
        case Tokens::Ret: {
            Advance(); // ret
            auto expr = parseExpression(); 
            Consume(Tokens::Semicolon, "Expected ';'");
            
            // Construct and RETURN the node
            return m_arena.alloc<ReturnNode>(std::move(expr)); 
        }

        case Tokens::Eof: 
            return nullptr;

        // Example: parseStatement's Let case
        case Tokens::Let: {
            Advance(); // 'let'
            Token_s ident = Consume(Tokens::Identifier, "Expected variable name");
            Consume(Tokens::Equal, "Expected '='");
            auto* expr = parseExpression();
            Consume(Tokens::Semicolon, "Expected ';'");
            
            // Use dup_string to ensure the identifier name lives as long as the Arena
            return m_arena.alloc<VarDeclarationNode>(m_arena.dup_string(ident.value), expr);        
        }

        // Example: parsePrimary's Identifier case
        if (Peek().TYPE == Tokens::Identifier) {
            return m_arena.alloc<IdentifierNode>(m_arena.dup_string(Advance().value)); //
        }

        case Tokens::Print: {
            Advance(); // Consume 'print'

            Consume(Tokens::LParen, "Expected '(' after 'print'");
            auto expr = parseExpression();
            Consume(Tokens::RParen, "Expected ')' after expression");
            Consume(Tokens::Semicolon, "Expected ';' after print statement");

            return m_arena.alloc<PrintNode>(std::move(expr));
        };

        case Tokens::If: {
            Advance();  // Consume 'if'
            Consume(Tokens::LParen, "Expected '('");
            auto* condition = parseExpression();
            Consume(Tokens::RParen, "Expected ')'");
            
            auto* then_block = parseBlock();
            BlockNode* else_block = nullptr;
            
            if (Peek().TYPE == Tokens::Else) {
                Advance();
                else_block = parseBlock();
            }
            return m_arena.alloc<IfStatementNode>(condition, then_block, else_block);
        };

        case Tokens::While: {
            Advance();  // Consume 'while'
            Consume(Tokens::LParen, "Expected '('");
            auto* condition = parseExpression();
            Consume(Tokens::RParen, "Expected ')'");

            auto body = parseBlock();
            return m_arena.alloc<WhileStatementNode>(condition, body);

        };
        

        default: {
            std::string msg = "Unexpected Token: '" + Peek().value + "' (Type " + std::to_string((int)Peek().TYPE) + ") at start of statement.";
            throw std::runtime_error(msg);
        }    
    }
}

ASTNode* Parser::parsePrimary() {    

    // 1. Handle Unary Minus:
    if (Peek().TYPE == Tokens::Minus) {
        Advance();
        auto to_ret = m_arena.alloc<UnaryExprNode>("-", parsePrimary());
        return to_ret;
    };

    // 2. Handle Parentheses:
    if (Peek().TYPE == Tokens::LParen) {
        Advance();
        auto to_ret = parseExpression();        // Make sure to catch everything in the brackets
        Consume(Tokens::RParen, "Expected ')'");
        return to_ret;
    };

    // 3. Base Cases:
    if (Peek().TYPE == Tokens::Number) {
        auto to_ret = m_arena.alloc<IntegerNode>(std::stoll(Advance().value));
        return to_ret;
    };

    if (Peek().TYPE == Tokens::Identifier) {
        return m_arena.alloc<IdentifierNode>(m_arena.dup_string(Advance().value));
    };

    throw std::runtime_error("Unexpected token in expression: " + Peek().value);
    
};

// 1. Level: Addition and Subtraction (Lowest Priority)
ASTNode* Parser::parseExpression() {
    auto left = parseTerm(); // Always try to parse a "Term" first

    while (Peek().TYPE == Tokens::Plus || Peek().TYPE == Tokens::Minus) {
        Token_s op = Advance();
        auto right = parseTerm();
        left = m_arena.alloc<BinaryExprNode>(m_arena.dup_string(op.value), left, right);    
    }
    return left;
}

// 2. Level: Multiplication and Division (Medium Priority)
ASTNode* Parser::parseTerm() {
    auto left = parsePrimary(); // Always try to parse a "Primary" (Number/Paren) first

    while (Peek().TYPE == Tokens::Multiply || Peek().TYPE == Tokens::Divide) {
        Token_s op = Advance();
        auto right = parsePrimary();
        left = m_arena.alloc<BinaryExprNode>(m_arena.dup_string(op.value), left, right);    
    }
    return left;
}

BlockNode *Parser::parseBlock() {
    Consume(Tokens::LBrace, "Expected '{'");
    auto *block = m_arena.alloc<BlockNode>();

    while (Peek().TYPE != Tokens::RBrace) {
        block->statements.push_back(parseStatement());
    };

    Consume(Tokens::RBrace, "Expected '}'");


    return block;
};