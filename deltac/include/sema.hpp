#include "expression.hpp"
#include "declaration.hpp"
#include "astcontext.hpp"

#include "llvm/ADT/ArrayRef.h"

namespace deltac {

class Sema;

class TypeBuilder {
public:
    TypeBuilder(Sema& action);

    bool add_ptr(bool constness = false);
    // bool add_array(Expr* size);
    // bool add_array_ref(bool constness = false);

    bool finalize(const Token& tok, bool constness = false);
    
    bool finalize(llvm::ArrayRef<QualType> param_ty, QualType ret_ty, util::use_move_t = util::use_move);

    bool has_errored() const { return errored; }
    bool has_finalized() const { return finalized; }

    bool reset();
    TypeResult release();
    TypeResult get();

private:
    void reset_internal();

private:
    bool errored = false;
    bool finalized = false;
    QualType res;
    QualType* base;
    Sema& action;
};

class Sema {
public:
    Sema(ASTContext& context) : context(context) {}
    
    const ASTContext& ast_context() const { return context; }

    ExprResult act_on_int_literal(const Token& tok, u8 posix, QualType* ty);
    ExprResult act_on_unary_expr(UnaryOp, Expr* expr);

    RawTypeResult act_on_raw_type(const Token& tok);

private:
    Expr* add_integer_promotion(Expr* expr);

    Type* new_type_from_tok(const Token& token);
    Type* new_function_ty(llvm::ArrayRef<QualType> param_ty, QualType ret_ty);

private:
    friend class TypeBuilder;

    ASTContext& context;
};

}
