#include "Debug.hpp"
#include <iostream>

void Debug::report(int line, const std::string& where, const std::string& message){
  hadError = true;
  //std::cerr << "[" + Debug::filename + "] " << "error: line: " << line << where << ": " << message << '\n';
  std::cerr << "erro: linha: " << line << where << ": " << message << '\n';
} 

void Debug::error(int line, const std::string& message){
  report(line, "", message);
}

void Debug::error(Token token, const std::string& message){
  if(token.type == TokenType::NX_EOF){
    report(token.line, " no final ", message);
  }else{
    report(token.line, " no " + token.lexeme, message);
  }
}

void Debug::runtimeError(const RuntimeError& error){
  //std::cerr << "[" + Debug::filename + "] " << "[line " << error.token.line << "] Error: " << error.what() << '\n';
  std::cerr << "[linha " << error.token.line << "] Erro: " << error.what() << '\n';
}
