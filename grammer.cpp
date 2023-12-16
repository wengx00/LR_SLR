#include "grammer.h"
#include <iostream>
#include <queue>
#include <sstream>

using namespace std;

Grammer::Grammer(string input) {
    vector<string> lines;
    int from = 0, i = 0;
    for (i = 0; i < input.size(); ++i) {
        if (input[i] == '\n') {
            // 换行分割语句
            lines.push_back(input.substr(from, i - from));
            from = i + 1;
        }
    }
    if (from != i)
        lines.push_back(input.substr(from));
    if (lines.empty()) {
        error = "未输入任何文法";
        return;
    }
    for (int i = 0; i < lines.size(); ++i) {
        string line = lines[i];
        string key;
        vector<string> raws;
        bool behind = false;
        for (int j = 0; j < line.size(); ++j) {
            if (line[j] == ' ')
                continue;
            if (line[j] == '-' && j < line.size() - 2 && line[j + 1] == '>') {
                // ->
                behind = true;
                j++;
                continue;
            }
            if (line[j] == '|') {
                if (!behind) {
                    error = "|符号不能出现在左值";
                    return;
                }
                // ｜需要分割
                formula[key].push_back(raws);
                raws.clear();
                continue;
            }
            // Common Symbol
            if (!behind) {
                if (key.size()) {
                    error = "文法左式不支持多字符";
                    return;
                }
                key += line[j];
                continue;
            }
            raws.push_back(string(1, line[j]));
        }
        if (!behind) {
            error = "文法输入有误";
            return;
        }
        if (!raws.empty()) {
            formula[key].push_back(raws);
        }
        if (i == 0)
            start = key;
    }
//    if (formula[start].size() > 1) {
        // 拓广文法
        formula[start + '\''].push_back(vector<string>(1, start));
        start = start + '\'';
//    }

    // 构建非终结符号集
    for (auto it = formula.begin(); it != formula.end(); ++it) {
        notEnd.insert(it->first);
    }

    // 构建终结符号集合
    for (auto& p : formula) {
        for (auto& raws : p.second) {
            for (auto& raw : raws) {
                if (!notEnd.count(raw) && !endSet.count(raw)) endSet.insert(raw);
            }
        }
    }

    // 初始化First集合元素
    initFirst();
    // 初始化Follow集合元素
    initFollow();
    // 构建DFA
    initRelation();
    // 判断是否SLR
    initIsSLR();
}

set<string> Grammer::getFirst(string key) {
    if (!notEnd.count(key)) {
        // 是终结节点
        set<string> res;
        res.insert(key);
        return res;
    }
    // 非终结节点，返回其First集
    return first[key];
}

set<string> Grammer::getFollow(string key) { return follow[key]; }

void Grammer::initFirst() {
    bool shouldUpdate = true;
    while (shouldUpdate) {
        shouldUpdate = false;

        for (auto &p : formula) {
            string key = p.first;                   // 非终结符
            vector<vector<string>> raws = p.second; // 产生式右侧
            for (const auto &raw : raws) {
                int cur = 0;
                for (; cur < raw.size(); ++cur) {
                    auto firstOfCur = getFirst(raw[cur]); // 当前元素的First集合

                    // 遍历当前First
                    for (auto &el : firstOfCur) {
                        // 除了EPSILON外，新增的元素都加入key的First
                        if (el != EPSILON && !first[key].count(el)) {
                            first[key].insert(el);
                            shouldUpdate = true;
                        }
                    }

                    // EPSILON不在cur的First，可以退出推导式右侧的遍历
                    if (!firstOfCur.count(EPSILON)) {
                        break;
                    }
                }
                // 右侧所有元素First都包含EPSILON，则key的First也应该包含EPSILON
                if (cur == raw.size() && !first[key].count(EPSILON)) {
                    first[key].insert(EPSILON);
                    shouldUpdate = true;
                }
            }
        }
    }
}

void Grammer::initFollow() {
    bool shouldUpdate = true;
    // start的Follow为END_FLAG
    follow[start].insert(END_FLAG);
    while (shouldUpdate) {
        shouldUpdate = false;

        for (auto &p : formula) {
            string key = p.first;
            vector<vector<string>> raws = p.second;
            // 遍历每一个推导式右侧
            for (const auto &raw : raws) {
                // 遍历每一个非终结符号
                for (int i = 0; i < raw.size(); ++i) {
                    if (!notEnd.count(raw[i]))
                        continue;
                    // 末尾
                    if (i == raw.size() - 1) {
                        for (const auto &el : getFollow(key)) {
                            if (!follow[raw[i]].count(el)) {
                                follow[raw[i]].insert(el);
                                shouldUpdate = true;
                            }
                        }
                        continue;
                    }
                    // 非末尾，获取后续元素的First集合
                    set<string> firstOfBehind;
                    int cur = i + 1;
                    for (; cur < raw.size(); ++cur) {
                        set<string> firstOfCur = getFirst(raw[cur]);
                        for (const auto &el : firstOfCur) {
                            if (el != EPSILON)
                                firstOfBehind.insert(el);
                        }
                        if (!firstOfCur.count(EPSILON)) {
                            // 不含EPSILON，First终止
                            break;
                        }
                    }
                    if (cur == raw.size()) {
                        // 每个元素的First都包含Epsilon
                        firstOfBehind.insert(EPSILON);
                    }
                    // 更新Follow集合
                    for (const auto &el : firstOfBehind) {
                        if (el != EPSILON && !follow[raw[i]].count(el)) {
                            follow[raw[i]].insert(el);
                            shouldUpdate = true;
                        }
                    }
                    if (firstOfBehind.count(EPSILON)) {
                        // 包含EPSILON，Follow集合包含产生式左侧的Follow集合
                        for (const auto &el : getFollow(key)) {
                            // 消除曾经的父级的EPSILON
                            follow[raw[i]].erase(EPSILON);
                            if (!follow[raw[i]].count(el)) {
                                follow[raw[i]].insert(el);
                                shouldUpdate = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Grammer::extend(int state) {
    vector<Node> &nodes = dfa[state];
    for (int i = 0; i < nodes.size(); ++i) {
        Node &node = nodes[i];
        if (node.type == NodeType::BACKWARD)
            continue; // 跳过规约节点
        string cur = formula[node.key][node.rawsIndex][node.rawIndex]; // 指示的符号
        if (!notEnd.count(cur))
            continue; // 终结字符不可扩展
        vector<vector<string>> &rawsOfCur = formula[cur]; // 非终结字符为Key的推导式
        for (int j = 0; j < rawsOfCur.size(); ++j) {
            int rawOffset = 0;
            for (; rawOffset < rawsOfCur[j].size(); ++rawOffset) {
                // 寻找到非空字符
                if (rawsOfCur[j][rawOffset] != EPSILON)
                    break;
            }
            // 新增节点，指示了Key对应的第i个推导式的第rawOffset个字符
            // 如果开头存在了EPSILON，则节点类型为规约
            Node instance(cur,
                          rawOffset == 0 ? NodeType::FORWARD : NodeType::BACKWARD, j,
                          rawOffset);
            // 无重复则扩展state指示的dfa节点
            if (!count(nodes.begin() /*+ i*/, nodes.end(), instance)) {
                nodes.push_back(instance);
            }
        }
    }
}

void Grammer::initRelation() {
    // 初始节点 => start指示的推导式的第一条的第一个符号
    vector<Node> beginState;
    beginState.push_back(Node(start, NodeType::FORWARD, 0, 0));
    dfa.push_back(beginState);
    isSLR = true; // 暂时先是
    // 遍历每一个DFA节点
    for (int cur = 0; cur < dfa.size(); ++cur) {
        // forwards[cur]和backwards[cur]分别记录了移进和规约关系
        extend(cur); // 扩展当前DFA节点(可能右侧项目含有非终结符号)
        // 遍历DFA节点上的每一个项目
        for (int it = 0; it < dfa[cur].size(); ++it) {
            Node &item = dfa[cur][it]; // 取出当前项
            if (item.type == NodeType::BACKWARD) {
                // 规约项
                set<string> followOfItem = getFollow(item.key);
                for (const auto &el : followOfItem) {
                    if (backwards[cur].count(el)) {
                        // 存在交集，非SLR(1)
                        isSLR = false;
                        stringstream ss;
                        ss << "第" << cur << "个节点中规约项目的Follow集合有交集\n";
                        reason += ss.str();
                    }
                    backwards[cur][el] = it;
                }
                continue;
            }
            // 移进项
            string raw = formula[item.key][item.rawsIndex][item.rawIndex];
            // 移进新的节点
            Node instance(item.key, NodeType::FORWARD, item.rawsIndex,
                          item.rawIndex + 1);
            if (instance.rawIndex >=
                formula[instance.key][instance.rawsIndex].size()) {
                // 超过了该推导式的结尾 -> 变成规约节点
                instance.type = NodeType::BACKWARD;
            }
            if (forwards[cur].count(raw)) {
                // 已经存在该移进关系
                int target = forwards[cur][raw];
                vector<Node> &next = dfa[target];
                if (!count(next.begin(), next.end(), instance)) {
                    // 如果下一DFA节点中未存在该Instance状态 -> 加入下一DFA节点中
                    dfa[target].push_back(instance);
                }
                continue;
            }
            // 未存在该移进关系
            int target = 0; // 移进的DFA目标
            for (; target < dfa.size(); ++target) {
                vector<Node> &next = dfa[target]; // 下一移进DFA节点
                if (count(next.begin(), next.end(), instance))
                    break; // 该Instance状态在某个DFA节点中被找到
            }
            if (target == dfa.size()) {
                // 该Instance状态不存在于任何DFA节点中 -> 新增一个DFA节点
                vector<Node> state;
                state.push_back(instance);
                dfa.push_back(state);
            }
            // 加入移进关系
            forwards[cur][raw] = target;
        }
    }
}

void Grammer::initIsSLR() {
    // DFA图构建完成后 -> 判断移进规约是否冲突
    if (isSLR) {
        stringstream ss;
        for (int cur = 0; cur < dfa.size(); ++cur) {
            set<string> curForwards, curBackwards, duplicates;
            for (auto p : forwards[cur]) {
                curForwards.insert(p.first);
            }
            for (auto p : backwards[cur]) {
                curBackwards.insert(p.first);
            }
            set_intersection(curForwards.begin(), curForwards.end(),
                             curBackwards.begin(), curBackwards.end(),
                             inserter(duplicates, duplicates.begin()));
            if (!duplicates.empty()) {
                // 交集不空 不为SLR
                isSLR = false;
                ss.str("");
                ss.clear();
                ss << "第" << cur
                   << "个节点的移进项First集合和规约项Follow集合有交集\n";
                reason += ss.str();
            }
        }
    }
}

bool Grammer::slr() { return isSLR; }
bool Grammer::bad() { return !error.empty(); }
string Grammer::getReason() { return reason; }
string Grammer::getError() { return error; }

set<string> Grammer::getNotEnd() { return notEnd; }
set<string> Grammer::getEnd() { return endSet; }

string Grammer::getStart() {
    return start;
}

string Grammer::getExtraGrammer() {
    stringstream ss;
    map<string, bool> visited;
    queue<string> ready;
    ready.push(start);
    while (ready.size()) {
        string cur = ready.front();
        ready.pop();
        if (visited[cur])
            continue;
        visited[cur] = true;
        vector<vector<string>> rawsOfCur = formula[cur];
        for (auto &raw : rawsOfCur) {
            ss << cur << " -> ";
            for (auto &token : raw) {
                if (notEnd.count(token)) {
                    ready.push(token);
                }
                ss << token;
            }
            ss << '\n';
        }
    }
    return ss.str();
}

vector<vector<Node>> Grammer::getDfa() { return dfa; }

map<string, vector<vector<string>>> Grammer::getFormula() {
    return formula;
}

int Grammer::forward(int state, string key) {
    if (forwards[state].count(key)) {
        return forwards[state][key];
    }
    return -1;
}

int Grammer::backward(int state, string key) {
    if (backwards[state].count(key)) {
        return backwards[state][key];
    }
    return -1;
}

ParsedResult Grammer::parse(string input) {
    string str;
    for (auto& s : input) {
        if (s != ' ' && s != '\n') str += s;
    }
    ParsedResult result;
    string output;
    queue<string> inputs;
    vector<int> stash;

    for (const char& c : input) {
        inputs.push(string(1, c));
    }
    inputs.push(END_FLAG);
    int state = 0; // 当前DFA状态编号
    int count = 0;
    stringstream ss;
    for (;;) {
        ss.str("");
        ss.clear();
        map<string, int>& curForwards = forwards[state]; // 当前所有可移进状态
        map<string, int>& curBackwards = backwards[state]; // 当前所有可规约状态

        string token = inputs.front(); // 当前输入的字符
        stash.push_back(state); // 当前状态入栈

        if (curForwards.count(token)) {
            // 找到了移进关系
            inputs.pop();
            ++count;
            int next = curForwards[token]; // 下一个状态
            ss << "在状态" << state << "通过" << token << "移进到状态" << next;
            state = next;
            output += token;
            result.outputs.push_back(output);
            result.routes.push_back(ss.str());
            result.inputs.push_back(str.substr(count));
            continue;
        }
        if (curBackwards.count(token)) {
            // 找到了规约关系
            int target = curBackwards[token];
            ss << "在状态" << state << "通过" << token << "规约到状态" << target;
            Node& node = dfa[state][target];
            if (count >= str.size()) {
                result.inputs.push_back("");
            }
            else {
                result.inputs.push_back(str.substr(count));
            }
            result.routes.push_back(ss.str());
            if (node.key == start) {
                // 接收
                result.accept = true;
                result.outputs.push_back(start);
                break;
            }
            vector<string>& raws = formula[node.key][node.rawsIndex];
            int useful = 0;
            for (int i = 0; i < raws.size(); ++i) {
                // 找到不是EPSILON的大小
                if (raws[i] != EPSILON) useful++;
            }
            if (useful > 0) {
                output.erase(output.end() - useful, output.end());
                stash.erase(stash.end() - useful, stash.end());
            }
            int next = forwards[stash[stash.size() - 1]][node.key];
            state = next;
            output += node.key;
            result.outputs.push_back(output);
            continue;
        }
        // 找不到关系，出错
        ss << "在状态" << state << "上找不到" << token << "对应的移进/规约关系";
            result.error = ss.str();
        break;
    }
    return result;
}
