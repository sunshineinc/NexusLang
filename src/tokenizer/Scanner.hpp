#pragma once

#include <vector>
#include <unordered_map>
#include "Token.hpp"

class Scanner {
  private:
    int start = 0;
    int current = 0;
    int line = 1;
    std::string source;
    std::vector<Token> tokens;
    std::unordered_map<std::string, TokenType> keywords = {
      {"incluir",TokenType::INCLUIR},
      {"e",    TokenType::E},
      {"classe",  TokenType::CLASSE},
      {"senao",   TokenType::SENAO},
      {"falso",  TokenType::FALSO},
      {"por",    TokenType::POR},
      {"definir",    TokenType::DEFINIR},
      {"se",     TokenType::SE},
      {"nulo",    TokenType::NULO},
      {"ou",     TokenType::OU},
      {"said",     TokenType::SAID},
      {"saida",  TokenType::SAIDA},
      {"retorne", TokenType::RETORNE},
      {"super",  TokenType::SUPER},
      {"isso",   TokenType::ISSO},
      {"verdadeiro",   TokenType::VERDADEIRO},
      {"var",    TokenType::VAR},
      {"enquanto",  TokenType::ENQUANTO}
    };

    bool isAlpha(char c);
    bool isDigit(char c);
    bool isAlphaNumeric(char c);
    bool match(char expected);
    void scanToken();
    char advance();
    void addToken(TokenType type);
    void addToken(TokenType type, std::any literal);
    char peek();
    char peekNext();
    void string();
    void number();
    void identifier();

  public:
    Scanner(const std::string&);
    bool isAtEnd();
    std::vector<Token> scanTokens();
};
