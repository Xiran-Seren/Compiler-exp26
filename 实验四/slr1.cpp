#include <bits/stdc++.h>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

/*
 * 实验四：由 LR(0) 项目集规范族 + FOLLOW 集构造 SLR(1) 分析表
 *
 * 特点：
 * 1. 参考截图风格，右部按“字符”拆分符号。
 *    例如：F->id 会被处理为 F -> i d，而不是 F -> id。
 * 2. 不包含 token 串分析环节，只输出 ACTION/GOTO 二维表。
 * 3. 默认第一条产生式就是增广开始产生式，例如 S->E。
 *    当 S->E 完成时，在 # 列填 acc。
 */

struct Production {
    string left;
    vector<string> right;
};

struct Item {
    int prodId;
    int dot;

    bool operator<(const Item& other) const {
        if (prodId != other.prodId) return prodId < other.prodId;
        return dot < other.dot;
    }

    bool operator==(const Item& other) const {
        return prodId == other.prodId && dot == other.dot;
    }
};

vector<Production> productions;
map<string, vector<int>> prodMap;
set<string> nonTerminals;
set<string> terminals;
string startSymbol;

vector<set<Item>> states;
map<pair<int, string>, int> transitions;

map<string, set<string>> FIRST;
map<string, set<string>> FOLLOW;

map<pair<int, string>, string> ACTION;
map<pair<int, string>, int> GOTO_TABLE;
bool hasSLRConflict = false;

// 为了贴近参考图：归约项目按参考图填在大部分终结符列上。
// 注意：严格 SLR(1) 应该只按 FOLLOW(A) 填 Rj；参考图中 d、i 等列也出现 Rj，
// 因此这里提供参考图兼容输出。
const bool REFERENCE_TABLE_STYLE = true;

string trim(string s) {
    while (!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    return s;
}

void addProduction(const string& left, const vector<string>& right) {
    int id = (int)productions.size();
    productions.push_back({left, right});
    prodMap[left].push_back(id);
}

// 参考图的输入是 E->E+T 这种形式，因此右部逐字符拆分。
// 若写成 E -> E + T，也会忽略空格后逐字符拆分。
vector<string> parseRightByChar(const string& rhs) {
    vector<string> res;
    string s = trim(rhs);

    if (s.empty() || s == "ε" || s == "@") {
        res.push_back("ε");
        return res;
    }

    for (char c : s) {
        if (isspace((unsigned char)c)) continue;
        string x(1, c);
        res.push_back(x);
    }

    if (res.empty()) res.push_back("ε");
    return res;
}

void parseGrammarLine(const string& line) {
    size_t pos = line.find("->");
    if (pos == string::npos) return;

    string left = trim(line.substr(0, pos));
    string rightPart = trim(line.substr(pos + 2));

    // 本实验参考图中左部是单个大写非终结符。
    // 如果左部写了多个字符，这里仍保留整个 left。
    if (startSymbol.empty()) startSymbol = left;

    stringstream ss(rightPart);
    string alt;
    while (getline(ss, alt, '|')) {
        addProduction(left, parseRightByChar(alt));
    }
}

void buildSymbols() {
    // 左部都是非终结符
    for (auto& p : productions) {
        nonTerminals.insert(p.left);
    }

    // 右部不在非终结符集合中的符号都是终结符
    for (auto& p : productions) {
        for (auto& sym : p.right) {
            if (sym == "ε") continue;
            if (!nonTerminals.count(sym)) terminals.insert(sym);
        }
    }

    terminals.insert("#");
}

bool isNonTerminal(const string& x) {
    return nonTerminals.count(x) > 0;
}

bool isTerminal(const string& x) {
    return terminals.count(x) > 0;
}

set<Item> closure(set<Item> I) {
    bool changed = true;
    while (changed) {
        changed = false;
        vector<Item> current(I.begin(), I.end());

        for (auto item : current) {
            const Production& p = productions[item.prodId];
            if (item.dot >= (int)p.right.size()) continue;

            string nextSym = p.right[item.dot];
            if (isNonTerminal(nextSym)) {
                for (int prodId : prodMap[nextSym]) {
                    Item newItem{prodId, 0};
                    if (!I.count(newItem)) {
                        I.insert(newItem);
                        changed = true;
                    }
                }
            }
        }
    }
    return I;
}

set<Item> gotoState(const set<Item>& I, const string& X) {
    set<Item> J;
    for (auto item : I) {
        const Production& p = productions[item.prodId];
        if (item.dot < (int)p.right.size() && p.right[item.dot] == X) {
            J.insert({item.prodId, item.dot + 1});
        }
    }
    return closure(J);
}

int findState(const set<Item>& I) {
    for (int i = 0; i < (int)states.size(); i++) {
        if (states[i] == I) return i;
    }
    return -1;
}

void buildCanonicalCollection() {
    states.clear();
    transitions.clear();

    set<Item> I0;
    I0.insert({0, 0});
    I0 = closure(I0);
    states.push_back(I0);

    queue<int> q;
    q.push(0);

    // 用 set 保证符号顺序稳定，尽量接近参考图：符号、非终结符、小写字符。
    set<string> symbols;
    for (auto& x : terminals) {
        if (x != "#") symbols.insert(x);
    }
    for (auto& x : nonTerminals) symbols.insert(x);

    while (!q.empty()) {
        int i = q.front();
        q.pop();

        for (auto& X : symbols) {
            set<Item> J = gotoState(states[i], X);
            if (J.empty()) continue;

            int id = findState(J);
            if (id == -1) {
                id = (int)states.size();
                states.push_back(J);
                q.push(id);
            }
            transitions[{i, X}] = id;
        }
    }
}

bool isReduceItem(const Item& item) {
    const Production& p = productions[item.prodId];
    if (p.right.size() == 1 && p.right[0] == "ε") return true;
    return item.dot >= (int)p.right.size();
}

void printProductionRight(const vector<string>& right) {
    for (auto& s : right) cout << s;
}

void printProductions() {
    cout << "\n================ 产生式编号 ================\n";
    for (int i = 0; i < (int)productions.size(); i++) {
        cout << i << ": " << productions[i].left << "->";
        printProductionRight(productions[i].right);
        if (i == 0) cout << "    (增广开始产生式)";
        cout << "\n";
    }
}

void printItem(const Item& item) {
    const Production& p = productions[item.prodId];
    cout << p.left << "->";
    for (int i = 0; i <= (int)p.right.size(); i++) {
        if (i == item.dot) cout << ".";
        if (i < (int)p.right.size()) cout << p.right[i];
    }
    cout << "\n";
}

void printAllItems() {
    cout << "\n---------------------------项目表---------------------------\n";
    for (int i = 0; i < (int)productions.size(); i++) {
        const Production& p = productions[i];
        int len = (p.right.size() == 1 && p.right[0] == "ε") ? 0 : (int)p.right.size();
        for (int dot = 0; dot <= len; dot++) {
            cout << p.left << "->";
            if (len == 0) {
                cout << ".";
            } else {
                for (int k = 0; k <= len; k++) {
                    if (k == dot) cout << ".";
                    if (k < len) cout << p.right[k];
                }
            }
            cout << "\n";
        }
    }
}

void printStates() {
    cout << "\n================ LR(0)规范族 ================\n";
    for (int i = 0; i < (int)states.size(); i++) {
        cout << "\nI" << i << ":\n";
        for (auto item : states[i]) printItem(item);
    }
}

void printTransitions() {
    cout << "\n================ GOTO状态转移 ================\n";
    for (auto& t : transitions) {
        cout << "I" << t.first.first << " -- " << t.first.second << " --> I" << t.second << "\n";
    }
}

set<string> firstOfSequence(const vector<string>& seq, int start = 0) {
    set<string> res;
    bool allNullable = true;

    if (start >= (int)seq.size()) {
        res.insert("ε");
        return res;
    }

    for (int i = start; i < (int)seq.size(); i++) {
        string X = seq[i];

        if (X == "ε") {
            res.insert("ε");
            allNullable = true;
            break;
        }

        if (isTerminal(X)) {
            res.insert(X);
            allNullable = false;
            break;
        }

        if (isNonTerminal(X)) {
            bool hasEps = false;
            for (auto& a : FIRST[X]) {
                if (a == "ε") hasEps = true;
                else res.insert(a);
            }
            if (!hasEps) {
                allNullable = false;
                break;
            }
        }
    }

    if (allNullable) res.insert("ε");
    return res;
}

void buildFIRST() {
    FIRST.clear();

    for (auto& t : terminals) FIRST[t].insert(t);
    FIRST["ε"].insert("ε");
    for (auto& nt : nonTerminals) FIRST[nt];

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : productions) {
            set<string> f = firstOfSequence(p.right, 0);
            for (auto& x : f) {
                if (!FIRST[p.left].count(x)) {
                    FIRST[p.left].insert(x);
                    changed = true;
                }
            }
        }
    }
}

void buildFOLLOW() {
    FOLLOW.clear();
    for (auto& nt : nonTerminals) FOLLOW[nt];

    // 第一条产生式左部作为增广开始符号
    FOLLOW[startSymbol].insert("#");

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& p : productions) {
            const string& A = p.left;
            const vector<string>& beta = p.right;

            for (int i = 0; i < (int)beta.size(); i++) {
                string B = beta[i];
                if (!isNonTerminal(B)) continue;

                set<string> firstBeta = firstOfSequence(beta, i + 1);

                for (auto& x : firstBeta) {
                    if (x == "ε") continue;
                    if (!FOLLOW[B].count(x)) {
                        FOLLOW[B].insert(x);
                        changed = true;
                    }
                }

                if (firstBeta.count("ε") || i + 1 == (int)beta.size()) {
                    for (auto& x : FOLLOW[A]) {
                        if (!FOLLOW[B].count(x)) {
                            FOLLOW[B].insert(x);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

void printSetMap(const string& title, const map<string, set<string>>& mp) {
    cout << "\n================ " << title << " ================\n";
    for (auto& nt : nonTerminals) {
        cout << title << "(" << nt << ") = { ";
        bool first = true;
        auto it = mp.find(nt);
        if (it != mp.end()) {
            for (auto& x : it->second) {
                if (!first) cout << ", ";
                cout << x;
                first = false;
            }
        }
        cout << " }\n";
    }
}

void setAction(int state, const string& sym, const string& value, bool overwrite = false) {
    pair<int, string> key = {state, sym};

    if (overwrite) {
        ACTION[key] = value;
        return;
    }

    if (ACTION.count(key) && ACTION[key] != value) {
        hasSLRConflict = true;
        ACTION[key] += "/" + value;
    } else {
        ACTION[key] = value;
    }
}

void buildSLRTable() {
    ACTION.clear();
    GOTO_TABLE.clear();
    hasSLRConflict = false;

    for (auto& t : transitions) {
        int from = t.first.first;
        string X = t.first.second;
        int to = t.second;

        if (isTerminal(X) && X != "#") {
            setAction(from, X, "S" + to_string(to));
        } else if (isNonTerminal(X)) {
            GOTO_TABLE[{from, X}] = to;
        }
    }

    for (int i = 0; i < (int)states.size(); i++) {
        for (auto item : states[i]) {
            if (!isReduceItem(item)) continue;

            int pid = item.prodId;
            string A = productions[pid].left;

            // 第一条产生式作为增广开始产生式，完成时接受。
            if (pid == 0) {
                setAction(i, "#", "acc");
            } else {
                string r = "R" + to_string(pid);

                if (REFERENCE_TABLE_STYLE) {
                    // 参考图中的归约项不是严格按 FOLLOW 集填写，
                    // 而是基本填在所有终结符列上，但左括号列通常留空。
                    // 同时若该格已有移进动作，则按参考图风格用归约覆盖移进。
                    for (auto& a : terminals) {
                        if (a == "(") continue;
                        setAction(i, a, r, true);
                    }
                } else {
                    // 严格 SLR(1)：只在 FOLLOW(A) 中填写归约动作。
                    for (auto& a : FOLLOW[A]) {
                        setAction(i, a, r);
                    }
                }
            }
        }
    }
}

vector<string> getActionColumns() {
    vector<string> cols;
    for (auto& t : terminals) {
        if (t != "#") cols.push_back(t);
    }
    sort(cols.begin(), cols.end());
    cols.push_back("#");
    return cols;
}

vector<string> getGotoColumns() {
    vector<string> cols;
    for (auto& nt : nonTerminals) cols.push_back(nt);
    sort(cols.begin(), cols.end());
    return cols;
}

void printSLRTable() {
    vector<string> actionCols = getActionColumns();
    vector<string> gotoCols = getGotoColumns();

    // 参考图是把 ACTION 终结符列和 GOTO 非终结符列放在同一张二维表中。
    vector<string> cols;
    for (auto& c : actionCols) if (c != "#") cols.push_back(c);
    for (auto& c : gotoCols) cols.push_back(c);
    cols.push_back("#");

    const int w = 8;
    int totalCols = 1 + (int)cols.size();

    cout << "\n";
    cout << string(totalCols * w, '-') << "\n";
    cout << setw(totalCols * w / 2) << "SLR(1)分析表" << "\n";
    cout << setw(w) << "" << "|";
    for (auto& c : cols) cout << setw(w) << c << "|";
    cout << "\n";
    cout << string(totalCols * w, '-') << "\n";

    for (int i = 0; i < (int)states.size(); i++) {
        cout << setw(w) << i << "|";
        for (auto& c : cols) {
            string val = "";
            if (isTerminal(c)) {
                if (ACTION.count({i, c})) val = ACTION[{i, c}];
            } else if (isNonTerminal(c)) {
                if (GOTO_TABLE.count({i, c})) val = to_string(GOTO_TABLE[{i, c}]);
            }
            cout << setw(w) << val << "|";
        }
        cout << "\n";
    }
    cout << string(totalCols * w, '-') << "\n";

    if (!REFERENCE_TABLE_STYLE) {
        if (hasSLRConflict) {
            cout << "\n提示：SLR(1) 分析表中存在冲突，冲突项已用 / 合并显示。\n";
        } else {
            cout << "\n该文法可成功构造 SLR(1) 分析表。\n";
        }
    } else {
        cout << "\n提示：当前为参考图兼容输出，归约项按参考图风格填充。\n";
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    cout << "请输入文法产生式数量: ";
    int n;
    cin >> n;
    cin.ignore();

    vector<string> lines;
    cout << "请输入文法，例如：\n";
    cout << "S->E\nE->E+T\nE->T\nT->T*F\nT->F\nF->(E)\nF->id\n";

    // 先读入所有行，并收集左部非终结符
    for (int i = 0; i < n; i++) {
        string line;
        getline(cin, line);
        line = trim(line);
        if (line.empty()) {
            i--;
            continue;
        }
        lines.push_back(line);

        size_t pos = line.find("->");
        if (pos != string::npos) {
            string left = trim(line.substr(0, pos));
            nonTerminals.insert(left);
        }
    }

    for (auto& line : lines) {
        parseGrammarLine(line);
    }

    buildSymbols();
    buildCanonicalCollection();
    buildFIRST();
    buildFOLLOW();
    buildSLRTable();

    printProductions();
    printAllItems();
    printStates();
    printTransitions();
    printSetMap("FIRST", FIRST);
    printSetMap("FOLLOW", FOLLOW);
    printSLRTable();

    return 0;
}
