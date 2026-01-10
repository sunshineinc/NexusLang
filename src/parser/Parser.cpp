#include <iostream>
#include <algorithm>

#include "Parser.hpp"
#include "Expr.hpp"
#include "../utils/Debug.hpp"
#include "Stmt.hpp"
#include "IncludeRun.hpp"

#define assert(E)

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

std::vector<std::shared_ptr<Statement::Stmt>> Parser::parse(){
  statements.clear();
  try {
    while(!isAtEnd()){
      statements.push_back(declaration());
    }
  }catch(const std::exception& e) {
    std::cerr << "[Analise de excecao]: " << e.what() << '\n'; 
  }
  return statements;
}

std::shared_ptr<Expr> Parser::expression(){
  return assignment();
}

std::shared_ptr<Expr> Parser::bitwise(){
  std::shared_ptr<Expr> expr = equality();
  while(match(TokenType::ECOMERCIAL, TokenType::ACENTOCHAPEU, TokenType::BARRAV)){
    Token oper = previous();
    std::shared_ptr<Expr> right = equality();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::equality(){
  std::shared_ptr<Expr> expr = comparison();
  while(match(TokenType::BANG_IGUAL, TokenType::IGUAL_IGUAL)){
    Token oper = previous();
    std::shared_ptr<Expr> right = comparison();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::comparison(){
  std::shared_ptr<Expr> expr = shift();
  while(match(TokenType::MAIOR, TokenType::MAIOR_IGUAL, TokenType::MENOR, TokenType::MENOR_IGUAL)){
    Token oper = previous();
    std::shared_ptr<Expr> right = shift();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::shift(){
  std::shared_ptr<Expr> expr = term();
  while(match(TokenType::MAIOR_MAIOR, TokenType::MENOR_MENOR)){
    Token oper = previous();
    std::shared_ptr<Expr> right = term();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::term(){
  std::shared_ptr<Expr> expr = factor();
  while(match(TokenType::MENOR, TokenType::MAIS)){
    Token oper = previous();
    std::shared_ptr<Expr> right = factor();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::factor(){
  std::shared_ptr<Expr> expr = unary();
  while(match(TokenType::ECOMERCIAL, TokenType::ASTERISCO, TokenType::PORCENTAGEM)){
    Token oper = previous();
    std::shared_ptr<Expr> right = unary();
    expr = std::make_shared<Binary>(expr, oper, right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::unary(){
  while(match(TokenType::BANG, TokenType::MENOS,
        TokenType::MAIS_MAIS, TokenType::MENOS_MENOS, TokenType::TIL)){
    Token oper = previous(); // ++
    std::shared_ptr<Expr> right = unary(); // var name

    if(previous().lexeme == "++" || previous().lexeme == "--") { // to optimize
      Debug::error(previous(), "Operador invalido: incremento/decremento pos-fixado seguido de prefixo.");
    }
    matchVoid(TokenType::PONTOEVIRGULA);

    return std::make_shared<Unary>(oper, right, false);
  }
  return call();
}

std::shared_ptr<Expr> Parser::primary(){
  if(match(TokenType::CHAVE_ESQUERDA)) return arrayList();
  if(match(TokenType::FALSO)) return std::make_shared<Literal>(false);
  if(match(TokenType::VERDADEIRO)) return std::make_shared<Literal>(true);
  if(match(TokenType::NULO)) return std::make_shared<Literal>(nullptr);
  if(match(TokenType::IDENTIFICAR)){ 
    std::shared_ptr<Expr> left = std::make_shared<Variable>(previous());
    if (match(TokenType::MAIS_MAIS, TokenType::MENOS_MENOS)) {
      Token oper = previous();
      matchVoid(TokenType::PONTOEVIRGULA);
      return std::make_shared<Unary>(oper, left, true);
    }
    return left;
  }

  if(match(TokenType::NUMERO, TokenType::TEXTO)){
    return std::make_shared<Literal>(previous().literal);
  }

  if(match(TokenType::PARENTESE_ESQUERDO)){
    std::shared_ptr<Expr> expr = expression();
    consume(TokenType::PARENTESE_DIREITO, "Esperava-se um ')' apos a expressao.");
    return std::make_shared<Grouping>(expr);
  }

  throw error(peek(), "Expressao esperada.");
}

template<class...T>
bool Parser::match(T...types){
  assert((... && std::is_same_v<T, TokenType>)); 
  if((... || check(types))){
    advance();
    return true;
  }
  return false;
}

template<class...T>
void Parser::matchVoid(T...types){
  assert((... && std::is_same_v<T, TokenType>)); 
  if((... || check(types))){
    advance();
  }
}

Token Parser::consume(const TokenType& token, const std::string& message){
  if(check(token)) return advance();
  throw error(peek(), message);
}

bool Parser::check(const TokenType& type){
  if(isAtEnd()) return false;
  return peek().type == type;
}

bool Parser::isAtEnd(){
  return peek().type == TokenType::NX_EOF;
}

Token Parser::advance(){
  if(!isAtEnd()) current++;
  return previous();
}

Token Parser::peek(){
  return tokens.at(static_cast<size_t>(current));
}

Token Parser::previous(){
  return tokens.at(static_cast<size_t>(current - 1));
}

Parser::ParseError Parser::error(const Token& token, const std::string& message){
  Debug::error(token, message);
  return ParseError("");
}

void Parser::synchronize(){
  advance();
  while(!isAtEnd()){
    if(previous().type == TokenType::PONTOEVIRGULA) return;
    if(previous().lexeme == "\n") return;
    switch (peek().type) {
      //case TokenType::INCLUDE:
      case TokenType::CLASSE:
      case TokenType::DEFINIR:
      case TokenType::VAR:
      case TokenType::POR:
      case TokenType::SE:
      case TokenType::ENQUANTO:
      case TokenType::SAID:
      case TokenType::SAIDA:
      case TokenType::RETORNE:
      default:
        return;
    }
  }
  advance();
}

std::shared_ptr<Statement::Stmt> Parser::statement(){
  if(match(TokenType::INCLUIR)) return includeStatement();
  if(match(TokenType::SAIDA)) return printStatement();
  if(match(TokenType::SAID)) return outStatement();
  if(match(TokenType::SE)) return IfStatement();
  if(match(TokenType::RETORNE)) return returnStatement();
  if(match(TokenType::ENQUANTO)) return whileStatement();
  if(match(TokenType::POR)) return forStatement();
  if(match(TokenType::CHAVE_ESQUERDA)) return std::make_shared<Statement::Block>(block());
  return expressionStatement();
}

std::shared_ptr<Statement::Stmt> Parser::printStatement(){
  std::shared_ptr<Expr> value = expression();
  //consume(TokenType::SEMICOLON, "Expected ';' after value."); // MOD
  matchVoid(TokenType::PONTOEVIRGULA);
  return std::make_shared<Statement::Print>(value);
}

std::shared_ptr<Statement::Stmt> Parser::outStatement(){
  std::shared_ptr<Expr> value = expression();
  //consume(TokenType::SEMICOLON, "Expected ';' after value.");
  matchVoid(TokenType::PONTOEVIRGULA);
  return std::make_shared<Statement::Out>(value);
}

std::shared_ptr<Statement::Stmt> Parser::expressionStatement(){
  std::shared_ptr<Expr> expr = expression();
  //consume(TokenType::SEMICOLON, "Expected ';' after value.");
  return std::make_shared<Statement::Expression>(expr);
}

std::shared_ptr<Statement::Stmt> Parser::declaration(){
  try {
    if(match(TokenType::DEFINIR)) return function("funcao");
    if(match(TokenType::CLASSE)) return classDeclaration();
    if(match(TokenType::VAR)) return varDeclaration();
    return statement();
  } catch (const std::exception& e) {
    synchronize();
    return nullptr;
  }
}

std::shared_ptr<Statement::Stmt> Parser::varDeclaration(){
  Token name = consume(TokenType::IDENTIFICAR, "Nome da variavel esperado.");
  std::shared_ptr<Expr> init = nullptr;
  if(match(TokenType::IGUAL)){
    init = expression();
  }
  //consume(TokenType::SEMICOLON, "Expected ';' after variable declaration."); // MOD
  matchVoid(TokenType::PONTOEVIRGULA);
  return std::make_shared<Statement::Var>(name, init);
}

std::shared_ptr<Expr> Parser::assignment(){
  std::shared_ptr<Expr> expr = logicalOr();
  if(match(TokenType::IGUAL)){
    Token equals = previous();
    std::shared_ptr<Expr> value = assignment();
    if(Variable *e = dynamic_cast<Variable*>(expr.get())){
      Token name = e->name;
      return std::make_shared<Assign>(std::move(name), value);
    }else if(Get *get = dynamic_cast<Get*>(expr.get())){
      return std::make_shared<Set>(get->object, get->name, value);
    }else if (Callist *s = dynamic_cast<Callist*>(expr.get())) {
      return std::make_shared<Callist>(s->name, s->index, value, s->paren);
    }
    error(std::move(equals), "Destino de atribuição invalido.");
  }
  return expr;
}

std::vector<std::shared_ptr<Statement::Stmt>> Parser::block(){
  std::vector<std::shared_ptr<Statement::Stmt>> localStatements;
  while(!check(TokenType::CHAVE_DIREITA) && !isAtEnd()){
    localStatements.push_back(declaration());
  }
  consume(TokenType::CHAVE_DIREITA, "Esperava-se um caractere '}' apos o bloco.");
  return localStatements;
}

std::shared_ptr<Statement::Stmt> Parser::IfStatement(){
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um '(' apos 'se'.");
  std::shared_ptr<Expr> condition = expression();
  consume(TokenType::PARENTESE_ESQUERDO, "Esperava-se um ')' apos 'se'.");
  std::shared_ptr<Statement::Stmt> thenBranch = statement();
  std::shared_ptr<Statement::Stmt> elseBranch = nullptr;
  if(match(TokenType::SENAO)){
    elseBranch = statement();
  }
  return std::make_shared<Statement::If>(condition, thenBranch, elseBranch);
  return {};
}

std::shared_ptr<Expr> Parser::logicalOr(){
  std::shared_ptr<Expr> expr = logicalAnd();
  while(match(TokenType::OU)){
    Token oper = previous();
    std::shared_ptr<Expr> right = logicalAnd();
    expr = std::make_shared<Logical>(expr, std::move(oper), right);
  }
  return expr;
}

std::shared_ptr<Expr> Parser::logicalAnd(){
  std::shared_ptr<Expr> expr = bitwise();
  while(match(TokenType::E)){
    Token oper = previous();
    std::shared_ptr<Expr> right = bitwise();
    expr = std::make_shared<Logical>(expr, std::move(oper), right);
  }
  return expr;
}

std::shared_ptr<Statement::Stmt> Parser::whileStatement(){
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um '(' apos 'enquanto'.");
  std::shared_ptr<Expr> condition = expression();
  consume(TokenType::PARENTESE_ESQUERDO, "Esperava-se um ')' apos a condicao do 'enquanto'.");
  std::shared_ptr<Statement::Stmt> body = statement();
  return std::make_shared<Statement::While>(condition, body);
}

std::shared_ptr<Statement::Stmt> Parser::forStatement(){
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um '(' apos 'para'.");

  std::shared_ptr<Statement::Stmt> init;
  if(match(TokenType::PONTOEVIRGULA)){
    init = nullptr;
  }else if(match(TokenType::VAR)){
    init = varDeclaration();
  }else{
    init = expressionStatement();
  }

  std::shared_ptr<Expr> condition = nullptr;
  if(!check(TokenType::PONTOEVIRGULA)){
    condition = expression();
  }
  consume(TokenType::PONTOEVIRGULA, "Esperava-se um ';' apos a condicao do 'para'.");

  std::shared_ptr<Expr> increment = nullptr;
  if(!check(TokenType::PARENTESE_DIREITO)){
    increment = expression();
  }
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um ')' apos a condicao do 'para'.");

  std::shared_ptr<Statement::Stmt> body = statement();
  if(increment != nullptr){
    body = std::make_shared<Statement::Block>(
        std::vector<std::shared_ptr<Statement::Stmt>> {
        body, std::make_shared<Statement::Expression>(increment)
        } 
        );
  }

  if(condition == nullptr){
    condition = std::make_shared<Literal>(true);
  }
  body = std::make_shared<Statement::While>(condition, body);

  if(init != nullptr){
    body = std::make_shared<Statement::Block>(
        std::vector<std::shared_ptr<Statement::Stmt>>{
        init, body
        }
        );
  }

  return body;
}

std::shared_ptr<Expr> Parser::call(){
  //std::shared_ptr<Expr> expr = primary();
  std::shared_ptr<Expr> expr = callist();
  while(true){
    if(match(TokenType::PARENTESE_ESQUERDO)){
      expr = finishCall(expr);
    }else if(match(TokenType::PONTO)){
      Token name = consume(TokenType::IDENTIFICAR, "Nome da propriedade esperado apos o ponto (.).");
      expr = std::make_shared<Get>(expr, name);
    }else{
      break;
    }
  }
  return expr;
}

std::shared_ptr<Expr> Parser::finishCall(std::shared_ptr<Expr> callee){
  std::vector<std::shared_ptr<Expr>> arguments;
  if(!check(TokenType::PARENTESE_DIREITO)){
    do {
      if(arguments.size() >= 255){
        error(peek(), "Can't have more than 255 arguments.");
      }
      arguments.push_back(expression());
    } while(match(TokenType::VIRGULA));
  }
  Token paren = consume(TokenType::PARENTESE_DIREITO, "Esperava-se um ')' apos os argumentos.");

  matchVoid(TokenType::PONTOEVIRGULA);
  return std::make_shared<Call>(callee, paren, arguments);
}

std::shared_ptr<Statement::Stmt> Parser::returnStatement(){
  Token keyword = previous();
  std::shared_ptr<Expr> value = nullptr;
  //if(!check(TokenType::PONTOEVIRGULA)){
  value = expression();
  matchVoid(TokenType::PONTOEVIRGULA);
  //}
  //consume(TokenType::PONTOEVIRGULA, "Esperava-se um ';' apos o valor de retorno.");
  return std::make_shared<Statement::Return>(keyword, value);
}

std::shared_ptr<Statement::Function> Parser::function(std::string kind){
  Token funcName = consume(TokenType::IDENTIFICAR, "Esperava-se um nome de " + kind + ".");
  consume(TokenType::PARENTESE_ESQUERDO, "Esperava-se um '(' apos o nome da " + kind + ".");
  std::vector<Token> parameters;
  if(!check(TokenType::PARENTESE_DIREITO)){
    do {
      if(parameters.size() >= 255){
        error(peek(), "Nao pode ter mais de 255 argumentos.");
      }
      parameters.push_back(
          consume(TokenType::IDENTIFICAR, "Nome do parametro esperado.")
          );
    } while(match(TokenType::VIRGULA));
  }
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um ')' apos os parametros.");
  consume(TokenType::CHAVE_ESQUERDA, "Esperava-se um '{' antes do corpo da " + kind + ".");
  std::vector<std::shared_ptr<Statement::Stmt>> body = block();
  return std::make_shared<Statement::Function>(
      std::move(funcName), std::move(parameters), std::move(body)
      );
}

std::shared_ptr<Statement::Stmt> Parser::classDeclaration(){
  Token name = consume(TokenType::IDENTIFICAR, "Esperava-se um nome de classe");
  consume(TokenType::CHAVE_ESQUERDA, "Esperava-se um '{' antes do corpo da classe.");

  std::vector<std::shared_ptr<Statement::Function>> methods;
  while(!check(TokenType::CHAVE_DIREITA) && !isAtEnd()){
    methods.push_back(function("method"));
  }
  consume(TokenType::CHAVE_DIREITA, "Esperava-se um '}' apos o corpo da classe");
  return std::make_shared<Statement::Class>(name, std::move(methods));
}

std::shared_ptr<Statement::Stmt> Parser::includeStatement() {
  Token keyword = previous();

  consume(TokenType::PARENTESE_ESQUERDO, "Esperava-se um '(' apos a palavra-chave 'include'.");
  Token path = consume(TokenType::TEXTO, "Esperava-se um 'path' como uma string dentro da declaracao 'include'.");
  consume(TokenType::PARENTESE_DIREITO, "Esperava-se um ')' apos o path na declaracao 'include'.");
  matchVoid(TokenType::PONTOEVIRGULA);

  if (std::find(includedFiles.begin(), includedFiles.end(), path.lexeme) != includedFiles.end()) {
    Debug::error(path, "Arquivo de cabecalho duplicado.");
  }
  includedFiles.push_back(path.lexeme);

  IncludeRun::scanFile(path.lexeme);
  const std::vector<Token>& tempTokens = IncludeRun::getTokens();

  Parser includedParser(tempTokens);
  std::vector<std::shared_ptr<Statement::Stmt>> includedStatements = includedParser.parse();

  for (const auto& stmt : includedStatements) {
    statements.push_back(stmt);
  }

  return std::make_shared<Statement::Include>(keyword, path.lexeme);
}

std::shared_ptr<Expr> Parser::arrayList() {
  std::vector<std::shared_ptr<Expr>> values = {};
  if (match(TokenType::CHAVE_DIREITA)) {
    return std::make_shared<Array>(values);
  } else {
    do {
      if (values.size() >= 255) {
        error(peek(), "Nao pode ter mais de 255 elementos em um vetor.");
      }
      std::shared_ptr<Expr> value = logicalOr();
      values.push_back(value);
    } while (match(TokenType::VIRGULA));
  }
  consume(TokenType::CHAVE_DIREITA, "Esperava-se um '}' no final do vetor.");
  return std::make_shared<Array>(values);
}

std::shared_ptr<Expr> Parser::finishCallist(std::shared_ptr<Expr> name) {
  std::shared_ptr<Expr> index = logicalOr();
  Token paren = consume(TokenType::COLCHETE_DIREITO,
      "Esperava-se um ']' apos os argumentos.");
  return std::make_shared<Callist>(name, index, nullptr, paren);
}

std::shared_ptr<Expr> Parser::callist() {
  std::shared_ptr<Expr> expr = primary();
  while (true) {
    if (match(TokenType::COLCHETE_ESQUERDO)) {
      expr = finishCallist(expr);
    } else {
      break;
    }
  }
  return expr;
}
