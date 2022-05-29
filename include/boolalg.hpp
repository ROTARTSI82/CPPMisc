#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class Expr {
public:
    virtual std::string to_string() {
        return "<expr>";
    }
};

typedef std::shared_ptr<Expr> PExpr;

struct Value {
    bool value{};
    std::string name;
};

class Constant : public Expr {
public:
    bool value{};

    Constant() = default;
    explicit Constant(bool x) : value(x) {}

    std::string to_string() override {
        return value ? "TRUE" : "FALSE";
    }
};

class Variable : public Expr {
public:
    std::shared_ptr<Value> value;
    bool inv = false;

    [[nodiscard]] std::shared_ptr<Variable> inverse() const {
        std::shared_ptr<Variable> ret = std::make_shared<Variable>();
        // std::cout << "INVERT " << value->name << '\n';

        ret->value = value;
        ret->inv = !inv;
        return ret;
    }

    std::string to_string() override {
        return (inv ? "!" : "") + value->name;
    }
};

PExpr new_var(const std::string &name) {
    std::shared_ptr<Variable> ret = std::make_shared<Variable>();
    std::shared_ptr<Value> val = std::make_shared<Value>();
    val->value = false;
    val->name = name;

    ret->value = val;
    return std::dynamic_pointer_cast<Expr>(ret);
}

class Not : public Expr {
public:
    PExpr next;

    Not() = default;
    explicit Not(PExpr exp) : next(std::move(exp)) {}

    std::string to_string() override {
        return "!(" + next->to_string() + ")";
    }
};

class Or : public Expr {
public:
    std::vector<PExpr> children;

    Or() = default;
    explicit Or(std::vector<PExpr> vec) : children(std::move(vec)) {}

    std::string to_string() override {
        std::string ret = "(";
        for (std::size_t i = 0; i < children.size(); i++) {
            if (i > 0)
                ret += " + ";
            ret += children[i]->to_string();
        }
        return ret + ")";
    }
};

class And : public Expr {
public:
    std::vector<PExpr> children;

    And() = default;
    explicit And(std::vector<PExpr> vec) : children(std::move(vec)) {}

    std::string to_string() override {
        std::string ret = "(";
        for (std::size_t i = 0; i < children.size(); i++) {
            if (i > 0)
                ret += " * ";
            ret += children[i]->to_string();
        }
        return ret + ")";
    }
};

PExpr expand(const PExpr &expr);

PExpr not_simplify(const std::shared_ptr<Not> &nexpr) {

    // apply demorgan's to OR
    std::shared_ptr<Or> pOr = std::dynamic_pointer_cast<Or>(nexpr->next);
    if (pOr) {
        std::shared_ptr<And> ret = std::make_shared<And>();
        ret->children = pOr->children;// make copy
        for (auto &i: ret->children)
            i = not_simplify(std::make_shared<Not>(i));

        return ret;
    }

    // apply demorgan's to AND
    std::shared_ptr<And> pAnd = std::dynamic_pointer_cast<And>(nexpr->next);
    if (pAnd) {
        std::shared_ptr<Or> ret = std::make_shared<Or>();
        ret->children = pAnd->children;// make copy
        for (auto &i: ret->children)
            i = not_simplify(std::make_shared<Not>(i));

        return ret;
    }

    // variables become their inverses
    std::shared_ptr<Variable> pVar = std::dynamic_pointer_cast<Variable>(nexpr->next);
    if (pVar)
        return pVar->inverse();

    // Two nots cancel
    std::shared_ptr<Not> n2expr = std::dynamic_pointer_cast<Not>(nexpr->next);
    if (n2expr)
        return expand(n2expr->next);

    // cannot expand constants
    std::shared_ptr<Constant> cexpr = std::dynamic_pointer_cast<Constant>(nexpr->next);
    return std::dynamic_pointer_cast<Expr>(std::make_shared<Constant>(!cexpr->value));
}

template<typename T, bool way>
PExpr short_circuit(const std::shared_ptr<T> &oexpr) {
    std::shared_ptr<T> cpy = std::make_shared<T>();
    cpy->children.reserve(oexpr->children.size());

    std::unordered_set<std::string> positives;
    std::unordered_set<std::string> negatives;

    std::queue<PExpr> queue{};

    auto run_loop = [&](const PExpr &i) {
        std::shared_ptr<Variable> var = std::dynamic_pointer_cast<Variable>(i);
        std::shared_ptr<Constant> constv = std::dynamic_pointer_cast<Constant>(i);

    retry:
        if (var) {
            // it's inverse was OP'd together with itself!
            // This always results in a constant
            if ((var->inv ? positives : negatives).count(var->value->name) > 0)
                return 1 + static_cast<int>(way);// true for OR, false for AND

            // test if insertion took place
            if ((var->inv ? negatives : positives).emplace(var->value->name).second) {
                cpy->children.emplace_back(var);
            }
        } else if (constv) {
            // way is false for AND, true for OR so that
            // OR returns TRUE if ANY true is encountered
            // AND returns FALSE if ANY false is encountered
            if (constv->value == way) return 1 + static_cast<int>(way);
        } else {
            // Nots, Ors, Ands
            auto res = expand(i);
            constv = std::dynamic_pointer_cast<Constant>(res);
            var = std::dynamic_pointer_cast<Variable>(res);
            if (constv || var)
                // simplification revealed additional short-circuit possibilities
                goto retry;

            std::shared_ptr<T> unwrap = std::dynamic_pointer_cast<T>(res);
            if (unwrap)
                for (auto &push: unwrap->children)
                    queue.push(push);
            else
                cpy->children.emplace_back(res);
        }

        return 0;
    };

    for (const auto &i: oexpr->children) {
        int r = run_loop(i);
        if (r) return std::make_shared<Constant>(r - 1 != 0);
    }

    while (!queue.empty()) {
        int r = run_loop(queue.front());
        if (r) return std::make_shared<Constant>(r - 1 != 0);
        queue.pop();
    }

    /* std::cout << "NEGATIVES: [";
    for (const auto &n : negatives) std::cout << n << ", ";
    std::cout << "]\nPOSITIVES: [";
    for (const auto &p : positives) std::cout << p << ", ";
    std::cout << "]\n"; */

    if (cpy->children.empty())
        return std::make_shared<Constant>(false);
    else if (cpy->children.size() == 1)
        return cpy->children[0];

    return cpy;
}

PExpr full_expand(const PExpr &expr, int n = 1) {
    if (n == 0) return expand(expr);
    return expand(full_expand(expr, n - 1));
}

PExpr expand(const PExpr &expr) {

    // Simplify NOT with demorgans and stuff
    std::shared_ptr<Not> nexpr = std::dynamic_pointer_cast<Not>(expr);
    if (nexpr)
        return not_simplify(nexpr);

    // short circuit OR
    std::shared_ptr<Or> oexpr = std::dynamic_pointer_cast<Or>(expr);
    if (oexpr) {
        // TODO: After simplification, get rid of all more specific cases in ORs
        //       For example: if we have A*B + A*B*C, we can simplify to A*B

        if (oexpr->children.size() <= 1)
            throw std::runtime_error{"OR Expression with fewer than 2 children"};
        //        if (oexpr->children.empty())
        //            return std::make_shared<Constant>(false);
        //        else if (oexpr->children.size() == 1)
        //            return oexpr->children[0];
        return short_circuit<Or, true>(oexpr);
    }

    // short circuit AND
    std::shared_ptr<And> aexpr = std::dynamic_pointer_cast<And>(expr);
    if (aexpr) {
        if (aexpr->children.size() <= 1)
            throw std::runtime_error{"AND Expression with fewer than 2 children"};
        //        if (aexpr->children.empty())
        //            return std::make_shared<Constant>(false);
        //        else if (aexpr->children.size() == 1)
        //            return aexpr->children[0];

        long dist_index = (long) aexpr->children.size();

        // advance dist_index forward until we find an OR expression
        while (--dist_index >= 0 &&
               std::dynamic_pointer_cast<Or>(aexpr->children[dist_index]) == nullptr)
            ;

        // cannot apply distributive property
        if (dist_index < 0)
            return short_circuit<And, false>(aexpr);

        // make a copy
        // TODO: Apply short-circuit here
        std::shared_ptr<Or> dist = std::make_shared<Or>(
                *std::dynamic_pointer_cast<Or>(aexpr->children[dist_index]));

        std::vector<PExpr> friends = aexpr->children;
        friends.erase(friends.begin() + dist_index);

        for (auto &i: dist->children) {
            auto to_be = std::make_shared<And>(friends);
            to_be->children.emplace_back(i);
            i = to_be;
        }

        if (dist->children.empty())
            return std::make_shared<Constant>(false);
        else if (dist->children.size() == 1)
            return dist->children[0];
        else
            return short_circuit<Or, true>(dist);
    }

    // constants and variables cannot be simplified
    return expr;
}

inline PExpr inv(const PExpr &rhs) {
    return std::dynamic_pointer_cast<Expr>(std::make_shared<Not>(rhs));
}

template<class... Args>
inline PExpr mul(Args... rhs) {
    return std::dynamic_pointer_cast<Expr>(std::make_shared<And>(
            std::vector<PExpr>{{std::dynamic_pointer_cast<Expr>(rhs)...}}));
}

template<class... Args>
inline PExpr add(Args... rhs) {
    return std::dynamic_pointer_cast<Expr>(std::make_shared<Or>(
            std::vector<PExpr>{{std::dynamic_pointer_cast<Expr>(rhs)...}}));
}
