#pragma once

#include "token.hpp"
#include "tokentype.hpp"
#include "typeinfo.hpp"
#include "utils.hpp"
#include "typeinfo.hpp"
#include "operators.hpp"
#include "ownership.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <cstddef>
#include <optional>
#include <utility>
#include <string>
#include <string_view>
// #include <format>

namespace deltac {

// there should not be something like a const Expr*
// constness is enforced by getter/setters
class Expr {
public:
    enum ValCate {
        RValue,
        LValue,
        Unclassified,
    };

public:
    Expr(QualType type, ValCate valcate) : exprtype(std::move(type)), valcate(valcate) {}
    Expr(const Expr&) = delete;
    Expr(Expr&&) = delete;
    virtual ~Expr() = 0;

    bool is_rval() const {
        return valcate == RValue;
    }

    void set_rval() {
        valcate = RValue;
    }

    bool is_lval() {
        return valcate == LValue;
    }

    void set_lval() {
        valcate = LValue;
    }

    bool is_unclassified() {
        return valcate == Unclassified;
    }

    void set_unclassified() {
        valcate = Unclassified;
    }

    void value(ValCate val) {
        valcate = val;
    }

    ValCate value() {
        return valcate;
    }

    bool can_modify() {
        return exprtype.is_mutable();
    }

    QualType& type() {
        return exprtype;
    }

private:
    QualType exprtype;
    ValCate valcate = Unclassified;
};

inline Expr::~Expr() = default;

class BinaryExpr : public Expr {
public:
    BinaryExpr(QualType type, Expr::ValCate valcate, Expr* lhs, BinaryOp op, Expr* rhs) : 
        Expr(std::move(type), valcate), exprs { lhs, rhs }, op(op) {}

    ~BinaryExpr() override { delete exprs[LHS]; delete exprs[RHS]; };

    Expr* lhs() const { return exprs[LHS]; }
    void lhs(Expr* expr) { exprs[LHS] = expr; }
    Expr* rhs() const { return exprs[RHS]; }
    void rhs(Expr* expr) { exprs[RHS] = expr; }

    BinaryOp op_code() { return op; }

private:
    enum { LHS, RHS, EXPR_END };
    Expr* exprs[EXPR_END];
    BinaryOp op;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(QualType type, ValCate valcate, UnaryOp op, Expr* expr) :
        Expr(std::move(type), valcate), op(op), mainexpr(expr) {}

    ~UnaryExpr() override { delete mainexpr; };

    Expr* expr() const { return mainexpr; }
    void expr(Expr* expr) { mainexpr = expr; }

private:
    UnaryOp op;
    Expr* mainexpr;
};

class PostfixExpr : public Expr {
public:
    PostfixExpr(QualType type, ValCate valcate, Expr* expr) :
        Expr(std::move(type), valcate), mainexpr(expr) {}
    ~PostfixExpr() override = 0;

    Expr* expr() const { return mainexpr; }
    void expr(Expr* expr) { mainexpr = expr; }

private:
    Expr* mainexpr;
};

inline PostfixExpr::~PostfixExpr() = default;

class CallExpr : public PostfixExpr {
public:
    CallExpr(QualType type, ValCate valcate, Expr* expr, llvm::ArrayRef<Expr*> arguments) : 
        PostfixExpr(std::move(type), valcate, expr), args(arguments) {}

    ~CallExpr() override {
        util::cleanup_ptrs(args.begin(), args.end());
    }

private:
    llvm::SmallVector<Expr*> args;
};

class IndexExpr : public PostfixExpr {
public:
    IndexExpr(QualType type, ValCate valcate, Expr* expr, Expr* index) : 
        PostfixExpr(std::move(type), valcate, expr), index(index) {}

    ~IndexExpr() override { delete index; }

private:
    Expr* index;
};


class CastExpr : public Expr { // currently only consists of implicit casts
public:
    enum CastKind {
        NoOp,
        LValueToRValue,
        BitCast,
        FnToPtrDecay,
        IntCast,
        FloatCast,
        FloatToBool,
        IntToFloat,
        IntToBool,
        FloatToInt,
        PtrToBool,
    };

public:
    CastExpr(Expr* e, QualType ty, CastKind op, bool is_part_of_explcast) : 
        Expr(std::move(ty), RValue), expr(e), 
        kind(op), is_part_of_explcast(is_part_of_explcast) {}

    ~CastExpr() override { delete expr; }

    CastKind op_code() const { return kind; }

    Expr* castee() const { return expr; }

    bool part_of_explcast() const { return is_part_of_explcast; }

    CastKind cast_kind() const { return kind; }

    std::pair<QualType*, QualType*> cast_types() {
        return { &castee()->type(), &this->type() };
    }

private:
    Expr* expr;
    CastKind kind;
    bool is_part_of_explcast;
};

class ImplicitCastExpr : public CastExpr {
public:
    ImplicitCastExpr(Expr* expr, QualType t, CastKind op) : CastExpr(expr, std::move(t), op, true) {}
};

class ExplicitCastExpr : public CastExpr {
public:
    ExplicitCastExpr(Expr* expr, QualType t, CastKind op) : CastExpr(expr, std::move(t), op, false) {}
};

class IdExpr : public Expr {
public:
    IdExpr(QualType type, std::string_view id) : 
        Expr(std::move(type), ValCate::LValue), identifier(id) {}
    ~IdExpr() override = default;

private:
    std::string identifier;
};

class IntLiteralExpr : public Expr {
public:
    IntLiteralExpr(
        QualType type, 
        u32 numbits, 
        std::string_view literalrepr, 
        bool is_unsigned = true,
        u8 radix = 10
    ) : 
        Expr(std::move(type), RValue), data(llvm::APInt(numbits, literalrepr, radix), is_unsigned) {}

    IntLiteralExpr(QualType type, llvm::APSInt data, bool is_unsigned = true) : 
        Expr(std::move(type), RValue), data(std::move(data), is_unsigned) {}
    
    ~IntLiteralExpr() override = default;

private:
    llvm::APSInt data;
};

class ParenExpr : public Expr {
public:
    ParenExpr(Expr* expr) : Expr(expr->type(), expr->value()), expr(expr) {}

    ~ParenExpr() override { delete expr; }

private:
    Expr* expr;
};

class AssignExpr : public Expr {
public:
    AssignExpr(Expr* lhs, AssignOp op, Expr* rhs) : 
        Expr(rhs->type(), LValue), exprs { lhs, rhs }, op(op) {}

    ~AssignExpr() { delete exprs[LHS]; delete exprs[RHS]; }

private:
    enum { LHS, RHS, EXPR_END };
    Expr* exprs[EXPR_END];
    AssignOp op;
};

}

/*

template <typename FormatContext>
std::string format_expr(const Expr& expr, FormatContext& ctx, int indent = 0) {
    std::string indent_str = std::string(indent*4, ' ');
    if (auto* literal = dynamic_cast<const LiteralExpr*>(&expr)) {
        return std::format("{}{}", indent_str, *literal);
    } else if (auto* id = dynamic_cast<const IdExpr*>(&expr)) {
        return std::format("{}{}", indent_str, *id);
    } else if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr)) {
        return std::format("{0}{1}{2}{3}{0}}}\n", indent_str, *bin, format_expr(*bin->lhs, ctx, indent + 1),
                           format_expr(*bin->rhs, ctx, indent + 1));
    } else if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr)) {
        return std::format("{0}{1}{2}{0}}}\n", indent_str, *unary, format_expr(*unary->expr, ctx, indent + 1));
    } else if (auto* cast = dynamic_cast<const CastExpr*>(&expr)) {
        return std::format("{0}{1}{2}{0}}}\n", indent_str, *cast, format_expr(*cast->expr, ctx, indent + 1));
    }
    return "unknown_expr";
}

template<>
struct std::formatter<BinaryExpr> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const BinaryExpr& bin_expr, FormatContext& ctx) -> decltype(ctx.out()) {
        return std::format_to(ctx.out(), "BinaryExpr: some bin op {{\n");
    }
};

template<>
struct std::formatter<IdExpr> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const IdExpr& expr, FormatContext& ctx) {
        std::string data_str = "some id";
        return std::format_to(ctx.out(), "IdExpr: {}\n", data_str);
    }
};

template<>
struct std::formatter<LiteralExpr> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const LiteralExpr& expr, FormatContext& ctx) {
        std::string data_str = "some literal data";
        return std::format_to(ctx.out(), "LiteralExpr: {}\n", data_str);
    }
};

template<>
struct std::formatter<CastExpr> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const CastExpr& expr, FormatContext& ctx) {
        return std::format_to(ctx.out(), "CastExpr: {{\n");
    }
};

template<>
struct std::formatter<UnaryExpr> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const UnaryExpr& expr, FormatContext& ctx) {
        std::string op_str;
        switch (expr.op) {
            case UnaryOp::Plus:
                op_str = "Plus '+'";
                break;
            case UnaryOp::Minus:
                op_str = "Minus '-'";
                break;
            case UnaryOp::Not:
                op_str = "Not '!'";
                break;
            case UnaryOp::BitwiseNot:
                op_str = "BitwiseNot '~'";
                break;
            case UnaryOp::Deref:
                op_str = "Deref '*'";
                break;
            case UnaryOp::AddressOf:
                op_str = "AddressOf '&'";
                break;
            default:
                op_str = "Unknown";
        }
        return std::format_to(ctx.out(), "UnaryExpr: {}\n", op_str);
    }
};

*/
