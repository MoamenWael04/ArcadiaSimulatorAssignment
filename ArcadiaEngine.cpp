// ArcadiaEngine.cpp - STUDENT TEMPLATE
// TODO: Implement all the functions below according to the assignment requirements

#include "ArcadiaEngine.h"
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <functional>
#include <cmath>
#include <climits>
using namespace std;


#define table_size 101
#define empty_slot -1
#define deleted_slot -2

// =========================================================
// PART A: DATA STRUCTURES (Concrete Implementations)
// =========================================================

// --- 1. PlayerTable (Double Hashing) ---
/* ================= PLAYER ================= */
struct Player {
    int id;
    string name;
    int score;

    Player() : id(empty_slot), name(""), score(0) {}
    Player(int i, string n, int s) : id(i), name(n), score(s) {}
};

class ConcretePlayerTable : public PlayerTable {
private:
    vector<Player> table;
    int currentSize;

    int hash1(int key) {
        return key % table_size;
    }


    int hash2(int key) {
        const int PRIME = 97;
        return PRIME - (key % PRIME);
    }

public:
    ConcretePlayerTable() {
        table.resize(table_size);
        currentSize = 0;
    }

    void insert(int playerID, string name) override {
        if (currentSize == table_size) {
            cout << "Table is Full\n";
            return;
        }

        int index = hash1(playerID);
        int step  = hash2(playerID);

        for (int i = 0; i < table_size; i++) {
            int probe = (index + i * step) % table_size;

            if(table[probe].id == playerID){
                table[probe].name = name;
                return;
            }

            if (table[probe].id == empty_slot || table[probe].id == deleted_slot) {
                //cout << "New to player table : "<<name<<" with id "<<playerID<<endl;
                table[probe].id = playerID;
                table[probe].name = name;
                table[probe].score = 0;
                currentSize++;
                return;
            }
        }

        cout << "Table is Full\n";
    }

    string search(int playerID) override {
        int index = hash1(playerID);
        int step  = hash2(playerID);

        for (int i = 0; i < table_size; i++) {
            int probe = (index + i * step) % table_size;

            if (table[probe].id == empty_slot)
                return "Not Found";

            if (table[probe].id == playerID)
                return table[probe].name;
        }
        return "Not Found";

    }
};

// --- 2. Leaderboard (Skip List) ---
class SkipNode {
public:
    Player player;
    vector<SkipNode*> forward;

    SkipNode(Player p, int level)
        : player(p), forward(level + 1, nullptr) {}
};

class ConcreteLeaderboard : public Leaderboard {
private:
    const int MAX_LEVEL = 6;
    const float P = 0.5f;

    int currentLevel;
    SkipNode* header;
    PlayerTable* playerTable;

    int randomLevel() {
        int lvl = 0;
        while (((double)rand() / RAND_MAX) < P && lvl < MAX_LEVEL)
            lvl++;
        return lvl;
    }

    bool isBefore(const Player& a, const Player& b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.id < b.id;
    }

public:

    ConcreteLeaderboard(PlayerTable* pt) {
        playerTable = pt;
        currentLevel = 0;
        header = new SkipNode(Player(-1, "", INT_MAX), MAX_LEVEL);
    }

     ~ConcreteLeaderboard() {
    SkipNode* current = header;

    while (current != nullptr) {
        SkipNode* next = current->forward[0];
        delete current;
        current = next;
        }
    }


    void addScore(int playerID, int score) override {

//        if (playerTable->search(playerID) == "Not Found"){
//                cout << "Player with id : "<<playerID<<" was not found in the player table\n";
//                return;
//        }

        removePlayer(playerID);

        Player newPlayer(playerID, "", score);
        vector<SkipNode*> update(MAX_LEVEL + 1);
        SkipNode* current = header;

        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] &&
                   isBefore(current->forward[i]->player, newPlayer)) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        int newLevel = randomLevel();
        if (newLevel > currentLevel) {
            for (int i = currentLevel + 1; i <= newLevel; i++)
                update[i] = header;
            currentLevel = newLevel;
        }

        SkipNode* node = new SkipNode(newPlayer, newLevel);
        for (int i = 0; i <= newLevel; i++) {
            node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = node;
        }
    }

    void removePlayer(int playerID) override {
        SkipNode* prev = header;

        while (prev->forward[0]) {
            if (prev->forward[0]->player.id == playerID) {
                SkipNode* target = prev->forward[0];

                for (int i = 0; i <= currentLevel; i++) {
                    if (prev->forward[i] == target)
                        prev->forward[i] = target->forward[i];
                }

                delete target;

                while (currentLevel > 0 &&
                       header->forward[currentLevel] == nullptr)
                    currentLevel--;

                return;
            }
            prev = prev->forward[0];
        }
    }

    vector<int> getTopN(int n) override {
        vector<int> result;
        SkipNode* current = header->forward[0];

        while (current && n--) {
            result.push_back(current->player.id);
            current = current->forward[0];
        }
        return result;
    }
};

// --- 3. AuctionTree (Red-Black Tree) ---

enum COLOR { RED, BLACK };

/* ================= RB TREE NODE ================= */
class RBNode {
public:
    int itemID;
    int price;
    COLOR color;
    RBNode *left, *right, *parent;

    RBNode(int id, int p)
        : itemID(id), price(p), color(RED),
          left(nullptr), right(nullptr), parent(nullptr) {}

    bool isOnLeft() { return this == parent->left; }

    RBNode* sibling() {
        if (!parent) return nullptr;
        return isOnLeft() ? parent->right : parent->left;
    }

    RBNode* uncle() {
        if (!parent || !parent->parent) return nullptr;
        return parent->isOnLeft()
               ? parent->parent->right
               : parent->parent->left;
    }

    bool hasRedChild() {
        return (left && left->color == RED) ||
               (right && right->color == RED);
    }
};

class ConcreteAuctionTree : public AuctionTree {
private:
    RBNode* root = nullptr;


    bool isLess(int price1, int id1, int price2, int id2) {
        if (price1 != price2) return price1 < price2;
        return id1 < id2;
    }


    void leftRotate(RBNode* x) {
        RBNode* nParent = x->right;
        if (x == root) root = nParent;

        x->right = nParent->left;
        if (nParent->left) nParent->left->parent = x;

        nParent->parent = x->parent;
        if (x->parent) {
            if (x->isOnLeft()) x->parent->left = nParent;
            else x->parent->right = nParent;
        }

        nParent->left = x;
        x->parent = nParent;
    }

    void rightRotate(RBNode* x) {
        RBNode* nParent = x->left;
        if (x == root) root = nParent;

        x->left = nParent->right;
        if (nParent->right) nParent->right->parent = x;

        nParent->parent = x->parent;
        if (x->parent) {
            if (x->isOnLeft()) x->parent->left = nParent;
            else x->parent->right = nParent;
        }

        nParent->right = x;
        x->parent = nParent;
    }


    void fixRedRed(RBNode* x) {
        if (x == root) {
            x->color = BLACK;
            return;
        }

        RBNode* parent = x->parent;
        RBNode* grandparent = parent->parent;
        RBNode* uncle = x->uncle();

        if (parent->color == BLACK) return;

        if (uncle && uncle->color == RED) {
            parent->color = BLACK;
            uncle->color = BLACK;
            grandparent->color = RED;
            fixRedRed(grandparent);
        } else {
            if (parent->isOnLeft()) {
                if (!x->isOnLeft()) {
                    leftRotate(parent);
                    x = parent;
                }
                parent->color = BLACK;
                grandparent->color = RED;
                rightRotate(grandparent);
            } else {
                if (x->isOnLeft()) {
                    rightRotate(parent);
                    x = parent;
                }
                parent->color = BLACK;
                grandparent->color = RED;
                leftRotate(grandparent);
            }
        }
    }


    void bstInsert(RBNode* node) {
        RBNode* temp = root;
        while (true) {
            if (isLess(node->price, node->itemID,
                       temp->price, temp->itemID)) {
                if (!temp->left) {
                    temp->left = node;
                    node->parent = temp;
                    break;
                }
                temp = temp->left;
            } else {
                if (!temp->right) {
                    temp->right = node;
                    node->parent = temp;
                    break;
                }
                temp = temp->right;
            }
        }
    }


    RBNode* findByID(RBNode* node, int id) {
        if (!node) return nullptr;
        if (node->itemID == id) return node;
        RBNode* left = findByID(node->left, id);
        if (left) return left;
        return findByID(node->right, id);
    }


    RBNode* successor(RBNode* x) {
        while (x->left) x = x->left;
        return x;
    }

    RBNode* BSTreplace(RBNode* x) {
        if (x->left && x->right) return successor(x->right);
        if (!x->left && !x->right) return nullptr;
        return x->left ? x->left : x->right;
    }

    void fixDoubleBlack(RBNode* x) {
        if (x == root) return;

        RBNode* sibling = x->sibling();
        RBNode* parent = x->parent;

        if (!sibling) {
            fixDoubleBlack(parent);
        } else {
            if (sibling->color == RED) {
                parent->color = RED;
                sibling->color = BLACK;
                sibling->isOnLeft()
                    ? rightRotate(parent)
                    : leftRotate(parent);
                fixDoubleBlack(x);
            } else {
                if (sibling->hasRedChild()) {
                    if (sibling->left && sibling->left->color == RED) {
                        if (sibling->isOnLeft()) {
                            sibling->left->color = sibling->color;
                            sibling->color = parent->color;
                            rightRotate(parent);
                        } else {
                            sibling->left->color = parent->color;
                            rightRotate(sibling);
                            leftRotate(parent);
                        }
                    } else {
                        if (sibling->isOnLeft()) {
                            sibling->right->color = parent->color;
                            leftRotate(sibling);
                            rightRotate(parent);
                        } else {
                            sibling->right->color = sibling->color;
                            sibling->color = parent->color;
                            leftRotate(parent);
                        }
                    }
                    parent->color = BLACK;
                } else {
                    sibling->color = RED;
                    if (parent->color == BLACK)
                        fixDoubleBlack(parent);
                    else
                        parent->color = BLACK;
                }
            }
        }
    }

    void deleteNode(RBNode* v) {
        RBNode* u = BSTreplace(v);
        bool uvBlack = ((u == nullptr || u->color == BLACK) && v->color == BLACK);
        RBNode* parent = v->parent;

        if (!u) {
            if (v == root) root = nullptr;
            else {
                if (uvBlack) fixDoubleBlack(v);
                else if (v->sibling()) v->sibling()->color = RED;

                if (v->isOnLeft()) parent->left = nullptr;
                else parent->right = nullptr;
            }
            delete v;
            return;
        }

        if (!v->left || !v->right) {
            if (v == root) {
                v->itemID = u->itemID;
                v->price = u->price;
                v->left = v->right = nullptr;
                delete u;
            } else {
                if (v->isOnLeft()) parent->left = u;
                else parent->right = u;
                u->parent = parent;
                delete v;
                if (uvBlack) fixDoubleBlack(u);
                else u->color = BLACK;
            }
            return;
        }

        swap(v->itemID, u->itemID);
        swap(v->price, u->price);
        deleteNode(u);
    }
    void inorder(RBNode* node) {
    if (!node) return;
    inorder(node->left);
    cout << "(ID=" << node->itemID
         << ", Price=" << node->price
         << ", Color=" << (node->color == RED ? "R" : "B") << ") ";
    inorder(node->right);
}
    void deleteSubtree(RBNode* node) {
    if (!node) return;

    deleteSubtree(node->left);
    deleteSubtree(node->right);
    delete node;
    }


public:
    ConcreteAuctionTree() {}
     ~ConcreteAuctionTree() {
    deleteSubtree(root);
    }

    void insertItem(int itemID, int price) override {
        RBNode* node = new RBNode(itemID, price);
        if (!root) {
            node->color = BLACK;
            root = node;
            return;
        }
        bstInsert(node);
        fixRedRed(node);
    }


    void deleteItem(int itemID) override {
        if (!root) return;
        RBNode* v = findByID(root, itemID);
        if (!v) return;
        deleteNode(v);
    }

    void printInOrder() {
    cout << "Inorder: ";
    inorder(root);
    cout << endl;
    }
};

// =========================================================
// PART B: INVENTORY SYSTEM (Dynamic Programming)
// =========================================================

int InventorySystem::optimizeLootSplit(int n, vector<int>& coins) {
    // TODO: Implement partition problem using DP
    // Goal: Minimize |sum(subset1) - sum(subset2)|
    // Hint: Use subset sum DP to find closest sum to total/2
    // return 0;
    if (n == 0 || coins.empty())
        return 0;

    int totalSum = 0;
    for (int c : coins)
        totalSum += c;

    int target = totalSum / 2;

    vector<vector<bool>> dp(n + 1, vector<bool>(target + 1, false));
    dp[0][0] = true;

    for (int i = 1; i <= n; i++) {
        int coin = coins[i - 1];
        for (int j = 0; j <= target; j++) {
            dp[i][j] = dp[i - 1][j];
            if (j >= coin) {
                dp[i][j] = dp[i][j] || dp[i - 1][j - coin];
            }
        }
    }

    int best = 0;
    for (int j = target; j >= 0; j--) {
        if (dp[n][j]) {
            best = j;
            break;
        }
    }

    return totalSum - 2 * best;
}


int InventorySystem::maximizeCarryValue(int capacity, vector<pair<int, int>>& items) {
    // TODO: Implement 0/1 Knapsack using DP
    // items = {weight, value} pairs
    // Return maximum value achievable within capacity
    if (capacity == 0 || items.empty())
        return 0;

    int n = items.size();
    vector<vector<int>> dp(n + 1, vector<int>(capacity + 1, 0));

    for (int i = 1; i <= n; i++) {
        int weight = items[i - 1].first;
        int value  = items[i - 1].second;

        for (int w = 1; w <= capacity; w++) {
            if (weight <= w) {
                dp[i][w] = max(dp[i - 1][w],
                               dp[i - 1][w - weight] + value);
            } else {
                dp[i][w] = dp[i - 1][w];
            }
        }
    }

    return dp[n][capacity];
}

long long InventorySystem::countStringPossibilities(string s) {
    // TODO: Implement string decoding DP
    // Rules: "uu" can be decoded as "w" or "uu"
    //        "nn" can be decoded as "m" or "nn"
    // Count total possible decodings
    // return 0;

    if (s.empty())
        return 1;

    for (char c : s) {
        if (c == 'w' || c == 'm')
            return 0;
    }

    const int MOD = 1e9 + 7;
    int n = s.size();

    vector<long long> dp(n + 1, 0);
    dp[0] = 1;

    for (int i = 1; i <= n; i++) {
        dp[i] = dp[i - 1];
        if (i >= 2) {
            if ((s[i - 2] == 'u' && s[i - 1] == 'u') ||
                (s[i - 2] == 'n' && s[i - 1] == 'n')) {
                dp[i] = (dp[i] + dp[i - 2]) % MOD;
            }
        }
    }

    return dp[n];
}

// =========================================================
// PART C: WORLD NAVIGATOR (Graphs)
// =========================================================

bool WorldNavigator::pathExists(int n, vector<vector<int>>& edges, int source, int dest) {
    if (source == dest)
        return true;

    vector<vector<int>> adj(n);
    for (auto& e : edges) {
        int u = e[0];
        int v = e[1];
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    vector<bool> visited(n, false);
    queue<int> q;
    q.push(source);
    visited[source] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        if (u == dest) return true;

        for (int v : adj[u]) {
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }

    return false;
}

long long WorldNavigator::minBribeCost(int n, int m, long long goldRate, long long silverRate,
                                       vector<vector<int>>& roadData) {
    struct Edge {
        int u, v;
        long long cost;
    };

    vector<Edge> edges;
    for (auto& r : roadData) {
        int u = r[0];
        int v = r[1];
        long long cost = r[2] * goldRate + r[3] * silverRate;
        edges.push_back({u, v, cost});
    }

    sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.cost < b.cost;
    });

    vector<int> parent(n), rank(n, 0);
    for (int i = 0; i < n; i++) parent[i] = i;

    function<int(int)> find = [&](int x) {
        if (parent[x] != x)
            parent[x] = find(parent[x]);
        return parent[x];
    };

    auto unite = [&](int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return false;

        if (rank[a] < rank[b]) swap(a, b);
        parent[b] = a;
        if (rank[a] == rank[b]) rank[a]++;
        return true;
    };

    long long totalCost = 0;
    int usedEdges = 0;

    for (auto& e : edges) {
        if (unite(e.u, e.v)) {
            totalCost += e.cost;
            usedEdges++;
        }
    }

    if (usedEdges != n - 1) return -1;
    return totalCost;
}

string WorldNavigator::sumMinDistancesBinary(int n, vector<vector<int>>& roads) {
    const long long INF = 1e18;

    vector<vector<long long>> dist(n, vector<long long>(n, INF));
    for (int i = 0; i < n; i++) {
        dist[i][i] = 0;
    }

    for (auto& r : roads) {
        int u = r[0];
        int v = r[1];
        int w = r[2];
        dist[u][v] = min(dist[u][v], (long long)w);
        dist[v][u] = min(dist[v][u], (long long)w);
    }

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[i][k] < INF && dist[k][j] < INF) {
                    dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
                }
            }
        }
    }

    long long totalSum = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (dist[i][j] < INF) {
                totalSum += dist[i][j];
            }
        }
    }

    if (totalSum == 0)
        return "0";

    string binary = "";
    while (totalSum > 0) {
        binary = char('0' + (totalSum % 2)) + binary;
        totalSum /= 2;
    }
    return binary;
}

// =========================================================
// PART D: SERVER KERNEL (Greedy)
// =========================================================

int ServerKernel::minIntervals(vector<char>& tasks, int n) {
    vector<int> freq(26, 0);
    for (char c : tasks) {
        freq[c - 'A']++;
    }

    int maxFreq = 0;
    for (int f : freq) {
        maxFreq = max(maxFreq, f);
    }

    int countMax = 0;
    for (int f : freq) {
        if (f == maxFreq) {
            countMax++;
        }
    }

    int intervals = (maxFreq - 1) * (n + 1) + countMax;

    return max((int)tasks.size(), intervals);
}

// =========================================================
// FACTORY FUNCTIONS (Required for Testing)
// =========================================================

extern "C" {

    // Create ONE shared PlayerTable
    static PlayerTable* sharedPlayerTable = nullptr;

    // ---------- PlayerTable ----------
    PlayerTable* createPlayerTable() {
        if (!sharedPlayerTable)
            sharedPlayerTable = new ConcretePlayerTable();
        return sharedPlayerTable;
    }

    // ---------- Leaderboard ----------
    Leaderboard* createLeaderboard() {
        if (!sharedPlayerTable)
            sharedPlayerTable = new ConcretePlayerTable();
        return new ConcreteLeaderboard(sharedPlayerTable);
    }

    // ---------- AuctionTree ----------
    AuctionTree* createAuctionTree() {
        return new ConcreteAuctionTree();
    }
}


