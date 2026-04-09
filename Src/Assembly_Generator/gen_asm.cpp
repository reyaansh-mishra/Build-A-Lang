#include "gen_asm.hpp"

std::string Generator::generate() {
    // --- Header ---
    m_output << "global _start\n_start:\n";
    m_output << "    push rbp\n";
    m_output << "    mov rbp, rsp\n";
    m_output << "    sub rsp, 128\n"; 

    // --- Process Statements ---
    auto *program = static_cast<ProgramNode*>(m_root);
    for (const auto& stmt : program->statements) {
            genStatement(stmt); // stmt is already a raw pointer now
        }

    // --- Global Exit (Fallthrough) ---
    m_output << "    add rsp, 128\n";
    m_output << "    pop rbp\n";
    m_output << "    mov rax, 60\n    mov rdi, 0\n    syscall\n\n";

    // --- THE PRINT HELPER ---
    m_output << "_print_int:\n"
         << "    mov rsi, rsp\n"
         << "    sub rsp, 32\n"       // Allocate 32 bytes on stack
         << "    mov r8, rsi\n"       // r8 will be our "moving" pointer
         << "    dec r8\n"
         << "    mov byte [r8], 10\n" // Put Newline (\n) at the very end
         << "    mov rbx, 10\n"
         << ".loop:\n"
         << "    xor rdx, rdx\n"
         << "    div rbx\n"           // RAX / 10 -> RAX (quotient), RDX (remainder)
         << "    add rdx, 48\n"       // Convert remainder to ASCII
         << "    dec r8\n"
         << "    mov [r8], dl\n"      // Store the digit
         << "    test rax, rax\n"
         << "    jnz .loop\n"         // Keep going until RAX is 0
         << "\n"
         << "    ; --- The Syscall ---\n"
         << "    mov rax, 1\n"         // syscall: write
         << "    mov rdi, 1\n"         // fd: stdout
         << "    mov rsi, r8\n"        // Start of our string (where r8 ended up)
         << "    mov rdx, rsp\n"       // Calculate length: original_rsp - r8
         << "    add rdx, 32\n"        // (Correcting for the sub rsp)
         << "    sub rdx, r8\n"        
         << "    syscall\n"
         << "\n"
         << "    add rsp, 32\n"        // Clean up buffer
         << "    ret\n";

    // --- Data Section ---
    m_output << "section .data\n"
             << "    minus_sign db '-'\n";

    return m_output.str();
}

void Generator::genExpr(const ASTNode* expr) {
    if (auto *num = dynamic_cast<const IntegerNode*>(expr)) {
        m_output << "    mov rax, " << num->val << "\n";
    } 

    else if (auto *id = dynamic_cast<const IdentifierNode*>(expr)) {
        // Check if the variable was actually declared
        if (m_vars.find(std::string(id->name)) == m_vars.end()) { 
            throw std::runtime_error("Variable not declared"); 
        }
        m_output << "    mov rax, [rbp - " << m_vars[std::string(id->name)] << "]\n";    }
    
    else if (auto *bin = dynamic_cast<const BinaryExprNode*>(expr)) {
        genExpr(bin->rhs);
        m_output << "    push rax\n"; 
        genExpr(bin->lhs);
        m_output << "    pop rbx\n";  
            
        if (bin->op == "+") {
            m_output << "    add rax, rbx\n";
        } else if (bin->op == "-") {
            m_output << "    sub rax, rbx\n"; // LHS is in RAX, RHS is in RBX
        }
        else if (bin->op == "*") {
            m_output << "    imul rax, rbx\n";
        } else if (bin->op == "/") {
            m_output << "    cqo\n";      // Clear RDX by sign-extending RAX
            m_output << "    idiv rbx\n";
        }
        else if (bin->op == "==") {
            m_output << "    cmp rax, rbx\n";  // Compare LHS (rax) and RHS (rbx)
            m_output << "    sete al\n";       // Set AL to 1 if equal, 0 otherwise
            m_output << "    movzx rax, al\n"; // Zero-extend AL to RAX (clean the rest)
        }
        else if (bin->op == "!=") {
            m_output << "    cmp rax, rbx\n";
            m_output << "    setne al\n";      // Set if Not Equal
            m_output << "    movzx rax, al\n";
        }
    }

    else if (auto *unary = dynamic_cast<const UnaryExprNode*>(expr)) {
        genExpr(unary->expr); // Get the value (e.g., y) into RAX
        if (unary->op == "-") {
            m_output << "    neg rax\n"; // Flips the sign: RAX = -RAX
        }
    }

    else if (auto *p = dynamic_cast<const PrintNode*>(expr)) {
        genExpr(p->expr);      // Puts the result to print into RAX
        m_output << "    call _print_int\n";
    }
}

void Generator::genStatement(const ASTNode* stmt) {
    if (auto *var = dynamic_cast<const VarDeclarationNode*>(stmt)) {
        genExpr(var->init_val); // Evaluate the value
        m_stack_ptr += 8;
        m_vars[std::string(var->name)] = m_stack_ptr;
        m_output << "    mov [rbp - " << m_stack_ptr << "], rax\n";
    }
    else if (auto *ret = dynamic_cast<const ReturnNode*>(stmt)) {
        genExpr(ret->expr);    // Put result in RAX
        m_output << "    mov rdi, rax\n"; // Move result to exit code register
        m_output << "    mov rax, 60\n    syscall\n";
    }

    else if (auto *p = dynamic_cast<const PrintNode*>(stmt)) {
        genExpr(p->expr);      // Puts result in RAX
        m_output << "    call _print_int\n";
    }
}