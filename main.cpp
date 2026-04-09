#include <iostream>
#include <vector>
#include <fstream>
#include "lexer.hpp"
#include "parser.hpp"
#include "gen_asm.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source.lpp>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream input_file(filename);

    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    // Read the entire file into a string buffer
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    std::string source = buffer.str();

    try {

        std::cout << "Source: " << source << "\n";

        // 2. Tokenize
        Lexer lexer(source);
        std::vector<Token_s> tokens = lexer.tokenize();

 
        // DEBUG PRINT
        for (const auto& t : tokens) {
            std::cout << "Token: " << (int)t.TYPE << " | Value: '" << t.value << "'" << std::endl;
        }

        // 3. Parse (The Walk)
        Arena arena; // Create it here so it lives for the whole program
        Parser parser(std::move(tokens), arena); 
        
        
        auto tree = parser.parse();
        if (tree) {
            std::cout << tree->dump() << std::endl;
        } else {
            std::cout << "Empty Program." << std::endl;
        }


        std::cout << "\nBuild-A-Lang: Parse successful." << "\n";

        Generator gen(std::move(tree));
        std::string asm_code = gen.generate();

        std::ofstream file("out.s");
        file << asm_code;
        file.close();

        std::cout << "Assembly generated: out.s" << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "\nLang Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}