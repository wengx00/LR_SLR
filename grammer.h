#include <vector>
#include <set>
#include <map>
#include <string>

#define EPSILON "@"
#define END_FLAG "$"

enum NodeType {
    FORWARD,
    BACKWARD
};

struct Node {
    std::string key; // 所属非终结符号
    NodeType type; // 递进还是规约
    int rawsIndex; // 推导式编号
    int rawIndex; // 推导式内编号

    Node(std::string key, NodeType type, int rawsIndex, int rawIndex): key(key), type(type), rawsIndex(rawsIndex), rawIndex(rawIndex) {}

    bool operator==(const Node& node) const {
        return node.key == key && node.type == type && node.rawsIndex == rawsIndex && node.rawIndex == rawIndex;
    }
};

// 句子分析结果
struct ParsedResult {
    std::vector<std::string> outputs;
    std::vector<std::string> inputs;
    std::vector<std::string> routes;

    bool accept = false; // 是否接受
    std::string error = ""; // 错误信息，空则无出错
};

class Grammer {
private:
    std::map<std::string, std::vector<std::vector<std::string> > > formula; // 分式
    std::string start; // 起始
    std::map<std::string, std::set<std::string> > first; // FIRST集合元素
    std::map<std::string, std::set<std::string> > follow; // FOLLOW集合元素
    std::set<std::string> notEnd; // 非终结符号集合
    std::set<std::string> endSet; // 终结符号集合
    std::string error; // 是否有错误
    std::string reason; // 为什么不是SLR
    bool isSLR = false; // 是否SLR(1)

    std::vector<std::vector<Node> > dfa; // DFA图
    std::map<int, std::map<std::string, int> > forwards; // 移进关系
    std::map<int, std::map<std::string, int> > backwards; // 规约关系


    void initFirst(); // 生成First集合
    void initFollow(); // 生成Follow集合
    void extend(std::vector<Node>&); // 扩展DFA某节点的推导式
    void extend(int); // 扩展DFA某节点的推导式
    void initRelation(); // 生成DFA图、规约关系
    void initIsSLR(); // 初始化是否SLR(1)
    int findState(std::vector<Node>&); // 是否包含此DFA节点
public:
    Grammer(std::string);

    std::set<std::string> getFirst(std::string); // 获取节点的First集合
    std::set<std::string> getFollow(std::string); // 获取节点的Follow集合
    std::set<std::string> getNotEnd(); // 获取非终结符号集
    std::set<std::string> getEnd(); // 获取终结符号集
    std::string getExtraGrammer(); // 获取拓广文法
    std::map<std::string, std::vector<std::vector<std::string> > > getFormula(); // 获取分式
    bool slr();
    bool bad();
    std::string getReason();
    std::string getError();
    std::vector<std::vector<Node> > getDfa();
    int forward(int, std::string);
    int backward(int, std::string);
    std::string getStart();

    ParsedResult parse(std::string);
};
