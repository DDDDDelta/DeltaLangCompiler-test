#pragma once

#include "token.hpp"
#include "tokentype.hpp"
#include "charinfo.hpp"
#include "filebuffer.hpp"
#include "keywordtrie.hpp"

#include <string>
#include <utility>
#include <cassert>

namespace deltac {

class Lexer {
public:
    explicit Lexer(const char* begin, const char* end);
    
    bool lex(Token& result);
    
    bool is_eof() const {
        return buffer_curr == buffer_end;
    }
    
private:
    static const KeywordTrie kwtrie;

private:
    void form_token(Token& result, const char* token_end, tok::Kind type);

    bool lex_numeric_literal(Token& result, const char* curr_ptr);
    bool lex_hex(Token& result, const char* curr_ptr);
    bool lex_string_literal(Token& result, const char* curr_ptr);
    bool lex_identifier_continue(Token& result, const char* curr_ptr);

private:
    // points to the first character
    const char* buffer_start;
    // points to the next position of null terminate character
    const char* buffer_end;

    // points the next character that is about to be lexed
    const char* buffer_curr;

    // friend int main();
};

}
