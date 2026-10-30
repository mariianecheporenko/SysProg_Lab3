#ifndef MINI_C_AST_H
#define MINI_C_AST_H

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using Environment = std::unordered_map<std::string, double>;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual double evaluate(Environment& environment) const = 0;
    double evaluate() const;
    virtual void generatePython(std::ostream& out, int indent = 0) const = 0;
    std::string generatePython() const;
    virtual std::string name() const = 0;
    virtual std::vector<const ASTNode*> children() const;
    std::string toJson() const;
    void saveToJson(const std::string& filename) const;
};

class Expr : public ASTNode {
public:
    void generatePython(std::ostream& out, int indent = 0) const override;
};

class IntegerExpr final : public Expr {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    explicit IntegerExpr(int value);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
private:
    int value_;
};

class VariableExpr final : public Expr {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    explicit VariableExpr(std::string name);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
private:
    std::string name_;
};

class UnaryExpr final : public Expr {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    UnaryExpr(std::string op, std::unique_ptr<Expr> operand);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::string op_;
    std::unique_ptr<Expr> operand_;
};

class BinaryExpr final : public Expr {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    BinaryExpr(std::string op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::string op_;
    std::unique_ptr<Expr> left_;
    std::unique_ptr<Expr> right_;
};

class Stmt : public ASTNode {};

class BlockStmt final : public Stmt {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    void add(std::unique_ptr<Stmt> statement);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::vector<std::unique_ptr<Stmt>> statements_;
};

class DeclStmt final : public Stmt {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    DeclStmt(std::string name, std::unique_ptr<Expr> initializer);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::string name_;
    std::unique_ptr<Expr> initializer_;
};

class AssignStmt final : public Stmt {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    AssignStmt(std::string name, std::unique_ptr<Expr> value);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::string name_;
    std::unique_ptr<Expr> value_;
};

class IfStmt final : public Stmt {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch = nullptr);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> thenBranch_;
    std::unique_ptr<Stmt> elseBranch_;
};

class WhileStmt final : public Stmt {
public:
    using ASTNode::evaluate;
    using ASTNode::generatePython;
    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body);
    double evaluate(Environment& environment) const override;
    void generatePython(std::ostream& out, int indent = 0) const override;
    std::string name() const override;
    std::vector<const ASTNode*> children() const override;
private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

#endif
