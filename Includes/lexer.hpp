#pragma once
#include <cctype>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

enum class Tokens {
    // Keywords:
    Func, Ret, Int, Let, Print, If, Else, While, Syscall,

    // Literals:
    Identifier, Number,

    // Operators:
    Plus, Minus, Multiply, Divide, Equal, NotEqual, DoubleEqual, Greater, GreaterEqual, Less, LessEqual,
    
    // Symbols:
    Semicolon, LParen, RParen, LBrace, RBrace, Comma,

    // Meta:
    Eof, Unknown
};


struct Token_s {
    Tokens TYPE;
    std::string value;
};


// ---------------------------------------------------

class Lexer {
    public:
        Lexer(std::string source) : src(source), pos(0) {}                  // Reset src to source and set pos = 0; pos is used as a virtual cursor
        std::vector<Token_s> tokenize();

    private:
        std::string src;
        size_t pos;

        const std::map<std::string, Tokens> keywords = {
            {"let", Tokens::Let},
            {"ret", Tokens::Ret},
            {"print", Tokens::Print},
            {"func", Tokens::Func},
            {"if", Tokens::If},
            {"else", Tokens::Else},
            {"while", Tokens::While},
            {"syscall", Tokens::Syscall}
        };

        std::string readNumber() {
            size_t start = pos;
            while (src.length() > pos && isdigit(src[pos])) pos++;
            return src.substr(start, pos - start);
        };

        std::string readIdentifier() {
            size_t start = pos;
            while (pos < src.length() && (isalnum(src[pos]) || src[pos] == '_')) pos++;
            return src.substr(start, pos - start);
        };

        char peek(size_t offset) const;
};