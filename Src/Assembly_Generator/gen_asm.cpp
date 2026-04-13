#include "gen_asm.hpp"

// Emitter helper to prevent missing newlines
void Generator::emit(std::string instr) {
    m_output << "    " << instr << "\n";
}

std::string Generator::generate() {
    m_output << "global _start\n_start:\n";
    emit("push rbp");
    emit("mov rbp, rsp");
    emit("sub rsp, 128"); 

    // --- Process Statements ---
    auto *program = static_cast<ProgramNode*>(m_root);
    for (const auto& stmt : program->statements) {
        genStatement(stmt);
    }

    emit("add rsp, 128");
    emit("pop rbp");
    emit("mov rax, 60");
    emit("mov rdi, 0");
    emit("syscall\n\n");

    // --- Minimalist, Non-Destructive Print ---
    m_output << "_print_int:\n";
    emit("push rbx");           // Save RBX
    emit("mov rbx, rax");       // Copy value to RBX (preserve RAX)
    emit("add rbx, 48");        // ASCII conversion
    emit("push rbx");           // Put char on stack
    emit("mov rax, 1");         // sys_write
    emit("mov rdi, 1");
    emit("mov rsi, rsp");
    emit("mov rdx, 1");
    emit("syscall");
    emit("pop rbx");            // Clear char
    emit("pop rbx");            // Restore original RBX
    emit("ret");

    m_output << "section .data\n" << "    char_a db 'A'\n";
    return m_output.str();
}

void Generator::genStatement(const ASTNode* stmt) {
    // 1. Variable Declaration
    if (auto *var = dynamic_cast<const VarDeclarationNode*>(stmt)) {
        genExpr(var->init_val);
        m_stack_ptr += 8;
        m_vars[std::string(var->name)] = m_stack_ptr;
        emit("mov [rbp - " + std::to_string(m_stack_ptr) + "], rax");
    }
    // 2. Assignment
    else if (auto *asgn = dynamic_cast<const AssignmentNode*>(stmt)) {
        genExpr(asgn->expr);
        std::string varName(asgn->name);
        emit("mov [rbp - " + std::to_string(m_vars.at(varName)) + "], rax");
    }
    // 3. Print Statement
    else if (auto *p = dynamic_cast<const PrintNode*>(stmt)) {
        genExpr(p->expr);
        emit("call _print_int");
    }
    // 4. If Statement
    else if (auto *if_stmt = dynamic_cast<const IfStatementNode*>(stmt)) {
        int id = m_label_count++;
        std::string label_else = "L_else_" + std::to_string(id);
        std::string label_end = "L_end_" + std::to_string(id);
        int snapshot = m_stack_ptr; // Snapshot before entering IF

        genExpr(if_stmt->condition);
        emit("cmp rax, 0");
        emit("je " + (if_stmt->else_block ? label_else : label_end));

        m_output << "; DEBUG: i is at " << m_vars.at("i") << "\n";

        // Then block
        for (auto* s : if_stmt->then_block->statements) genStatement(s);
        emit("jmp " + label_end);

        // Else block
        if (if_stmt->else_block) {
            m_output << label_else << ":\n";
            m_stack_ptr = snapshot; // Reset for else block
            for (auto* s : if_stmt->else_block->statements) genStatement(s);
        }

        m_output << label_end << ":\n";
        m_stack_ptr = snapshot; // Restore after IF/ELSE
    }
    // 5. While Statement
    else if (auto *while_node = dynamic_cast<const WhileStatementNode*>(stmt)) {
        int id = m_label_count++;
        std::string start = "L_while_" + std::to_string(id);
        std::string end = "L_end_" + std::to_string(id);
        int snapshot = m_stack_ptr;

        m_output << start << ":\n";
        genExpr(while_node->condition);
        emit("cmp rax, 0");
        emit("je " + end);

        for (auto s : while_node->body->statements) genStatement(s);

        m_stack_ptr = snapshot;
        emit("mov rsp, rbp");
        emit("sub rsp, 128");
        emit("jmp " + start);
        m_output << end << ":\n";
    }

    else if (auto *sys = dynamic_cast<const SyscallNode*>(stmt)) {
        // 1. Calculate the syscall number (the first argument)
        genExpr(sys->sc); 
        
        // 2. The result of the expression is now in RAX. 
        // Move it to a temporary register (or push/pop) 
        // so we can use RAX for other things.
        emit("push rax"); 

        // 3. Evaluate all other arguments and put them in the required registers
        for (size_t i = 0; i < sys->args.size(); ++i) {
            genExpr(sys->args[i]);
            emit("mov " + reg_map[i] + ", rax");
        }

        // 4. Pop the syscall number back into RAX
        emit("pop rax"); 
        emit("syscall");
    };
};

void Generator::genExpr(const ASTNode* expr) {
    if (auto *num = dynamic_cast<const IntegerNode*>(expr)) {
        emit("mov rax, " + std::to_string(num->val));
    } 
    else if (auto *id = dynamic_cast<const IdentifierNode*>(expr)) {
        // Explicitly convert string_view to std::string
        std::string varName(id->name); 
        if (m_vars.find(varName) == m_vars.end()) {
            throw std::runtime_error("Variable not declared: " + varName);
        }
        emit("mov rax, [rbp - " + std::to_string(m_vars.at(varName)) + "]");
    }
    else if (auto *bin = dynamic_cast<const BinaryExprNode*>(expr)) {
        genExpr(bin->rhs);
        emit("push rax");
        genExpr(bin->lhs);
        emit("pop rbx");
        if (bin->op == "+") emit("add rax, rbx");
        else if (bin->op == "<") {
            // 1. RHS is already in RAX, pushed to stack, then popped to RBX
            // 2. LHS is in RAX
            // 3. Compare them!
            emit("cmp rax, rbx");      // Compare LHS (RAX) and RHS (RBX)
            emit("setl al");           // 'l' = less than
            emit("movzx rax, al");     // Set RAX to 1 (true) or 0 (false)
        }
    }
    else if (auto *p = dynamic_cast<const PrintNode*>(expr)) {
        genExpr(p->expr);
        emit("push rax");
        emit("call _print_int");
        emit("pop rax");
    }
}