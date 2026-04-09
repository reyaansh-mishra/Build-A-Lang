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
     << "    test rax, rax\n"
     << "    jns .P1\n"           // If positive, jump to P1
     << "    push rax\n"          // Save RAX
     << "    mov rax, 1\n"        // sys_write
     << "    mov rdi, 1\n"        // stdout
     << "    mov rsi, minus_sign\n"
     << "    mov rdx, 1\n"
     << "    syscall\n"
     << "    pop rax\n"
     << "    neg rax\n"           // Convert to positive
     << ".P1:\n"
     << "    mov rbx, 10\n"
     << "    push 10\n"           // Push newline as string terminator
     << "    mov r12, 1\n"        // r12 will track our string length
     << ".loop:\n"
     << "    xor rdx, rdx\n"
     << "    div rbx\n"
     << "    add rdx, 48\n"       // Convert to ASCII
     << "    push rdx\n"          // Push digit to stack
     << "    inc r12\n"
     << "    test rax, rax\n"
     << "    jnz .loop\n"
     << ".print_loop:\n"
     << "    mov rax, 1\n"        // sys_write
     << "    mov rdi, 1\n"        // stdout
     << "    mov rsi, rsp\n"      // Top of stack is our character
     << "    mov rdx, 1\n"        // Just print one char at a time
     << "    syscall\n"
     << "    pop rax\n"           // Clean that char off the stack
     << "    dec r12\n"
     << "    jnz .print_loop\n"
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

    if (auto *asgn = dynamic_cast<const AssignmentNode*>(stmt)) {
        genExpr(asgn->expr); // Puts new value in RAX
        
        // Look up where 'x' was stored during its 'let' declaration
        if (m_vars.find(std::string(asgn->name)) == m_vars.end()) {
            throw std::runtime_error("Undeclared variable: " + std::string(asgn->name));
        }
        
        int offset = m_vars[std::string(asgn->name)];
        m_output << "    mov [rbp - " << offset << "], rax\n";
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

    if (auto *if_stmt = dynamic_cast<const IfStatementNode*>(stmt)) {
        int label_id = m_label_count++;
        std::string label_else = ".L_else_" + std::to_string(label_id);
        std::string label_end = ".L_end_" + std::to_string(label_id);

        // 1. Evaluate condition (result in RAX)
        genExpr(if_stmt->condition);
        
        // 2. If RAX is 0 (false), jump to else (or end if no else)
        m_output << "    cmp rax, 0\n";
        m_output << "    je " << (if_stmt->else_block ? label_else : label_end) << "\n";

        // 3. Generate 'then' block
        for (auto* s : if_stmt->then_block->statements) genStatement(s);
        m_output << "    jmp " << label_end << "\n";

        // 4. Generate 'else' block if it exists
        if (if_stmt->else_block) {
            m_output << label_else << ":\n";
            for (auto* s : if_stmt->else_block->statements) genStatement(s);
        }

        // 5. Final label
        m_output << label_end << ":\n";
    }

    if (auto *while_node = dynamic_cast<const WhileStatementNode*>(stmt)) {
        int label_id = m_label_count++;
        std::string start_label = ".L_while_start_" + std::to_string(label_id);
        std::string end_label = ".L_while_end_" + std::to_string(label_id);

        m_output << start_label << ":\n"; // --- The Re-entry Point ---
        genExpr(while_node->condition);
        m_output << "    cmp rax, 0\n";
        m_output << "    je " << end_label << "\n"; // Exit if false

        for (auto s : while_node->body->statements) genStatement(s);

        m_output << "    jmp " << start_label << "\n"; // --- The Loop Back ---
        m_output << end_label << ":\n";
    }
}