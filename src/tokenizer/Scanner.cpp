#include "Scanner.hpp"
#include "../utils/Debug.hpp"

Scanner::Scanner(const std::string& source) : source(source) {}

std::vector<Token> Scanner::scanTokens(){
  while(!isAtEnd()){
    start = current;
    scanToken();
  }
  tokens.emplace_back(TokenType::NX_EOF, "", nullptr, line);
  return tokens;
}

bool Scanner::isAtEnd(){
  return current >= static_cast<int>(source.length());
}

void Scanner::addToken(TokenType type){
  addToken(type, nullptr);
}

void Scanner::addToken(TokenType type, std::any literal){
  std::string text{source.substr(static_cast<size_t>(start), static_cast<size_t>(current - start))};
  tokens.emplace_back(type, text, literal, line);
}

void Scanner::identifier(){
  while(isAlphaNumeric(peek())) advance();
  std::string text{source.substr(static_cast<size_t>(start), static_cast<size_t>(current - start))};
  auto it = keywords.find(text);
  TokenType type = it == keywords.end() ? TokenType::IDENTIFICAR : it->second;
  addToken(type);
}

void Scanner::number(){
  while(isDigit(peek())) advance();

  if(peek() == '.' && isDigit(peekNext())){
    advance();
    while(isDigit(peek())) advance();
  }

  std::string text{source.substr(static_cast<size_t>(start), static_cast<size_t>(current - start))};
  double number = std::stod(std::string{text});
  addToken(TokenType::NUMERO, number);
}

void Scanner::string(){
  while(peek() != '"' && !isAtEnd()){
    if(peek() == '\n'){ line++; }
    advance();
  }

  if(isAtEnd()){
    Debug::error(line, "Texto nao terminado.");
    return;
  }

  advance();

  std::string value{source.substr(static_cast<size_t>(start + 1), static_cast<size_t>(current - start - 2))};
  addToken(TokenType::TEXTO, value);
}

bool Scanner::match(char expected){
  if (isAtEnd() || source.at(static_cast<size_t>(current)) != expected) return false;
  current++;
  return true;
}

char Scanner::peek(){
  if(isAtEnd()) return '\0';
  return source.at(static_cast<size_t>(current));
}

char Scanner::peekNext(){
  if(current + 1 > static_cast<int>(source.length())) return '\0';
  return source.at(static_cast<size_t>(current + 1));
}

bool Scanner::isAlpha(char c){
  return (c >= 'a' && c <= 'z') || 
    (c >= 'A' && c <= 'Z') || 
    c == '_';
}

bool Scanner::isAlphaNumeric(char c){
  return isAlpha(c) || isDigit(c);
}

bool Scanner::isDigit(char c){
  return c >= '0' && c <= '9';
}

char Scanner::advance(){
  return source[static_cast<size_t>(current++)];
}

void Scanner::scanToken(){
  char c = advance();
  switch(c){
    case '&': addToken(TokenType::ECOMERCIAL); break;
    case '^': addToken(TokenType::ACENTOCHAPEU); break;
    case '|': addToken(TokenType::BARRAV); break;
    case '~': addToken(TokenType::TIL); break;
    case '%': addToken(TokenType::PORCENTAGEM); break;
    case '(': addToken(TokenType::PARENTESE_ESQUERDO); break;
    case ')': addToken(TokenType::PARENTESE_DIREITO); break;
    case '{': addToken(TokenType::CHAVE_ESQUERDA); break;
    case '}': addToken(TokenType::CHAVE_DIREITA); break;
    case ',': addToken(TokenType::VIRGULA); break;
    case '.': addToken(TokenType::PONTO); break;
    case '-': 
              if(match('-')){
                addToken(TokenType::MENOS_MENOS);
              }else{
                addToken(TokenType::MENOS); 
              }
              break;
    case '+': 
              if(match('+')){
                addToken(TokenType::MAIS_MAIS); 
              }else{
                addToken(TokenType::MAIS); 
              }
              break;
    case ';': addToken(TokenType::PONTOEVIRGULA); break;
    case '*': addToken(TokenType::ASTERISCO); break;
    case '[': addToken(TokenType::COLCHETE_ESQUERDO); break;
    case ']': addToken(TokenType::COLCHETE_DIREITO); break;
    case '!':
              addToken(match('=') ? TokenType::BANG_IGUAL : TokenType::BANG); break;
    case '=':
              addToken(match('=') ? TokenType::IGUAL_IGUAL : TokenType::IGUAL); break;
    case '<':
              if (match('=')) {
                addToken(TokenType::MAIOR_IGUAL);
              }else if (match('<')) {
                addToken(TokenType::MENOR_MENOR);
              }else {
                addToken(TokenType::MENOR);
              }
              break;
    case '>':
              if (match('=')) {
                addToken(TokenType::MAIOR_IGUAL);
              }else if (match('>')) {
                addToken(TokenType::MAIOR_MAIOR);
              }else {
                addToken(TokenType::MAIOR);
              }
              break;
    case '/':
              if(match('/')){
                while(peek() != '\n' && !isAtEnd()){
                  advance();
                }
              }else if(match('*')){
                while(!isAtEnd()){
                  if(peek() == '\n') line++;
                  if(match('*') && peek() == '/'){
                    advance();
                    return;
                  }
                  advance();
                }
                Debug::error(line, "Esperava-se que '*/' fechasse um comentário de varias linhas.");
              }else{
                addToken(TokenType::BARRA);
              }
              break;
    case ' ':
    case '\r':
    case '\t':
              break;
    case '\n':
              line++;
              break;
    case '"':
              string();
              break;
    default:
              if(isDigit(c)){
                number();
              }else if(isAlpha(c)){
                identifier();
              }else{
                Debug::error(line, "Caractere inesperado.");
              }
              break;
  } 
}
