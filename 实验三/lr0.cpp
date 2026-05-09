#include <bits/stdc++.h>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

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
string augmentedStart;

vector<set<Item>> states;
map<pair<int, string>, int> transitions;

vector<string> split(const string& s) {
    stringstream ss(s);
    vector<string> res;
    string x;
    while (ss >> x) res.push_back(x);
    return res;
}

string trim(string s) {
    while (!s.empty() && isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && isspace(s.back())) s.pop_back();
    return s;
}

void addProduction(const string& left, const vector<string>& right) {
    int id = productions.size();
    productions.push_back({left, right});
    prodMap[left].push_back(id);
}

void parseGrammarLine(const string& line) {
    size_t pos = line.find("->");
    if (pos == string::npos) return;

    string left = trim(line.substr(0, pos));
    string rightPart = trim(line.substr(pos + 2));

    if (startSymbol.empty()) startSymbol = left;
    nonTerminals.insert(left);

    stringstream ss(rightPart);
    string segment;
    vector<string> alternatives;

    string temp;
    while (getline(ss, segment, '|')) {
        alternatives.push_back(trim(segment));
    }

    for (auto& alt : alternatives) {
        vector<string> right = split(alt);
        if (right.empty()) right.push_back("ε");
        addProduction(left, right);
    }
}

void buildSymbols() {
    for (auto& p : productions) {
        for (auto& sym : p.right) {
            if (sym == "ε") continue;
            if (!nonTerminals.count(sym)) {
                terminals.insert(sym);
            }
        }
    }
}

set<Item> closure(set<Item> I) {
    bool changed = true;

    while (changed) {
        changed = false;
        vector<Item> current(I.begin(), I.end());

        for (auto item : current) {
            Production& p = productions[item.prodId];

            if (item.dot >= (int)p.right.size()) continue;

            string nextSym = p.right[item.dot];

            if (nonTerminals.count(nextSym)) {
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
        Production& p = productions[item.prodId];

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
    set<Item> I0;
    I0.insert({0, 0});
    I0 = closure(I0);

    states.push_back(I0);

    queue<int> q;
    q.push(0);

    set<string> symbols;
    for (auto& x : terminals) symbols.insert(x);
    for (auto& x : nonTerminals) symbols.insert(x);

    while (!q.empty()) {
        int i = q.front();
        q.pop();

        for (auto& X : symbols) {
            set<Item> J = gotoState(states[i], X);
            if (J.empty()) continue;

            int id = findState(J);
            if (id == -1) {
                id = states.size();
                states.push_back(J);
                q.push(id);
            }

            transitions[{i, X}] = id;
        }
    }
}

void printItem(const Item& item) {
    Production& p = productions[item.prodId];

    cout << p.left << " -> ";

    for (int i = 0; i <= (int)p.right.size(); i++) {
        if (i == item.dot) cout << "· ";
        if (i < (int)p.right.size()) cout << p.right[i] << " ";
    }

    cout << "\n";
}

void printStates() {
    cout << "\n========== LR(0) 项目集规范族 ==========\n";

    for (int i = 0; i < (int)states.size(); i++) {
        cout << "\nI" << i << ":\n";
        for (auto item : states[i]) {
            printItem(item);
        }
    }
}

void printTransitions() {
    cout << "\n========== GOTO 状态转移 ==========\n";

    for (auto& t : transitions) {
        cout << "I" << t.first.first
             << " -- " << t.first.second
             << " --> I" << t.second << "\n";
    }
}

bool isReduceItem(const Item& item) {
    Production& p = productions[item.prodId];
    return item.dot >= (int)p.right.size();
}

bool isShiftItem(const Item& item) {
    Production& p = productions[item.prodId];
    return item.dot < (int)p.right.size();
}

void checkLR0() {
    bool ok = true;

    cout << "\n========== LR(0) 冲突检查 ==========\n";

    for (int i = 0; i < (int)states.size(); i++) {
        int reduceCount = 0;
        int shiftCount = 0;

        for (auto item : states[i]) {
            if (isReduceItem(item)) reduceCount++;
            if (isShiftItem(item)) shiftCount++;
        }

        if (reduceCount > 0 && shiftCount > 0) {
            ok = false;
            cout << "I" << i << " 存在 移进-归约冲突\n";
        }

        if (reduceCount > 1) {
            ok = false;
            cout << "I" << i << " 存在 归约-归约冲突\n";
        }
    }

    if (ok) {
        cout << "该文法是 LR(0) 文法\n";
    } else {
        cout << "该文法不是 LR(0) 文法\n";
    }
}

void printProductions() {
    cout << "\n========== 增广后的文法 ==========\n";

    for (int i = 0; i < (int)productions.size(); i++) {
        cout << i << ": " << productions[i].left << " -> ";
        for (auto& s : productions[i].right) cout << s << " ";
        cout << "\n";
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
    cout << "请输入文法，每个符号之间用空格分隔，例如 E -> E + T | T\n";

    for (int i = 0; i < n; i++) {
        string line;
        getline(cin, line);
        lines.push_back(line);
    }

    for (auto& line : lines) {
        size_t pos = line.find("->");
        if (pos != string::npos) {
            string left = trim(line.substr(0, pos));
            if (startSymbol.empty()) startSymbol = left;
            nonTerminals.insert(left);
        }
    }

    augmentedStart = startSymbol + "'";
    nonTerminals.insert(augmentedStart);

    addProduction(augmentedStart, {startSymbol});

    for (auto& line : lines) {
        parseGrammarLine(line);
    }

    buildSymbols();

    printProductions();

    buildCanonicalCollection();

    printStates();
    printTransitions();
    checkLR0();

    return 0;
}