#include "ast.h"

#include <stdexcept>
#include <ostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <utility>

namespace {
void indent(std::ostream& out, int level) { for (int i = 0; i < level; ++i) out << "    "; }
std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                const char* hex = "0123456789abcdef";
                out << "\\u00" << hex[ch >> 4] << hex[ch & 0x0f];
            } else out << static_cast<char>(ch);
        }
    }
    return out.str();
}
void writeJson(const ASTNode& node, std::ostream& out) {
    out << "{\"name\":\"" << jsonEscape(node.name()) << "\",\"children\":[";
    const auto descendants = node.children();
    for (std::size_t i = 0; i < descendants.size(); ++i) {
        if (i) out << ',';
        writeJson(*descendants[i], out);
    }
    out << "]}";
}
}

double ASTNode::evaluate() const { Environment environment; return evaluate(environment); }
std::string ASTNode::generatePython() const { std::ostringstream out; generatePython(out); return out.str(); }
std::vector<const ASTNode*> ASTNode::children() const { return {}; }
std::string ASTNode::toJson() const { std::ostringstream out; writeJson(*this, out); return out.str(); }
void ASTNode::saveToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("cannot open JSON file: " + filename);
    file << toJson() << '\n';
    if (!file) throw std::runtime_error("failed writing JSON file: " + filename);
}
void Expr::generatePython(std::ostream&, int) const {}

IntegerExpr::IntegerExpr(int value) : value_(value) {}
double IntegerExpr::evaluate(Environment&) const { return static_cast<double>(value_); }
void IntegerExpr::generatePython(std::ostream& out, int) const { out << value_; }
std::string IntegerExpr::name() const { return "IntegerExpr(" + std::to_string(value_) + ")"; }

VariableExpr::VariableExpr(std::string name) : name_(std::move(name)) {}
double VariableExpr::evaluate(Environment& env) const { return env[name_]; }
void VariableExpr::generatePython(std::ostream& out, int) const { out << name_; }
std::string VariableExpr::name() const { return "VariableExpr(" + name_ + ")"; }

UnaryExpr::UnaryExpr(std::string op, std::unique_ptr<Expr> operand)
    : op_(std::move(op)), operand_(std::move(operand)) {}
double UnaryExpr::evaluate(Environment& env) const {
    const double value = operand_->evaluate(env);
    if (op_ == "-") return -value;
    if (op_ == "+") return value;
    if (op_ == "!") return !value;
    throw std::runtime_error("unsupported unary operator: " + op_);
}
void UnaryExpr::generatePython(std::ostream& out, int) const {
    out << (op_ == "!" ? "not " : op_); operand_->generatePython(out);
}
std::string UnaryExpr::name() const { return "UnaryExpr(" + op_ + ")"; }
std::vector<const ASTNode*> UnaryExpr::children() const { return {operand_.get()}; }

BinaryExpr::BinaryExpr(std::string op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
    : op_(std::move(op)), left_(std::move(left)), right_(std::move(right)) {}
double BinaryExpr::evaluate(Environment& env) const {
    const double a = left_->evaluate(env);
    if (op_ == "&&") return a && right_->evaluate(env);
    if (op_ == "||") return a || right_->evaluate(env);
    const double b = right_->evaluate(env);
    if (op_ == "+") return a + b;
    if (op_ == "-") return a - b;
    if (op_ == "*") return a * b;
    if (op_ == "/") return b != 0.0 ? a / b : 0.0;
    if (op_ == "%") return b != 0.0 ? std::fmod(a, b) : 0.0;
    if (op_ == "<") return a < b;
    if (op_ == "<=") return a <= b;
    if (op_ == ">") return a > b;
    if (op_ == ">=") return a >= b;
    if (op_ == "==") return a == b;
    if (op_ == "!=") return a != b;
    throw std::runtime_error("unsupported binary operator: " + op_);
}
void BinaryExpr::generatePython(std::ostream& out, int) const {
    const std::string pythonOp = op_ == "&&" ? "and" : (op_ == "||" ? "or" : op_);
    out << '('; left_->generatePython(out); out << ' ' << pythonOp << ' '; right_->generatePython(out); out << ')';
}
std::string BinaryExpr::name() const { return "BinaryExpr(" + op_ + ")"; }
std::vector<const ASTNode*> BinaryExpr::children() const { return {left_.get(), right_.get()}; }

void BlockStmt::add(std::unique_ptr<Stmt> statement) { statements_.push_back(std::move(statement)); }
double BlockStmt::evaluate(Environment& env) const { double result = 0; for (const auto& s : statements_) result = s->evaluate(env); return result; }
std::string BlockStmt::name() const { return "BlockStmt"; }
std::vector<const ASTNode*> BlockStmt::children() const {
    std::vector<const ASTNode*> result;
    result.reserve(statements_.size());
    for (const auto& statement : statements_) result.push_back(statement.get());
    return result;
}
void BlockStmt::generatePython(std::ostream& out, int level) const {
    if (statements_.empty()) { indent(out, level); out << "pass\n"; return; }
    for (const auto& s : statements_) s->generatePython(out, level);
}

DeclStmt::DeclStmt(std::string name, std::unique_ptr<Expr> initializer)
    : name_(std::move(name)), initializer_(std::move(initializer)) {}
double DeclStmt::evaluate(Environment& env) const {
    if (env.count(name_)) throw std::runtime_error("variable already declared: " + name_);
    env[name_] = initializer_ ? initializer_->evaluate(env) : 0; return env[name_];
}
void DeclStmt::generatePython(std::ostream& out, int level) const {
    indent(out, level); out << name_ << " = "; if (initializer_) initializer_->generatePython(out); else out << '0'; out << '\n';
}
std::string DeclStmt::name() const { return "DeclStmt(" + name_ + ")"; }
std::vector<const ASTNode*> DeclStmt::children() const { return initializer_ ? std::vector<const ASTNode*>{initializer_.get()} : std::vector<const ASTNode*>{}; }

AssignStmt::AssignStmt(std::string name, std::unique_ptr<Expr> value) : name_(std::move(name)), value_(std::move(value)) {}
double AssignStmt::evaluate(Environment& env) const {
    return env[name_] = value_->evaluate(env);
}
void AssignStmt::generatePython(std::ostream& out, int level) const { indent(out, level); out << name_ << " = "; value_->generatePython(out); out << '\n'; }
std::string AssignStmt::name() const { return "AssignStmt(" + name_ + ")"; }
std::vector<const ASTNode*> AssignStmt::children() const { return {value_.get()}; }

IfStmt::IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch)
    : condition_(std::move(condition)), thenBranch_(std::move(thenBranch)), elseBranch_(std::move(elseBranch)) {}
double IfStmt::evaluate(Environment& env) const {
    if (condition_->evaluate(env)) thenBranch_->evaluate(env);
    else if (elseBranch_) elseBranch_->evaluate(env);
    return 0.0;
}
void IfStmt::generatePython(std::ostream& out, int level) const {
    indent(out, level); out << "if "; condition_->generatePython(out); out << ":\n";
    thenBranch_->generatePython(out, level + 1);
    if (elseBranch_) { indent(out, level); out << "else:\n"; elseBranch_->generatePython(out, level + 1); }
}
std::string IfStmt::name() const { return "IfStmt"; }
std::vector<const ASTNode*> IfStmt::children() const {
    std::vector<const ASTNode*> result{condition_.get(), thenBranch_.get()};
    if (elseBranch_) result.push_back(elseBranch_.get());
    return result;
}

WhileStmt::WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
    : condition_(std::move(condition)), body_(std::move(body)) {}
double WhileStmt::evaluate(Environment& env) const { while (condition_->evaluate(env)) body_->evaluate(env); return 0.0; }
void WhileStmt::generatePython(std::ostream& out, int level) const {
    indent(out, level); out << "while "; condition_->generatePython(out); out << ":\n"; body_->generatePython(out, level + 1);
}
std::string WhileStmt::name() const { return "WhileStmt"; }
std::vector<const ASTNode*> WhileStmt::children() const { return {condition_.get(), body_.get()}; }
