#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

struct Token {
    string type;
    string value;
};

class Scanner {
private:
    string src;
    size_t pos = 0;

    char peek(int offset = 0) {
        if (pos + offset >= src.size())
            return '\0';
        return src[pos + offset];
    }

    char get() {
        if (pos >= src.size())
            return '\0';
        return src[pos++];
    }

    void skipBlank() {
        while (isspace(peek()))
            get();
    }

    bool isLetter(char c) {
        return isalpha(c);
    }

    bool isDigit2(char c) {
        return isdigit(c);
    }

public:
    Scanner(const string& s) : src(s) {}

    vector<Token> scan() {
        vector<Token> tokens;

        while (true) {
            skipBlank();

            char c = peek();

            if (c == '\0') {
                break;
            }

            // =========================
            // ID / KEY
            // =========================
            if (isLetter(c)) {
                string word;

                while (isLetter(peek()) || isDigit2(peek())) {
                    word += get();
                }

                if (word == "int")
                    tokens.push_back({"INT", word});
                else if (word == "float")
                    tokens.push_back({"FLOAT", word});
                else if (word == "void")
                    tokens.push_back({"VOID", word});
                else if (word == "if")
                    tokens.push_back({"IF", word});
                else if (word == "else")
                    tokens.push_back({"ELSE", word});
                else if (word == "while")
                    tokens.push_back({"WHILE", word});
                else if (word == "return")
                    tokens.push_back({"RETURN", word});
                else if (word == "input")
                    tokens.push_back({"INPUT", word});
                else if (word == "print")
                    tokens.push_back({"PRINT", word});
                else
                    tokens.push_back({"ID", word});

                continue;
            }

            // =========================
            // NUM / FLO
            // =========================
            if (isdigit(c) ||
                ((c == '+' || c == '-') && isdigit(peek(1)))) {

                string num;

                if (c == '+' || c == '-') {
                    num += get();
                }

                while (isdigit(peek())) {
                    num += get();
                }

                // float
                if (peek() == '.') {
                    num += get();

                    while (isdigit(peek())) {
                        num += get();
                    }

                    tokens.push_back({"FLO", num});
                }
                else {
                    tokens.push_back({"NUM", num});
                }

                continue;
            }

            // =========================
            // AAA ++
            // =========================
            if (peek() == '+' && peek(1) == '+') {
                get();
                get();
                tokens.push_back({"AAA", "++"});
                continue;
            }

            // =========================
            // AAS +=
            // =========================
            if (peek() == '+' && peek(1) == '=') {
                get();
                get();
                tokens.push_back({"AAS", "+="});
                continue;
            }

            // =========================
            // ROP
            // =========================
            if (peek() == '<') {
                get();

                if (peek() == '=') {
                    get();
                    tokens.push_back({"ROP", "<="});
                } else {
                    tokens.push_back({"ROP", "<"});
                }

                continue;
            }

            if (peek() == '>') {
                get();

                if (peek() == '=') {
                    get();
                    tokens.push_back({"ROP", ">="});
                } else {
                    tokens.push_back({"ROP", ">"});
                }

                continue;
            }

            if (peek() == '=' && peek(1) == '=') {
                get();
                get();
                tokens.push_back({"ROP", "=="});
                continue;
            }

            if (peek() == '!' && peek(1) == '=') {
                get();
                get();
                tokens.push_back({"ROP", "!="});
                continue;
            }

            // =========================
            // BOP
            // =========================
            if (peek() == '&' && peek(1) == '&') {
                get();
                get();
                tokens.push_back({"BOP", "&&"});
                continue;
            }

            if (peek() == '|' && peek(1) == '|') {
                get();
                get();
                tokens.push_back({"BOP", "||"});
                continue;
            }

            if (peek() == '!') {
                get();
                tokens.push_back({"BOP", "!"});
                continue;
            }

            // =========================
            // ASG =
            // =========================
            if (peek() == '=') {
                get();
                tokens.push_back({"ASG", "="});
                continue;
            }

            // =========================
            // AOP
            // =========================
            if (peek() == '+') {
                get();
                tokens.push_back({"ADD", "+"});
                continue;
            }

            if (peek() == '-') {
                get();
                tokens.push_back({"SUB", "-"});
                continue;
            }

            if (peek() == '*') {
                get();
                tokens.push_back({"MUL", "*"});
                continue;
            }

            if (peek() == '/') {
                get();
                tokens.push_back({"DIV", "/"});
                continue;
            }

            // =========================
            // DELIMITER
            // =========================
            if (peek() == '(') {
                get();
                tokens.push_back({"LPA", "("});
                continue;
            }

            if (peek() == ')') {
                get();
                tokens.push_back({"RPA", ")"});
                continue;
            }

            if (peek() == '[') {
                get();
                tokens.push_back({"LBK", "["});
                continue;
            }

            if (peek() == ']') {
                get();
                tokens.push_back({"RBK", "]"});
                continue;
            }

            if (peek() == '{') {
                get();
                tokens.push_back({"LBR", "{"});
                continue;
            }

            if (peek() == '}') {
                get();
                tokens.push_back({"RBR", "}"});
                continue;
            }

            if (peek() == ',') {
                get();
                tokens.push_back({"CMA", ","});
                continue;
            }

            if (peek() == ';') {
                get();
                tokens.push_back({"SCO", ";"});
                continue;
            }

            // UNKNOWN
            string unknown;
            unknown += get();
            tokens.push_back({"UNKNOWN", unknown});
        }

        return tokens;
    }
};

string readFile(const string& filename) {
    ifstream fin(filename);

    if (!fin.is_open()) {
        cerr << "无法打开文件" << endl;
        exit(1);
    }

    stringstream buffer;
    buffer << fin.rdbuf();

    return buffer.str();
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    cout << "请选择扫描模式：" << endl;
    cout << "1. 按空格分隔的符号串识别" << endl;
    cout << "2. 输入一行源码字符串扫描" << endl;
    cout << "3. 从文件读取源码扫描" << endl;
    cout << "请输入模式编号: ";

    int mode;
    cin >> mode;
    cin.ignore();

    string code;

    if (mode == 1) {
        int n;
        cout << "请输入符号串个数: ";
        cin >> n;

        cout << "请依次输入 " << n << " 个符号串:" << endl;

        for (int i = 0; i < n; i++) {
            string word;
            cin >> word;
            code += word;
            code += " ";
        }
    }
    else if (mode == 2) {
        cout << "请输入一行源码字符串:" << endl;
        getline(cin, code);
    }
    else if (mode == 3) {
        string filename;

        if (argc >= 2) {
            filename = argv[1];
        } else {
            cout << "请输入源程序文件名: ";
            getline(cin, filename);
        }

        code = readFile(filename);
    }
    else {
        cout << "模式编号错误" << endl;
        return 1;
    }

    Scanner scanner(code);
    vector<Token> tokens = scanner.scan();

    cout << endl;
    cout << "词法分析结果：" << endl;
    cout << "------------------------" << endl;

    for (auto& t : tokens) {
        cout << "(" << t.type << ", " << t.value << ")" << endl;
    }

    return 0;
}