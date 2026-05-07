#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

vector<char> alphabet;                  // 字符集
vector<string> states;                  // 状态集
string startState;                      // 开始状态
vector<string> acceptStates;            // 接受状态集
vector<vector<string>> trans;           // 转移矩阵

// 判断状态是否在状态集中
bool inStates(string s) {
    for (int i = 0; i < states.size(); i++) {
        if (states[i] == s) return true;
    }
    return false;
}

// 判断是否为接受状态
bool isAccept(string s) {
    for (int i = 0; i < acceptStates.size(); i++) {
        if (acceptStates[i] == s) return true;
    }
    return false;
}

// 判断字符是否在字符集中
bool inAlphabet(char c) {
    for (int i = 0; i < alphabet.size(); i++) {
        if (alphabet[i] == c) return true;
    }
    return false;
}

// 返回字符在字符集中的下标
int getAlphabetIndex(char c) {
    for (int i = 0; i < alphabet.size(); i++) {
        if (alphabet[i] == c) return i;
    }
    return -1;
}

// 返回状态在状态集中的下标
int getStateIndex(string s) {
    for (int i = 0; i < states.size(); i++) {
        if (states[i] == s) return i;
    }
    return -1;
}

// 从文件读取 DFA
bool loadDFA(string filename) {
    ifstream fin(filename);
    if (!fin) {
        cout << "文件打开失败" << endl;
        return false;
    }

    string line, temp;

    // 第1行：字符集
    if (!getline(fin, line)) return false;
    stringstream ss1(line);
    char ch;
    while (ss1 >> ch) {
        alphabet.push_back(ch);
    }

    // 第2行：状态集
    if (!getline(fin, line)) return false;
    stringstream ss2(line);
    while (ss2 >> temp) {
        states.push_back(temp);
    }

    // 第3行：开始状态
    if (!getline(fin, line)) return false;
    stringstream ss3(line);
    ss3 >> startState;

    // 第4行：接受状态集
    if (!getline(fin, line)) return false;
    stringstream ss4(line);
    while (ss4 >> temp) {
        acceptStates.push_back(temp);
    }

    // 后面读取转移矩阵
    trans.resize(states.size(), vector<string>(alphabet.size()));

    for (int i = 0; i < states.size(); i++) {
        if (!getline(fin, line)) {
            cout << "转移矩阵行数不足" << endl;
            return false;
        }
        stringstream ss(line);
        for (int j = 0; j < alphabet.size(); j++) {
            if (!(ss >> trans[i][j])) {
                cout << "转移矩阵列数不足" << endl;
                return false;
            }
        }
    }

    fin.close();
    return true;
}

// 检查 DFA 是否合法
bool checkDFA() {
    if (alphabet.size() == 0) {
        cout << "字符集为空" << endl;
        return false;
    }

    if (states.size() == 0) {
        cout << "状态集为空" << endl;
        return false;
    }

    if (!inStates(startState)) {
        cout << "开始状态不在状态集中" << endl;
        return false;
    }

    if (acceptStates.size() == 0) {
        cout << "接受状态集为空" << endl;
        return false;
    }

    for (int i = 0; i < acceptStates.size(); i++) {
        if (!inStates(acceptStates[i])) {
            cout << "接受状态 " << acceptStates[i] << " 不在状态集中" << endl;
            return false;
        }
    }

    for (int i = 0; i < states.size(); i++) {
        for (int j = 0; j < alphabet.size(); j++) {
            if (!inStates(trans[i][j])) {
                cout << "转移矩阵中状态 " << trans[i][j] << " 不在状态集中" << endl;
                return false;
            }
        }
    }

    cout << "检查通过" << endl;
    return true;
}

// 判断字符串是否被接受
bool recognize(string str, bool show = true) {
    string cur = startState;

    if (show) {
        cout << "开始状态" << cur << endl;
    }

    for (int i = 0; i < str.size(); i++) {
        char ch = str[i];

        if (!inAlphabet(ch)) {
            if (show) cout << "字符 " << ch << " 不在字符集中，拒绝" << endl;
            return false;
        }

        int row = getStateIndex(cur);
        int col = getAlphabetIndex(ch);

        string next = trans[row][col];

        if (show) cout << cur << " --" << ch << "--> " << next << endl;

        cur = next;
    }

    if (show) cout << "结束状态: " << cur << endl;

    if (isAccept(cur)) {
        if (show) cout << "该字符串被接受" << endl;
        return true;
    } else {
        if (show) cout << "该字符串不被接受" << endl;
        return false;
    }
}

// 递归生成长度 <= N 的所有字符串
void generateStrings(string cur, int N) {
    if (cur.size() > N) return;

    if (recognize(cur, false)) {
        if (cur == "") cout << "ε" << endl;
        else cout << cur << endl;
    }

    for (int i = 0; i < alphabet.size(); i++) {
        generateStrings(cur + alphabet[i], N);
    }
}

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    string filename;
    cout << u8"输入DFA文件名：";
    cin >> filename;

    if (!loadDFA(filename)) return 0;
    if (!checkDFA()) return 0;

    int N;
    cout << u8"输入最大长度N：";
    cin >> N;

    cout << u8"长度 <= " << N << u8" 的合法字符串有：" << endl;
    generateStrings("", N);

    string s;
    cout << u8"输入要判断的字符串：";
    cin >> s;

    recognize(s, true);

    return 0;
}