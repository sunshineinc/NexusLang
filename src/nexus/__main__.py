import sys
import re
import io

if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(50000)

if hasattr(sys, "setrecursionlimit"):
    sys.setrecursionlimit(2000)

from .runtime import SymbolTable, Context
from .values import Number, BuiltInFunction, List
from .lexer import Lexer
from .parser import Parser
from .interpreter import Interpreter


def get_fresh_global_scope():
    scope = SymbolTable()

    scope.set("NULO", Number.null)
    scope.set("FALSO", Number.false)
    scope.set("VERDADEIRO", Number.true)

    scope.set("ENTRADA", BuiltInFunction("ENTRADA"))
    scope.set("TEXT", BuiltInFunction("TEXT"))
    scope.set("INT", BuiltInFunction("INT"))
    scope.set("FLUTUANTE", BuiltInFunction("FLUTUANTE"))
    scope.set("BOOL", BuiltInFunction("BOOL"))
    scope.set("IMPRIMIR", BuiltInFunction("IMPRIMIR"))

    return scope


def run(fn, text, context=None):
    lexer = Lexer(fn, text)
    tokens, error = lexer.make_tokens()
    if error:
        return None, error

    parser = Parser(tokens)
    ast = parser.parse()
    if ast.error:
        return None, ast.error

    interpreter = Interpreter()

    if context is None:
        context = Context("<program>")
        context.symbol_table = get_fresh_global_scope()

    result = interpreter.visit(ast.node, context)

    if result.should_return:
        return result.return_value, result.error

    return result.value, result.error


def is_complete(text):
    text = re.sub(r"#.*", "", text)

    temp_text = re.sub(r'""".*?"""', "", text, flags=re.DOTALL)
    if temp_text.count('"""') % 2 != 0:
        return False

    temp_text = re.sub(r"`[^`]*`", "", temp_text, flags=re.DOTALL)
    if temp_text.count("`") % 2 != 0:
        return False

    temp_text = re.sub(r'"[^"\\]*(\\.[^"\\]*)*"', "", temp_text)
    if temp_text.count('"') % 2 != 0:
        return False

    if temp_text.count("(") != temp_text.count(")"):
        return False
    if temp_text.count("[") != temp_text.count("]"):
        return False
    if temp_text.count("{") != temp_text.count("}"):
        return False

    depth = 0
    bracket_level = 0

    pattern = r"(\[|\]|\b(FUNCAO|CLASSE|SE|SENAO\s+SE|SENAO|ENQUANTO|PARA|TENTE|ESCOLHA|FIMFUNCAO|FIMCLASSE|FIMSE|FIMENQUANTO|PARA|FIMTENTE|FIMESCOLHA)\b)"

    tokens = re.finditer(pattern, temp_text)

    start_keys = {"FUNCAO", "CLASSE", "SE", "ENQUANTO", "PARA", "TENTE", "ESCOLHA"}
    end_keys = {
        "FIMFUNCAO",
        "FIMCLASSE",
        "FIMSE",
        "FIMENQUANTO",
        "FOR",
        "FIMTENTE",
        "FIMESCOLHA",
    }

    for match in tokens:
        token = match.group()

        if token in ("SENAO", "SENAO SE"):
            continue

        if token == "[":
            bracket_level += 1
        elif token == "]":
            bracket_level -= 1
        elif token in start_keys:
            depth += 1
        elif token in end_keys:
            depth -= 1

    return depth <= 0


def main():
    GLADLANG_VERSION = "0.1.3"
    GLADLANG_HELP = f"""
Uso: nexus [comando] [arquivo] [argumentos...]

Comandos:
  <nenhum argumento>        Inicia o shell interativo do GladLang.
  [arquivo.nx]            Executa um arquivo de script GladLang.
  [arquivo.nx] [args]     Executa o script e passa os argumentos para INPUT().
  --help                    Mostra esta mensagem de ajuda e sai.
  --version                 Mostra a versão do interpretador e sai.
"""

    if len(sys.argv) == 1:
        print(f"Bem-vindo ao Nexus (v{GLADLANG_VERSION})")
        print("Escreva 'sair' ou 'quitar' para fechar o shell.")
        print("--------------------------------------------------")

        repl_context = Context("<repl>")
        repl_context.symbol_table = get_fresh_global_scope()

        full_text = ""

        while True:
            try:
                prompt = "Nexus > " if not full_text else "...        > "
                line = input(prompt)

                if not full_text and line.strip().lower() in ("sair", "quitar"):
                    break

                full_text += line + "\n"

                if is_complete(full_text):
                    if full_text.strip() == "":
                        full_text = ""
                        continue

                    result, error = run("<stdin>", full_text, repl_context)

                    if error:
                        print(error.as_string())
                    elif result:
                        clean_text = re.sub(r"#.*", "", full_text).strip()

                        if clean_text.startswith("LET "):
                            pass
                        elif isinstance(result, Number) and result.value == 0:
                            pass
                        elif (
                            isinstance(result, List)
                            and len(result.elements) == 1
                            and result.elements[0].value == "NULL"
                        ):
                            pass
                        else:
                            print(result)

                    full_text = ""

            except KeyboardInterrupt:
                print("\nTecladoInterrupcao")
                full_text = ""
                continue
            except EOFError:
                print("\nSaindo do shell.")
                break
            except Exception as e:
                print(f"Erro no Shell: {e}")
                full_text = ""

    elif len(sys.argv) >= 2:
        arg = sys.argv[1]

        if arg == "--help":
            print(GLADLANG_HELP)

        elif arg == "--version":
            print(f"Nexus v{GLADLANG_VERSION}")

        else:
            try:
                filename = arg

                script_args = sys.argv[2:]

                if script_args:
                    sys.stdin = io.StringIO("\n".join(script_args) + "\n")

                with open(filename, "r") as f:
                    text = f.read()

                result, error = run(filename, text)

                if error:
                    print(error.as_string(), file=sys.stderr)

            except FileNotFoundError:
                print(f"Arquivo nao encontrado: '{filename}'", file=sys.stderr)
            except Exception as e:
                print(f"Ocorreu um erro inesperado: {e}", file=sys.stderr)

    else:
        print("Erro: Argumentos invalidos.")
        print(GLADLANG_HELP)


if __name__ == "__main__":
    main()
