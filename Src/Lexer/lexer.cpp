#include "lexer.hpp"
#include <map>
#include <vector>
#include <iostream>

std::vector<Token_s> Lexer::tokenize() {
    std::vector<Token_s> tokens_vec;
    
    while (src.length() > pos) {
        char currentChar = src[pos];

        // 1. Whitespace
        if (std::isspace(currentChar)) {
            pos++; 
            continue;
        }

        // 2. Digits
        if (std::isdigit(currentChar)) {
            tokens_vec.push_back({Tokens::Number, readNumber()});
            continue; // readNumber moves 'pos', so we continue
        }

        // 3. Identifiers & Keywords
        if (std::isalpha(currentChar)) {
            std::string character = readIdentifier();
            if (keywords.count(character)) {
                tokens_vec.push_back({keywords.at(character), character});
            } else {
                tokens_vec.push_back({Tokens::Identifier, character});
            }
            continue; // readIdentifier moves 'pos'
        }

        // 4. Symbols (NO CONTINUE BEFORE THIS!)
        bool matched = true;
        switch (currentChar) {
            case '+': tokens_vec.push_back({Tokens::Plus, "+"}); break;
            case '-': tokens_vec.push_back({Tokens::Minus, "-"}); break;
            case '*': tokens_vec.push_back({Tokens::Multiply, "*"}); break;
            case '/': tokens_vec.push_back({Tokens::Divide, "/"}); break;

            case '=':
                if (peek(1) == '=') {
                    tokens_vec.push_back({Tokens::DoubleEqual, "=="});
                    pos++; // Consume the second '='
                } else {
                    tokens_vec.push_back({Tokens::Equal, "="});
                }
                break;
            
            case '!':
                if (peek(1) == '=') {
                    tokens_vec.push_back({Tokens::NotEqual, "!="});
                    pos++;
                } else {
                    tokens_vec.push_back({Tokens::Unknown, "!"});
                }
                break;

            case ';': tokens_vec.push_back({Tokens::Semicolon, ";"}); break;
            case '(': tokens_vec.push_back({Tokens::LParen, "("}); break;
            case ')': tokens_vec.push_back({Tokens::RParen, ")"}); break;
            case '{': tokens_vec.push_back({Tokens::LBrace, "{"}); break;
            case '}': tokens_vec.push_back({Tokens::RBrace, "}"}); break;

            default:
                tokens_vec.push_back({Tokens::Unknown, std::string(1, currentChar)});
                matched = false;
                break;
        }

        if (!matched) {
            std::cerr << "Lexer Warning: Character '" << currentChar << "' not recognized." << std::endl;
        }

        pos++; // Move to next character for symbols
    }

    tokens_vec.push_back({Tokens::Eof, "EOF"});
    return tokens_vec;
}


char Lexer::peek(size_t offset = 0) const {
    if (pos + offset >= src.length()) return '\0';
    return src[pos + offset];
}