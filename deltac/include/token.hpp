#pragma once

#include "tokentype.hpp"
#include "charinfo.hpp"

#include <iostream>
#include <optional>
#include <typeinfo>

namespace deltac {

class Token {
public:
    bool is(tok::Kind type1) const { return type == type1; }
    template <typename... Ts>
    bool is_one_of(tok::Kind type, Ts... types) const {
        return (is(type) || ...);
    }

    void concat(const Token& other) {
        code_view = util::make_sv(std::min(code_view.begin(), other.code_view.begin()),
                                  std::max(code_view.end(), other.code_view.end()));
    }

    void start_token() {
        type = tok::ERROR;
        code_view = "";
    }

    void set_view(std::string_view sv) { code_view = sv; }
    void set_view(const char* begin, const char* end) { code_view = util::make_sv(begin, end); }
    void set_view(const char* begin, std::size_t size) { code_view = std::string_view(begin, size); }
    std::string_view get_view() const { return code_view; }

    void set_type(tok::Kind t) { type = t; }
    tok::Kind get_type() const { return type; }

    std::string_view get_name() const { return token_type_name(get_type()); }

private:
    tok::Kind type = tok::ERROR;
    std::string_view code_view;
    // std::any data;

    friend std::ostream& operator <<(std::ostream&, const Token& tk);
};

inline std::ostream& operator <<(std::ostream& os, const Token& tk) {
    os << token_type_enum_name(tk.type) << ": " << tk.code_view;
    return os;
}

}
