#include "adk.hpp"
#include <climits>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <queue>
#include <random>
#include <tuple>
#define ARG 1.0f
#define TIME 0.95f
#define SUBMAXVAL_DIFF 0.025f
#define MAX_NODE_COUNT 300000
#define MAX_LENGTH 80
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define abs(x) (((x) > 0) ? (x) : -(x))

const short max_round = 512;

struct MyPos {
    char x, y;
    MyPos() = default;
    MyPos(char _x, char _y) : x(_x), y(_y) {}
};

bool operator==(const MyPos &lhs, const MyPos &rhs) { return (lhs.x == rhs.x) && (lhs.y == rhs.y); }
bool operator!=(const MyPos &lhs, const MyPos &rhs) { return !(lhs == rhs); }

struct MySnake {
    MyPos pos_list[MAX_LENGTH];
    char head, tail;
    char length_bank;
    char id;
    bool camp, railgun_item;

    inline void push_front(MyPos mypos) {
        if (head > 0) {
            head--;
            pos_list[head] = mypos;
        } else {
            char diff = MAX_LENGTH - tail;
            for (char i = tail - 1; i >= head; i--) {
                pos_list[i + diff] = pos_list[i];
            }
            head += diff - 1;
            tail += diff;
            pos_list[head] = mypos;
        }
    }
    inline void pop_back() { tail--; }
    inline const MyPos &front() { return pos_list[head]; }
    inline const MyPos &back() { return pos_list[tail - 1]; }
    MySnake() = default;
    MySnake(char _id) : length_bank(0), railgun_item(false), id(_id), head(MAX_LENGTH), tail(MAX_LENGTH) {}
    MySnake(const Snake &snake)
        : id(snake.id), length_bank(snake.length_bank), camp(snake.camp == 1),
          railgun_item(snake.railgun_item.id >= 0) {
        char size = snake.coord_list.size();
        for (char i = MAX_LENGTH - size; i < MAX_LENGTH; i++) {
            pos_list[i] = MyPos(snake.coord_list[i - MAX_LENGTH + size].x, snake.coord_list[i - MAX_LENGTH + size].y);
        }
        head = MAX_LENGTH - size;
        tail = MAX_LENGTH;
    }
    MySnake(MyPos list[], char size, char newid, const MySnake &snake)
        : id(newid), length_bank(snake.length_bank), camp(snake.camp), railgun_item(false) {
        for (char i = MAX_LENGTH - size; i < MAX_LENGTH; i++) {
            pos_list[i] = list[i - MAX_LENGTH + size];
        }
        head = MAX_LENGTH - size;
        tail = MAX_LENGTH;
    }

    inline char getdir() {
        if (tail - head < 2) {
            return 0;
        }
        if (pos_list[head].x - pos_list[head + 1].x == 1) {
            return 0;
        } else if (pos_list[head].y - pos_list[head + 1].y == 1) {
            return 1;
        } else if (pos_list[head].x - pos_list[head + 1].x == -1) {
            return 2;
        } else {
            return 3;
        }
    }

    // MySnake(const MySnake &mysnake)
    //     : id(mysnake.id), length_bank(mysnake.length_bank), camp(mysnake.camp), railgun_item(mysnake.railgun_item) {
    //     head = mysnake.head;
    //     tail = mysnake.tail;
    //     for (char i = head; i < tail; i++) {
    //         pos_list[i] = mysnake.pos_list[i];
    //     }
    // }

    // MySnake &operator=(const MySnake &mysnake) {
    //     if (&mysnake != this) {
    //         id = mysnake.id;
    //         length_bank = mysnake.length_bank;
    //         camp = mysnake.camp;
    //         railgun_item = mysnake.railgun_item;
    //         head = mysnake.head;
    //         tail = mysnake.tail;
    //         for (char i = head; i < tail; i++) {
    //             pos_list[i] = mysnake.pos_list[i];
    //         }
    //     }
    //     return *this;
    // }

    inline char length() const { return tail - head; }

    inline const MyPos &operator[](char idx) const { return pos_list[head + idx]; }
};
MySnake wrong_snake(-1);

bool operator==(const MySnake &lhs, const MySnake &rhs) { return lhs.id == rhs.id; }
bool operator!=(const MySnake &lhs, const MySnake &rhs) { return lhs.id != rhs.id; }
bool snake_compare(const MySnake &s1, const MySnake &s2) { return s1.id < s2.id; }

struct MyItem {
    char x, y, type, param;
    short time;
    MyItem() = default;
    MyItem(char _x, char _y, char _type, short _time, char _param)
        : x(_x), y(_y), type(_type), param(_param), time(_time) {}
};

bool mycamp;
short current_round;
bool firstaction = true;
clock_t start_t;
bool if_gen_item[513];
MyItem gen_itemlist[513];
short future_item_rounds[513][10];
short future_item_rounds_count[513];
const char dx[4] = {1, 0, -1, 0};
const char dy[4] = {0, 1, 0, -1};
float time_ratio = 1.0;      //用于时间分配
float used_time_ratio = 0.0; //已经用的时间比例
float adjust_ratio;
char last_action;
char snakes_left;

struct Node {
    MySnake player_snakes[2][4];
    MyItem itemlist[17];
    std::vector<bool> placeholder;
    char itemcount;
    char snakecount[2];
    char countbound[2];
    char map[16][16]; //-1为0号墙,-2为1号墙,0或正数为蛇,CHAR_MIN为空
    float count, win; // MCTS使用
    int p;            //父节点
    int lc, rc;       //孩子所在的区间
    float origin;     //估值使用
    // float pure, pureval;
    short round;
    char useaction; //使用的行动,1右2上3左4下5射线6分裂
    char nownumber; //现在行动的蛇编号
    bool nowaction; //现在行动的玩家
    bool end;       //是否是终结节点

    inline void item_push_back(MyItem item) {
        itemlist[itemcount] = item;
        itemcount++;
    }
    inline void item_remove(char index) {
        for (char i = index; i < itemcount - 1; i++) {
            itemlist[i] = itemlist[i + 1];
        }
        itemcount--;
    }
    inline void update_items() {
        for (char i = 0; i < itemcount;) {
            if (itemlist[i].time + 16 <= round) {
                item_remove(i);
                continue;
            }
            i++;
        }
    }

    inline void addsnake(bool player, MySnake snake) {
        player_snakes[player][snakecount[player]] = snake;
        snakecount[player]++;
    }
    inline void removesnake(bool player, char index) {
        if (player == nowaction && index <= nownumber) {
            nownumber--;
        }
        for (char i = index; i < snakecount[player] - 1; i++) {
            player_snakes[player][i] = player_snakes[player][i + 1];
        }
        snakecount[player]--;
    }
    inline void insertsnake(bool player, char index, MySnake snake) {
        for (char i = snakecount[player]; i > index; i--) {
            player_snakes[player][i] = player_snakes[player][i - 1];
        }
        player_snakes[player][index] = snake;
        snakecount[player]++;
    }

    Node() = default;
    Node(const std::vector<Snake> &p0, const std::vector<Snake> &p1, const TwoDimArray<int> &wall,
         const TwoDimArray<int> &snake, const std::vector<Item> &items, int snake_index, int _round)
        : p(-1), lc(-1), rc(-1), count(0), win(0), origin(0), nowaction(mycamp), nownumber(snake_index), round(_round),
          end(false), itemcount(0) {
        snakecount[0] = snakecount[1] = 0;
        countbound[0] = countbound[1] = 4;
        for (char i = 0; i < 16; i++) {
            for (char j = 0; j < 16; j++) {
                if (snake[i][j] != -1) {
                    map[i][j] = snake[i][j];
                    continue;
                }
                if (wall[i][j] != -1) {
                    map[i][j] = wall[i][j] == 0 ? -1 : -2;
                    continue;
                }
                map[i][j] = CHAR_MIN;
            }
        }
        for (auto &c : items) {
            if (!c.eaten && !c.expired && c.time <= _round && c.time + 16 > _round) {
                item_push_back(MyItem(c.x, c.y, c.type, c.time, c.param));
            }
        }

        for (auto &c : p0) {
            addsnake(0, MySnake(c));
        }
        for (auto &c : p1) {
            addsnake(1, MySnake(c));
        }
    }
    // Node(const Node &node)
    //     : p(node.p), lc(node.lc), rc(node.rc), count(node.count), win(node.win), origin(node.origin),
    //       nowaction(node.nowaction), nownumber(node.nownumber), round(node.round), end(node.end),
    //       useaction(node.useaction) {
    //     memcpy(map, node.map, sizeof(char) * 256);
    //     itemcount = node.itemcount;
    //     for (char i = 0; i < itemcount; i++) {
    //         itemlist[i] = node.itemlist[i];
    //     }
    //     snakecount[0] = node.snakecount[0];
    //     snakecount[1] = node.snakecount[1];
    //     for (char i = 0; i < 2; i++) {
    //         for (char j = 0; j < snakecount[i]; j++) {
    //             player_snakes[i][j] = node.player_snakes[i][j];
    //         }
    //     }
    // }
    // Node &operator=(const Node &node) {
    //     if (&node != this) {
    //         p = node.p;
    //         lc = node.lc;
    //         rc = node.rc;
    //         count = node.count;
    //         win = node.win;
    //         origin = node.origin;
    //         nowaction = node.nowaction;
    //         nownumber = node.nownumber;
    //         round = node.round;
    //         end = node.end;
    //         useaction = node.useaction;
    //         memcpy(map, node.map, sizeof(char) * 256);
    //         itemcount = node.itemcount;
    //         for (char i = 0; i < itemcount; i++) {
    //             itemlist[i] = node.itemlist[i];
    //         }
    //         snakecount[0] = node.snakecount[0];
    //         snakecount[1] = node.snakecount[1];
    //         for (char i = 0; i < 2; i++) {
    //             for (char j = 0; j < snakecount[i]; j++) {
    //                 player_snakes[i][j] = node.player_snakes[i][j];
    //             }
    //         }
    //     }
    //     return *this;
    // }

    inline void clean() {
        p = lc = rc = -1;
        count = win = origin = 0;
    }

    inline MySnake &findsnake(char id) {
        for (char i = 0; i < 2; i++) {
            for (char j = 0; j < snakecount[i]; j++) {
                if (player_snakes[i][j].id == id) {
                    return player_snakes[i][j];
                }
            }
        }
    }
    inline MyPos snakeindex(char id) {
        for (char i = 0; i < 2; i++) {
            for (char j = 0; j < snakecount[i]; j++) {
                if (player_snakes[i][j].id == id) {
                    return MyPos(i, j);
                }
            }
        }
    }

    void split() {
        MySnake &nowsnake = player_snakes[nowaction][nownumber];
        MyPos newsnake_pos[MAX_LENGTH / 2];
        char maxid = -1;
        for (char i = 0; i < 2; i++) {
            for (char j = 0; j < snakecount[i]; j++) {
                if (player_snakes[i][j].id > maxid) {
                    maxid = player_snakes[i][j].id;
                }
            }
        }
        if (maxid == CHAR_MAX) {
            maxid--;
        }
        maxid++;
        char length = nowsnake.length() / 2;
        for (char i = 0; i < length; i++) {
            newsnake_pos[i] = nowsnake.back();
            map[nowsnake.back().x][nowsnake.back().y] = maxid;
            nowsnake.pop_back();
        }
        insertsnake(nowaction, nownumber + 1, MySnake(newsnake_pos, length, maxid, nowsnake));
        // }
        player_snakes[nowaction][nownumber].length_bank = 0;
        nownumber++;
    }

    void printmap() { // for debug
        for (char i = 15; i >= 0; i--) {
            for (char j = 0; j < 16; j++) {
                std::cerr << (short)(map[j][i]) << " ";
            }
            std::cerr << '\n';
        }
    }
} node[MAX_NODE_COUNT];
Node main_node;
int nodecount = 0;
int root; //根节点
int local_root;

inline bool time_judge() {
    if (clock() - start_t < TIME * CLOCKS_PER_SEC * time_ratio * adjust_ratio) {
        return true;
    }
    return false;
}
// inline bool local_time_judge() {
//     if (clock() - start_t < 0.05 * CLOCKS_PER_SEC) {
//         return true;
//     }
//     return false;
// }

int utcbest(int rootindex) {
    float max = 0;
    int now = rootindex;
    int best = rootindex;
    float val;
    while (node[now].lc != -1) {
        for (int i = node[now].lc; i <= node[now].rc; i++) {
            if (node[i].count != 0)
                val = (node[now].nowaction == 0 ? (node[i].win / node[i].count) : 1 - (node[i].win / node[i].count)) +
                      ARG * sqrt(log(node[now].count) / node[i].count);
            else
                val = 2.0;
            if (val > max) {
                max = val;
                best = i;
            }
        }
        now = best;
        max = -1;
    }
    return best;
}

char actions[7];
inline void get_current_actions(Node &nownode) {
    bool player = nownode.nowaction;
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    char t_x, t_y;
    char validcount, solidcount;
    validcount = solidcount = 0;
    actions[6] = 0;
    for (char dir = 0; dir < 4; dir++) {
        //蛇头到达位置
        t_x = nowsnake.front().x + dx[dir];
        t_y = nowsnake.front().y + dy[dir];

        //超出边界
        if (t_x >= 16 || t_x < 0 || t_y >= 16 || t_y < 0) {
            actions[dir] = -1;
            actions[6] = 1;
            continue;
        }

        //撞墙或非本蛇
        if (nownode.map[t_x][t_y] != CHAR_MIN && nownode.map[t_x][t_y] != nowsnake.id) {
            actions[dir] = -1;
            actions[6] = 1;
            continue;
        }

        //回头撞自己
        if ((nowsnake.length() > 2 && nowsnake[1].x == t_x && nowsnake[1].y == t_y) ||
            (nowsnake.length() == 2 && nowsnake[1].x == t_x && nowsnake[1].y == t_y && nowsnake.length_bank > 0)) {
            actions[dir] = -2;
            continue;
        }

        //移动导致固化
        if (nownode.map[t_x][t_y] == nowsnake.id &&
            !(nowsnake.back().x == t_x && nowsnake.back().y == t_y && nowsnake.length_bank == 0)) {
            actions[dir] = 1;
            solidcount++;
            continue;
        }
        actions[dir] = 0;
        validcount++;
    }
    actions[4] = validcount;
    actions[5] = solidcount;
}

void process_items(Node &nownode) { //先结算当前场上道具
    for (char i = 0; i < nownode.itemcount;) {
        MyItem c = nownode.itemlist[i];
        if (nownode.map[c.x][c.y] >= 0) {
            if (c.type == 0) {
                nownode.findsnake(nownode.map[c.x][c.y]).length_bank += c.param;
            } else {
                nownode.findsnake(nownode.map[c.x][c.y]).railgun_item = true;
            }
            nownode.item_remove(i);
            continue;
        }
        i++;
    }
}

bool fillmap[16][16];
MyPos quene[256];
short head, tail;
void fill(Node &nownode, char xmin, char xmax, char ymin, char ymax, char snake_id) {
    head = tail = 0;
    memset(fillmap, 0, sizeof(fillmap));
    for (char i = xmin; i <= xmax; i++) {
        if (fillmap[i][ymin] == 0 && nownode.map[i][ymin] != snake_id) {
            quene[tail] = MyPos(i, ymin);
            tail++;
            fillmap[i][ymin] = 1;
            while (head != tail) {
                MyPos &pos = quene[head];
                head++;
                if (!(pos.x + 1 > xmax || fillmap[pos.x + 1][pos.y] == 1 ||
                      nownode.map[pos.x + 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x + 1, pos.y);
                    tail++;
                    fillmap[pos.x + 1][pos.y] = 1;
                }
                if (!(pos.y + 1 > ymax || fillmap[pos.x][pos.y + 1] == 1 ||
                      nownode.map[pos.x][pos.y + 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y + 1);
                    tail++;
                    fillmap[pos.x][pos.y + 1] = 1;
                }
                if (!(pos.x - 1 < xmin || fillmap[pos.x - 1][pos.y] == 1 ||
                      nownode.map[pos.x - 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x - 1, pos.y);
                    tail++;
                    fillmap[pos.x - 1][pos.y] = 1;
                }
                if (!(pos.y - 1 < ymin || fillmap[pos.x][pos.y - 1] == 1 ||
                      nownode.map[pos.x][pos.y - 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y - 1);
                    tail++;
                    fillmap[pos.x][pos.y - 1] = 1;
                }
            }
        }
        if (fillmap[i][ymax] == 0 && nownode.map[i][ymax] != snake_id) {
            quene[tail] = MyPos(i, ymax);
            tail++;
            fillmap[i][ymax] = 1;
            while (head != tail) {
                MyPos &pos = quene[head];
                head++;
                if (!(pos.x + 1 > xmax || fillmap[pos.x + 1][pos.y] == 1 ||
                      nownode.map[pos.x + 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x + 1, pos.y);
                    tail++;
                    fillmap[pos.x + 1][pos.y] = 1;
                }
                if (!(pos.y + 1 > ymax || fillmap[pos.x][pos.y + 1] == 1 ||
                      nownode.map[pos.x][pos.y + 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y + 1);
                    tail++;
                    fillmap[pos.x][pos.y + 1] = 1;
                }
                if (!(pos.x - 1 < xmin || fillmap[pos.x - 1][pos.y] == 1 ||
                      nownode.map[pos.x - 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x - 1, pos.y);
                    tail++;
                    fillmap[pos.x - 1][pos.y] = 1;
                }
                if (!(pos.y - 1 < ymin || fillmap[pos.x][pos.y - 1] == 1 ||
                      nownode.map[pos.x][pos.y - 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y - 1);
                    tail++;
                    fillmap[pos.x][pos.y - 1] = 1;
                }
            }
        }
    }
    for (char i = ymin; i <= ymax; i++) {
        if (fillmap[xmin][i] == 0 && nownode.map[xmin][i] != snake_id) {
            quene[tail] = MyPos(xmin, i);
            tail++;
            fillmap[xmin][i] = 1;
            while (head != tail) {
                MyPos &pos = quene[head];
                head++;
                if (!(pos.x + 1 > xmax || fillmap[pos.x + 1][pos.y] == 1 ||
                      nownode.map[pos.x + 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x + 1, pos.y);
                    tail++;
                    fillmap[pos.x + 1][pos.y] = 1;
                }
                if (!(pos.y + 1 > ymax || fillmap[pos.x][pos.y + 1] == 1 ||
                      nownode.map[pos.x][pos.y + 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y + 1);
                    tail++;
                    fillmap[pos.x][pos.y + 1] = 1;
                }
                if (!(pos.x - 1 < xmin || fillmap[pos.x - 1][pos.y] == 1 ||
                      nownode.map[pos.x - 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x - 1, pos.y);
                    tail++;
                    fillmap[pos.x - 1][pos.y] = 1;
                }
                if (!(pos.y - 1 < ymin || fillmap[pos.x][pos.y - 1] == 1 ||
                      nownode.map[pos.x][pos.y - 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y - 1);
                    tail++;
                    fillmap[pos.x][pos.y - 1] = 1;
                }
            }
        }
        if (fillmap[xmax][i] == 0 && nownode.map[xmax][i] != snake_id) {
            quene[tail] = MyPos(xmax, i);
            tail++;
            fillmap[xmax][i] = 1;
            while (head != tail) {
                MyPos &pos = quene[head];
                head++;
                if (!(pos.x + 1 > xmax || fillmap[pos.x + 1][pos.y] == 1 ||
                      nownode.map[pos.x + 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x + 1, pos.y);
                    tail++;
                    fillmap[pos.x + 1][pos.y] = 1;
                }
                if (!(pos.y + 1 > ymax || fillmap[pos.x][pos.y + 1] == 1 ||
                      nownode.map[pos.x][pos.y + 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y + 1);
                    tail++;
                    fillmap[pos.x][pos.y + 1] = 1;
                }
                if (!(pos.x - 1 < xmin || fillmap[pos.x - 1][pos.y] == 1 ||
                      nownode.map[pos.x - 1][pos.y] == snake_id)) {
                    quene[tail] = MyPos(pos.x - 1, pos.y);
                    tail++;
                    fillmap[pos.x - 1][pos.y] = 1;
                }
                if (!(pos.y - 1 < ymin || fillmap[pos.x][pos.y - 1] == 1 ||
                      nownode.map[pos.x][pos.y - 1] == snake_id)) {
                    quene[tail] = MyPos(pos.x, pos.y - 1);
                    tail++;
                    fillmap[pos.x][pos.y - 1] = 1;
                }
            }
        }
    }
    if (nownode.nowaction == 0) { //填-1
        for (char i = xmin; i <= xmax; i++) {
            for (char j = ymin; j <= ymax; j++) {
                if (fillmap[i][j] == 0) {                                          //需要填充
                    if (nownode.map[i][j] >= 0 && nownode.map[i][j] != snake_id) { //别的蛇，杀掉
                        MyPos index = nownode.snakeindex(nownode.map[i][j]);
                        MySnake &killsnake = nownode.player_snakes[index.x][index.y];
                        for (char k = killsnake.head; k < killsnake.tail; k++) {
                            nownode.map[killsnake.pos_list[k].x][killsnake.pos_list[k].y] = -1;
                        }
                        nownode.removesnake(index.x, index.y);
                    } else {
                        nownode.map[i][j] = -1;
                    }
                }
            }
        }
    } else { //填-2
        for (char i = xmin; i <= xmax; i++) {
            for (char j = ymin; j <= ymax; j++) {
                if (fillmap[i][j] == 0) {                                          //需要填充
                    if (nownode.map[i][j] >= 0 && nownode.map[i][j] != snake_id) { //别的蛇，杀掉
                        MyPos index = nownode.snakeindex(nownode.map[i][j]);
                        MySnake &killsnake = nownode.player_snakes[index.x][index.y];
                        for (char k = killsnake.head; k < killsnake.tail; k++) {
                            nownode.map[killsnake.pos_list[k].x][killsnake.pos_list[k].y] = -2;
                        }
                        nownode.removesnake(index.x, index.y);
                    } else {
                        nownode.map[i][j] = -2;
                    }
                }
            }
        }
    }
}

bool tonext(Node &nownode, char action) {
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    if (action <= 4) {
        //蛇头到达位置
        char t_x = nowsnake.front().x + dx[action - 1];
        char t_y = nowsnake.front().y + dy[action - 1];

        if (t_x >= 16 || t_x < 0 || t_y >= 16 || t_y < 0 ||
            (nownode.map[t_x][t_y] != CHAR_MIN && nownode.map[t_x][t_y] != nowsnake.id)) {
            for (char k = nowsnake.head; k < nowsnake.tail; k++) {
                nownode.map[nowsnake.pos_list[k].x][nowsnake.pos_list[k].y] = CHAR_MIN;
            }
            nownode.removesnake(nownode.nowaction, nownode.nownumber);
        } else if (nownode.map[t_x][t_y] == nowsnake.id &&
                   !(nowsnake.back().x == t_x && nowsnake.back().y == t_y && nowsnake.length_bank == 0)) {
            char xmin, xmax, ymin, ymax;
            xmin = ymin = 16;
            xmax = ymax = 0;
            char k = nowsnake.head;
            for (; k < nowsnake.tail; k++) {
                MyPos &c = nowsnake.pos_list[k];
                if (c.x < xmin) {
                    xmin = c.x;
                }
                if (c.x > xmax) {
                    xmax = c.x;
                }
                if (c.y < ymin) {
                    ymin = c.y;
                }
                if (c.y > ymax) {
                    ymax = c.y;
                }
                if (c.x == t_x && c.y == t_y) {
                    break;
                }
            }
            k++;
            for (; k < nowsnake.tail; k++) { //删除后面的
                nownode.map[nowsnake.pos_list[k].x][nowsnake.pos_list[k].y] = CHAR_MIN;
            }
            char id = nowsnake.id;
            nownode.removesnake(nownode.nowaction, nownode.nownumber);
            fill(nownode, xmin, xmax, ymin, ymax, id);
        } else {
            if (nowsnake.length_bank > 0) {
                nowsnake.push_front(MyPos(t_x, t_y));
                nowsnake.length_bank--;
            } else {
                nownode.map[nowsnake.back().x][nowsnake.back().y] = CHAR_MIN;
                nowsnake.pop_back();
                nowsnake.push_front(MyPos(t_x, t_y));
            }
            nownode.map[t_x][t_y] = nowsnake.id;
            for (char i = 0; i < nownode.itemcount;) {
                MyItem c = nownode.itemlist[i];
                if (c.x == t_x && c.y == t_y && c.time + 16 > nownode.round) { //同位置,且未过期
                    if (c.type == 0) {
                        nowsnake.length_bank += c.param;
                    } else {
                        nowsnake.railgun_item = true;
                    }
                    nownode.item_remove(i);
                    break;
                }
                i++;
            }
        }
    } else if (action == 5) {
        char dx = nowsnake[0].x - nowsnake[1].x;
        char dy = nowsnake[0].y - nowsnake[1].y;
        char nx = nowsnake[0].x + dx;
        char ny = nowsnake[0].y + dy;
        while (nx >= 0 && nx < 16 && ny >= 0 && ny < 16) {
            if (nownode.map[nx][ny] < 0) {
                nownode.map[nx][ny] = CHAR_MIN;
            }
            nx += dx;
            ny += dy;
        }
        nowsnake.railgun_item = false;
    } else if (action == 6) {
        nownode.split();
    }

    if (nownode.snakecount[0] == 0 && nownode.snakecount[1] == 0) { //没蛇了
        return true;
    }

    if (nownode.nowaction == 0) {
        if (nownode.nownumber + 1 == nownode.snakecount[0]) { //换人
            if (nownode.snakecount[1] > 0) {                  // 1号有蛇
                nownode.nowaction = 1;
                nownode.nownumber = 0;
            } else { //直接下一轮
                nownode.round++;
                nownode.update_items();
                nownode.nownumber = 0;
                if (nownode.round > max_round) { //最大回合,结束
                    return true;
                }
                //生成道具
                if (if_gen_item[nownode.round]) {
                    auto &c = gen_itemlist[nownode.round];
                    if (nownode.map[c.x][c.y] >= 0) { //当即被吃掉
                        if (c.type == 0) {
                            nownode.findsnake(nownode.map[c.x][c.y]).length_bank += c.param;
                        } else {
                            nownode.findsnake(nownode.map[c.x][c.y]).railgun_item = true;
                        }
                    } else { //生成在场面上
                        nownode.item_push_back(c);
                    }
                }
            }
        } else {
            nownode.nownumber++;
        }
    } else {
        if (nownode.nownumber + 1 == nownode.snakecount[1]) { //换人
            if (nownode.snakecount[0] > 0) {                  // 0号有蛇
                nownode.nowaction = 0;
                nownode.nownumber = 0;
            } else { //直接下一轮
                nownode.nownumber = 0;
            }
            nownode.round++;
            nownode.update_items();
            if (nownode.round > max_round) { //最大回合,结束
                return true;
            }
            //生成道具
            if (if_gen_item[nownode.round]) {
                auto &c = gen_itemlist[nownode.round];
                if (nownode.map[c.x][c.y] >= 0) { //当即被吃掉
                    if (c.type == 0) {
                        nownode.findsnake(nownode.map[c.x][c.y]).length_bank += c.param;
                    } else {
                        nownode.findsnake(nownode.map[c.x][c.y]).railgun_item = true;
                    }
                } else { //生成在场面上
                    nownode.item_push_back(c);
                }
            }
        } else {
            nownode.nownumber++;
        }
    }
    return false;
}

inline char distance(const MyPos &a, const MyPos &b) { return abs(a.x - b.x) + abs(a.y - b.y); }
inline bool danger_border(const MyPos &pos) { return pos.x == 0 || pos.x == 15 || pos.y == 0 || pos.y == 15; }
inline bool dangerous_action(Node &node_before, char action) {
    MySnake &action_snake = node_before.player_snakes[node_before.nowaction][node_before.nownumber];
    const MyPos &before_pos = action_snake.front();
    if (action_snake.length() + action_snake.length_bank <= 2) {
        return false;
    }
    if (danger_border(before_pos)) {
        MyPos nextpos = MyPos(before_pos.x + dx[action - 1], before_pos.y + dy[action - 1]);
        if (nextpos.x >= 0 && nextpos.y >= 0 && nextpos.x <= 15 && nextpos.y <= 15 && danger_border(nextpos)) {
            for (char i = 0; i < node_before.snakecount[!node_before.nowaction]; i++) {
                if (distance(node_before.player_snakes[!node_before.nowaction][i].front(), nextpos) == 2 &&
                    !danger_border(node_before.player_snakes[!node_before.nowaction][i].front())) {
                    return true;
                }
            }
        }
    }
    return false;
}

inline char get_qi(Node &nownode, const MyPos pos) {
    char ret = 0;
    if (pos.x > 0 && nownode.map[pos.x - 1][pos.y] == CHAR_MIN) {
        ret++;
    }
    if (pos.x < 15 && nownode.map[pos.x + 1][pos.y] == CHAR_MIN) {
        ret++;
    }
    if (pos.y > 0 && nownode.map[pos.x][pos.y - 1] == CHAR_MIN) {
        ret++;
    }
    if (pos.y < 15 && nownode.map[pos.x][pos.y + 1] == CHAR_MIN) {
        ret++;
    }
    return ret;
}
inline char get_2_relational(const MyPos &centerpos, const MyPos &otherpos) {
    if (centerpos.x == otherpos.x - 2) {
        return 0;
    } else if (centerpos.x == otherpos.x + 2) {
        return 4;
    } else if (centerpos.y == otherpos.y - 2) {
        return 2;
    } else if (centerpos.y == otherpos.y + 2) {
        return 6;
    } else if (centerpos.x == otherpos.x - 1 && centerpos.y == otherpos.y - 1) {
        return 1;
    } else if (centerpos.x == otherpos.x + 1 && centerpos.y == otherpos.y - 1) {
        return 3;
    } else if (centerpos.x == otherpos.x + 1 && centerpos.y == otherpos.y + 1) {
        return 5;
    } else {
        return 7;
    }
}
inline char get_1_relational(const MyPos &centerpos, const MyPos &otherpos) {
    if (centerpos.x == otherpos.x - 1) {
        return 0;
    } else if (centerpos.x == otherpos.x + 1) {
        return 2;
    } else if (centerpos.y == otherpos.y - 1) {
        return 1;
    } else {
        return 3;
    }
}

inline char local_action_judge(Node &nownode, char valid_actions[], char action_count) {
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    const MyPos &nowpos = nowsnake.front();

    char dis1_snakes[4];
    char dis2_snakes[4];
    char dis1_snakes_count = 0;
    char dis2_snakes_count = 0;

    bool player = !nownode.nowaction;
    for (char i = 0; i < nownode.snakecount[player]; i++) {
        if (nownode.player_snakes[player][i].length() >= 2 &&
            nownode.player_snakes[player][i].length() + nownode.player_snakes[player][i].length_bank > 2) {
            const MyPos &enemypos = nownode.player_snakes[player][i].front();
            char dis = distance(enemypos, nowpos);
            if (dis == 2) {
                dis2_snakes[dis2_snakes_count++] = i;

            } else if (dis == 1) {
                dis1_snakes[dis1_snakes_count++] = i;
            }
        }
    }
    if (dis1_snakes_count == 0 && dis2_snakes_count == 0) {
        return -1;
    }
    char choose_actions[4];
    char choose_actions_count = 0;
    char sub_actions[4];
    char sub_actions_count = 0;

    if (dis2_snakes_count == 1) {
        const MyPos &enemypos = nownode.player_snakes[player][dis2_snakes[0]].front();
        char enemydir = nownode.player_snakes[player][dis2_snakes[0]].getdir();
        char relation = get_2_relational(nowpos, enemypos);
        if (relation == 0) {
            choose_actions[choose_actions_count++] = 0;
        } else if (relation == 2) {
            choose_actions[choose_actions_count++] = 1;
        } else if (relation == 4) {
            choose_actions[choose_actions_count++] = 2;
        } else if (relation == 6) {
            choose_actions[choose_actions_count++] = 3;
        } else if (relation == 1) {
            if (enemydir == 0 || enemydir == 2) {
                choose_actions[choose_actions_count++] = 0;
                sub_actions[sub_actions_count++] = 1;
            } else {
                choose_actions[choose_actions_count++] = 1;
                sub_actions[sub_actions_count++] = 0;
            }
        } else if (relation == 3) {
            if (enemydir == 0 || enemydir == 2) {
                choose_actions[choose_actions_count++] = 2;
                sub_actions[sub_actions_count++] = 1;
            } else {
                choose_actions[choose_actions_count++] = 1;
                sub_actions[sub_actions_count++] = 2;
            }
        } else if (relation == 5) {
            if (enemydir == 0 || enemydir == 2) {
                choose_actions[choose_actions_count++] = 2;
                sub_actions[sub_actions_count++] = 3;
            } else {
                choose_actions[choose_actions_count++] = 3;
                sub_actions[sub_actions_count++] = 2;
            }
        } else {
            if (enemydir == 0 || enemydir == 2) {
                choose_actions[choose_actions_count++] = 0;
                sub_actions[sub_actions_count++] = 3;
            } else {
                choose_actions[choose_actions_count++] = 3;
                sub_actions[sub_actions_count++] = 0;
            }
        }
    } else if (dis2_snakes_count == 2) {
        const MyPos &pos1 = nownode.player_snakes[player][dis2_snakes[0]].front();
        const MyPos &pos2 = nownode.player_snakes[player][dis2_snakes[1]].front();
        char rel1 = get_2_relational(nowpos, pos1);
        char rel2 = get_2_relational(nowpos, pos2);
        if (rel1 % 2 == 0 && abs(rel1 - rel2) % 2 == 0) { //两偶方向
            switch (rel1) {
            case 0:
                choose_actions[choose_actions_count++] = 0;
                break;
            case 2:
                choose_actions[choose_actions_count++] = 1;
                break;
            case 4:
                choose_actions[choose_actions_count++] = 2;
                break;
            case 6:
                choose_actions[choose_actions_count++] = 3;
                break;
            default:
                break;
            }
            switch (rel2) {
            case 0:
                choose_actions[choose_actions_count++] = 0;
                break;
            case 2:
                choose_actions[choose_actions_count++] = 1;
                break;
            case 4:
                choose_actions[choose_actions_count++] = 2;
                break;
            case 6:
                choose_actions[choose_actions_count++] = 3;
                break;
            default:
                break;
            }
        } else if (rel1 % 2 == 1 && abs(rel1 - rel2) % 2 == 0) { //两奇方向
            if (abs(rel1 - rel2) == 4) {
                // do nothing
            } else {
                if (rel1 + rel2 == 8) {
                    choose_actions[choose_actions_count++] = 1;
                    choose_actions[choose_actions_count++] = 3;
                } else {
                    choose_actions[choose_actions_count++] = 0;
                    choose_actions[choose_actions_count++] = 2;
                }
            }
        } else { //一奇一偶,对奇下手
            char relation;
            char enemydir;
            if (rel1 % 2 == 1) {
                enemydir = nownode.player_snakes[player][dis2_snakes[0]].getdir();
                relation = rel1;
            } else {
                enemydir = nownode.player_snakes[player][dis2_snakes[1]].getdir();
                relation = rel2;
            }
            if (relation == 1) {
                if (enemydir == 0 || enemydir == 2) {
                    choose_actions[choose_actions_count++] = 0;
                    sub_actions[sub_actions_count++] = 1;
                } else {
                    choose_actions[choose_actions_count++] = 1;
                    sub_actions[sub_actions_count++] = 0;
                }
            } else if (relation == 3) {
                if (enemydir == 0 || enemydir == 2) {
                    choose_actions[choose_actions_count++] = 2;
                    sub_actions[sub_actions_count++] = 1;
                } else {
                    choose_actions[choose_actions_count++] = 1;
                    sub_actions[sub_actions_count++] = 2;
                }
            } else if (relation == 5) {
                if (enemydir == 0 || enemydir == 2) {
                    choose_actions[choose_actions_count++] = 2;
                    sub_actions[sub_actions_count++] = 3;
                } else {
                    choose_actions[choose_actions_count++] = 3;
                    sub_actions[sub_actions_count++] = 2;
                }
            } else {
                if (enemydir == 0 || enemydir == 2) {
                    choose_actions[choose_actions_count++] = 0;
                    sub_actions[sub_actions_count++] = 3;
                } else {
                    choose_actions[choose_actions_count++] = 3;
                    sub_actions[sub_actions_count++] = 0;
                }
            }
        }
    } else if (dis2_snakes_count >= 3) {
        char maxqi = -1;
        char maxdir = -1;
        char tx, ty, qi;
        for (char i = 0; i < 4; i++) {
            tx = nowpos.x + dx[i];
            ty = nowpos.y + dy[i];
            if (tx >= 0 && tx <= 15 && ty >= 0 && ty <= 15) {
                qi = get_qi(nownode, MyPos(tx, ty));
                if (qi > maxqi) {
                    maxqi = qi;
                    maxdir = i;
                }
            }
        }
        choose_actions[choose_actions_count++] = maxdir;
        for (char i = 0; i < 4; i++) {
            tx = nowpos.x + dx[i];
            ty = nowpos.y + dy[i];
            if (tx >= 0 && tx <= 15 && ty >= 0 && ty <= 15) {
                qi = get_qi(nownode, MyPos(tx, ty));
                if (qi == maxqi && i != maxdir) {
                    choose_actions[choose_actions_count++] = i;
                }
            }
        }
    } else if (dis1_snakes_count == 1) {
        char enemydir = nownode.player_snakes[player][dis1_snakes[0]].getdir();
        char mydir = nowsnake.getdir();
        if (enemydir == mydir || nowsnake.length() == 2) {
            choose_actions[choose_actions_count++] = enemydir;
        }
    }
    for (char i = 0; i < choose_actions_count;) {
        if (actions[choose_actions[i]] != 0) { // remove
            for (char j = i; j < choose_actions_count - 1; j++) {
                choose_actions[j] = choose_actions[j + 1];
            }
            choose_actions_count--;
        } else {
            i++;
        }
    }
    for (char i = 0; i < sub_actions_count;) {
        if (actions[sub_actions[i]] != 0) { // remove
            for (char j = i; j < sub_actions_count - 1; j++) {
                sub_actions[j] = sub_actions[j + 1];
            }
            sub_actions_count--;
        } else {
            i++;
        }
    }

    if (choose_actions_count == 0 && sub_actions_count == 0) {
        return -1;
    } else if (choose_actions_count >= 1) {
        return choose_actions[rand() % choose_actions_count] + 1;
    } else if (sub_actions_count >= 1) {
        return sub_actions[rand() % sub_actions_count] + 1;
    }
    return -1;
}
inline char enemy_pos_judge(Node &nownode, char valid_actions[], char action_count) {
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    const MyPos &nowpos = nowsnake.front();
    bool player = nownode.nowaction;
    short fieldx, fieldy;
    fieldx = fieldy = 0;
    bool should_action = false;
    // for (char i = 0; i < nownode.snakecount[player]; i++) {
    //     if (i != nownode.nownumber) {
    //         const MyPos &otherpos = nownode.player_snakes[player][i].front();
    //         char dis = distance(otherpos, nowpos);
    //         if (dis <= 5 &&
    //             (nownode.player_snakes[player][i].length() + nownode.player_snakes[player][i].length_bank > 2) &&
    //             get_qi(nownode, otherpos) == 1) {
    //             should_action = true;
    //             fieldx += otherpos.x - nowpos.x;
    //             fieldy += otherpos.y - nowpos.y;
    //         }
    //     }
    // }
    player = !player;
    for (char i = 0; i < nownode.snakecount[player]; i++) {
        const MyPos &otherpos = nownode.player_snakes[player][i].front();
        char dis = distance(otherpos, nowpos);
        char qi = get_qi(nownode, otherpos);
        if (dis <= 6 &&
            (nownode.player_snakes[player][i].length() + nownode.player_snakes[player][i].length_bank > 2) && qi <= 2) {
            should_action = true;
            fieldx += 5 * (nownode.player_snakes[player][i].length() + nownode.player_snakes[player][i].length_bank) *
                      (qi == 1 ? 2 : 1) * (otherpos.x - nowpos.x) / (dis * dis);
            fieldy += 5 * (nownode.player_snakes[player][i].length() + nownode.player_snakes[player][i].length_bank) *
                      (qi == 1 ? 2 : 1) * (otherpos.y - nowpos.y) / (dis * dis);
        }
    }

    if (!should_action) {
        return -1;
    }
    short max_value = abs(fieldx) + abs(fieldy);
    if (max_value == 0) {
        return -1;
    } else {
        short action_value[4];
        short total = 0;
        for (char i = 0; i < action_count; i++) {
            if (valid_actions[i] == 1) {
                action_value[i] = fieldx + max_value;
            } else if (valid_actions[i] == 2) {
                action_value[i] = fieldy + max_value;
            } else if (valid_actions[i] == 3) {
                action_value[i] = -fieldx + max_value;
            } else {
                action_value[i] = -fieldy + max_value;
            }
            total += action_value[i];
        }
        if (total == 0) {
            return -1;
        } else {
            short rand_val = rand() % total;
            for (char i = 0; i < action_count; i++) {
                if (action_value[i] > rand_val) {
                    return valid_actions[i];
                }
                rand_val -= action_value[i];
            }
        }
        return -1;
    }
}

inline char distance_actual(Node &nownode, const MyPos &pos1, const MyPos &item) {
    char dis = abs(pos1.x - item.x) + abs(pos1.y - item.y);
    if (nownode.map[item.x][item.y] == -1 || nownode.map[item.x][item.y] == -2) {
        return dis + 2;
    } else {
        return dis;
    }
    if (dis > 4 || dis <= 1) {
        return dis;
    }

    char nowx = pos1.x;
    char nowy = pos1.y;
    char x_bias, y_bias;
    while (nowx != item.x || nowy != item.y) {
        x_bias = item.x - nowx;
        y_bias = item.y - nowy;
        if (abs(x_bias) + abs(y_bias) == 1) {
            return dis;
        }
        if (abs(x_bias) < abs(y_bias)) {
            if (y_bias < 0) {
                if (nownode.map[nowx][nowy - 1] == CHAR_MIN) {
                    nowy--;
                } else if (x_bias < 0 && nownode.map[nowx - 1][nowy] == CHAR_MIN) {
                    nowx--;
                } else if (x_bias > 0 && nownode.map[nowx + 1][nowy] == CHAR_MIN) {
                    nowx++;
                } else {
                    return dis + 2;
                }
            } else {
                if (nownode.map[nowx][nowy + 1] == CHAR_MIN) {
                    nowy++;
                } else if (x_bias < 0 && nownode.map[nowx - 1][nowy] == CHAR_MIN) {
                    nowx--;
                } else if (x_bias > 0 && nownode.map[nowx + 1][nowy] == CHAR_MIN) {
                    nowx++;
                } else {
                    return dis + 2;
                }
            }
        } else {
            if (x_bias < 0) {
                if (nownode.map[nowx - 1][nowy] == CHAR_MIN) {
                    nowx--;
                } else if (y_bias < 0 && nownode.map[nowx][nowy - 1] == CHAR_MIN) {
                    nowy--;
                } else if (y_bias > 0 && nownode.map[nowx][nowy + 1] == CHAR_MIN) {
                    nowy++;
                } else {
                    return dis + 2;
                }
            } else {
                if (nownode.map[nowx + 1][nowy] == CHAR_MIN) {
                    nowx++;
                } else if (y_bias < 0 && nownode.map[nowx][nowy - 1] == CHAR_MIN) {
                    nowy--;
                } else if (y_bias > 0 && nownode.map[nowx][nowy + 1] == CHAR_MIN) {
                    nowy++;
                } else {
                    return dis + 2;
                }
            }
        }
    }
    return dis;
}

inline float item_value_judge(Node &nownode) {
    float val = 0;
    MyPos heads[2][4];
    char id[2][4];
    char dis[2][4];
    // char round_bias[2][4];
    char mindis[2];
    // char mindis_id[2];
    char snakecount[2];
    snakecount[0] = nownode.snakecount[0];
    snakecount[1] = nownode.snakecount[1];
    for (char i = 0; i < 2; i++) {
        for (char j = 0; j < nownode.snakecount[i]; j++) {
            heads[i][j] = nownode.player_snakes[i][j].front();
            id[i][j] = nownode.player_snakes[i][j].id;
            // if (i <= nownode.nowaction && j < nownode.nownumber) {
            //     round_bias[i][j] = 0;
            // } else {
            //     round_bias[i][j] = -1;
            // }
        }
    }

    for (char i = 0; i < nownode.itemcount; i++) {
        MyPos item_pos = MyPos(nownode.itemlist[i].x, nownode.itemlist[i].y);
        char item_val = nownode.itemlist[i].type == 0 ? nownode.itemlist[i].param : 3;
        short time_last = nownode.itemlist[i].time + 16 - nownode.round;
        mindis[0] = mindis[1] = 100;
        for (char j = 0; j < 2; j++) {
            for (char k = 0; k < snakecount[j]; k++) {
                dis[j][k] = distance_actual(nownode, heads[j][k], item_pos);
                mindis[j] = min(dis[j][k], mindis[j]);
            }
        }
        if (mindis[0] > time_last && mindis[1] > time_last) {
            continue;
        }
        if (mindis[0] + 1 < mindis[1]) {
            val += item_val / (float)(max(mindis[0], 2.0));
        } else if (mindis[0] > mindis[1] + 1) {
            val -= item_val / (float)(max(mindis[1], 2.0));
        } else {
            val += item_val / (float)(max(mindis[0], 2.0)) - item_val / (float)(max(mindis[1], 2.0));
        }
    }

    for (short i = 0; i < future_item_rounds_count[nownode.round]; i++) { //向前搜
        auto &c = gen_itemlist[future_item_rounds[nownode.round][i]];
        if (c.time - nownode.round > 20) {
            break;
        }
        MyPos item_pos = MyPos(c.x, c.y);
        bool tobe_eaten = false;
        if (nownode.map[c.x][c.y] >= 0) { //在某条蛇身上
            for (char j = 0; j < 2; j++) {
                for (char k = 0; k < snakecount[j]; k++) {
                    if (nownode.map[c.x][c.y] == id[j][k]) {
                        MySnake &nowsnake = nownode.player_snakes[j][k];
                        short left;
                        for (char l = nowsnake.head; l < nowsnake.tail; l++) {
                            if (nowsnake.pos_list[l].x == c.x && nowsnake.pos_list[l].y == c.y) {
                                left = nowsnake.tail + nowsnake.length_bank - l;
                                break;
                            }
                        }
                        if (left > c.time - nownode.round) { //必吃到
                            if (j == 0) {
                                val += c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3);
                            } else {
                                val -= c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3);
                            }

                            tobe_eaten = true;
                            break;
                        } else { //给奖励
                            if (j == 0) {
                                val += (c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3)) / 3.0f;
                            } else {
                                val -= (c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3)) / 3.0f;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (!tobe_eaten) {
            char item_val = c.type == 0 ? c.param : 3;
            mindis[0] = mindis[1] = 100;
            for (char j = 0; j < 2; j++) {
                for (char k = 0; k < snakecount[j]; k++) {
                    dis[j][k] = distance_actual(nownode, heads[j][k], item_pos);
                    mindis[j] = min(dis[j][k], mindis[j]);
                }
            }
            if (mindis[0] + 1 < mindis[1]) {
                val += item_val / (float)(max(mindis[0], 2.0));
            } else if (mindis[0] > mindis[1] + 1) {
                val -= item_val / (float)(max(mindis[1], 2.0));
            } else {
                val += item_val / (float)(max(mindis[0], 2.0)) - item_val / (float)(max(mindis[1], 2.0));
            }
        }
    }
    return val;
}

inline float snake_value_judge(Node &nownode) {
    float val[2];
    char qi[2][4];
    MyPos pos[2][4];
    char length[2][4];
    char snakecount[2];
    // bool chosen[2][4];
    val[0] = val[1] = 0;
    // memset(chosen, 0, sizeof(chosen));
    snakecount[0] = nownode.snakecount[0];
    snakecount[1] = nownode.snakecount[1];
    for (char i = 0; i < 2; i++) {
        for (char j = 0; j < nownode.snakecount[i]; j++) {
            length[i][j] = nownode.player_snakes[i][j].length() + nownode.player_snakes[i][j].length_bank;
            pos[i][j] = nownode.player_snakes[i][j].front();
            qi[i][j] = get_qi(nownode, pos[i][j]);
        }
    }
    for (char i = 0; i < 2; i++) {
        for (char j = 0; j < snakecount[i]; j++) {
            if (qi[i][j] == 0) {
                val[i] -= length[i][j];
            } else if (qi[i][j] == 1) {
                for (char k = 0; k < snakecount[!i]; k++) {
                    char dist = distance(pos[i][j], pos[!i][k]);
                    if (qi[!i][k] >= 1 && dist <= 5) {
                        val[i] -= length[i][j] / 1.5 / dist;
                    }
                }
            } else if (qi[i][j] == 2) {
                for (char k = 0; k < snakecount[!i]; k++) {
                    char dist = distance(pos[i][j], pos[!i][k]);
                    if (qi[!i][k] >= 1 && dist <= 5) {
                        val[i] -= length[i][j] / 2.0 / dist;
                    }
                }
            } else {
                for (char k = 0; k < snakecount[!i]; k++) {
                    char dist = distance(pos[i][j], pos[!i][k]);
                    if (qi[!i][k] >= 1 && dist <= 5) {
                        val[i] -= length[i][j] / 2.5 / dist;
                    }
                }
            }
        }
    }
    return (val[0] - val[1]) / 2.0;
}

inline char if_border(Node &nownode, const MyPos &pos, char dir, char snakeid) {
    char x1, x2, y1, y2;
    if (dir == 0 || dir == 2) {
        x1 = x2 = pos.x;
        y1 = pos.y + 1;
        y2 = pos.y - 1;
    } else {
        y1 = y2 = pos.y;
        x1 = pos.x + 1;
        x2 = pos.x - 1;
    }
    // char count1=(x1>=0&&x1<=15&&y1>=0&&y1<=15&&nownode.map[x1][y1]!=CHAR_MIN&&nownode.map[x1][y1]!=snakeid)?1:0;
    // char count2=(x2>=0&&x2<=15&&y2>=0&&y2<=15&&nownode.map[x2][y2]!=CHAR_MIN&&nownode.map[x2][y2]!=snakeid)?1:0;
    char is1 = 1;
    char is2 = 1;
    for (int i = 0; i < 3; i++) {
        x1 += dx[dir];
        x2 += dx[dir];
        y1 += dy[dir];
        y2 += dy[dir];
        if (is1 && x1 >= 0 && x1 <= 15 && y1 >= 0 && y1 <= 15 && nownode.map[x1][y1] != CHAR_MIN &&
            nownode.map[x1][y1] != snakeid) {
            // count1++;
        } else {
            is1 = 0;
        }
        if (is2 && x2 >= 0 && x2 <= 15 && y2 >= 0 && y2 <= 15 && nownode.map[x2][y2] != CHAR_MIN &&
            nownode.map[x2][y2] != snakeid) {
            // count1++;
        } else {
            is2 = 0;
        }
    }
    return is1 + is2;
}
inline float border_value_judge(Node &nownode, MySnake &nowsnake, short len) {
    if (nowsnake.length() == 1) {
        return 0;
    }
    char border = if_border(nownode, nowsnake.front(), nowsnake.getdir(), nowsnake.id);
    if (border == 0) {
        return 0;
    } else if (border == 1) {
        return len / 6.0;
    } else {
        return len / 4.0;
    }
}

float value_judge(Node &nownode) { // TODO估值,正数代表对0有利
    float val = 0;
    for (char i = 0; i < 16; i++) {
        for (char j = 0; j < 16; j++) {
            if (nownode.map[i][j] == -1) {
                val++;
            } else if (nownode.map[i][j] == -2) {
                val--;
            }
        }
    }
    if (nownode.round <= max_round) {
        short length1 = 0;
        short length2 = 0;
        for (char i = 0; i < nownode.snakecount[0]; i++) {
            auto &c = nownode.player_snakes[0][i];
            short len = c.length_bank + c.length();
            length1 += len;
            val += (c.railgun_item ? 3 : 0);
            val -= border_value_judge(nownode, c, len);
        }
        for (char i = 0; i < nownode.snakecount[1]; i++) {
            auto &c = nownode.player_snakes[1][i];
            short len = c.length_bank + c.length();
            length2 += len;
            val -= (c.railgun_item ? 3 : 0);
            val += border_value_judge(nownode, c, len);
        }
        if (nownode.snakecount[0] == 1) {
            val -= 3;
        } else if (nownode.snakecount[0] == 2) {
            val -= 1;
        } else if (nownode.snakecount[0] == 0) {
            val -= (max_round - nownode.round) / 6.0f;
        }
        if (nownode.snakecount[1] == 1) {
            val += 3;
        } else if (nownode.snakecount[1] == 2) {
            val += 1;
        } else if (nownode.snakecount[1] == 0) {
            val += (max_round - nownode.round) / 6.0f;
        }
        float length_val = ((length1 - length2) * 1.2f);
        if (length1 < 8) {
            val -= (8 - length1) * 0.5f;
        }
        if (length2 < 8) {
            val += (8 - length2) * 0.5f;
        }
        val += length_val;
        val += item_value_judge(nownode);
        val += snake_value_judge(nownode);
    } else {
        for (char i = 0; i < nownode.snakecount[0]; i++) {
            auto &c = nownode.player_snakes[0][i];
            val += c.length();
        }
        for (char i = 0; i < nownode.snakecount[1]; i++) {
            auto &c = nownode.player_snakes[1][i];
            val -= c.length();
        }
    }

    return val;
}

short ray_value(Node &nownode) {
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    if (nowsnake.length() < 2 || !nowsnake.railgun_item) {
        return CHAR_MIN;
    }
    char dx = nowsnake[0].x - nowsnake[1].x;
    char dy = nowsnake[0].y - nowsnake[1].y;
    char nx = nowsnake[0].x + dx;
    char ny = nowsnake[0].y + dy;
    short val = 0;
    while (nx >= 0 && nx < 16 && ny >= 0 && ny < 16) {
        if (nownode.map[nx][ny] == -1) {
            val--;
        } else if (nownode.map[nx][ny] == -2) {
            val++;
        }
        nx += dx;
        ny += dy;
    }
    return nownode.nowaction == 0 ? val : -val;
}

void randomplay(int node_index, short simround, bool collision, int root_index) {
    if (!node[node_index].end) {
        Node tmpnode = node[node_index];
        char depth = 0;
        process_items(tmpnode);
        bool nowaction = node[node_index].nowaction;
        while (depth < simround) {
            if (tmpnode.snakecount[nowaction] == 0) {
                break;
            } else if (tmpnode.snakecount[!nowaction] == 0 && depth > 10) {
                break;
            }
            char length_flag = false;
            for (char i = 0; i < 2; i++) {
                for (char j = 0; j < tmpnode.snakecount[i]; j++) {
                    if (tmpnode.player_snakes[i][j].length() >= MAX_LENGTH - 2) {
                        length_flag = true;
                        break;
                    }
                }
                if (length_flag) {
                    break;
                }
            }
            if (length_flag) { //避免蛇过长
                break;
            }
            get_current_actions(tmpnode);
            char validcount = actions[4];
            char solidcount = actions[5];
            short rayvalue = ray_value(tmpnode);
            char choose_action = -1;
            MySnake &nowsnake = tmpnode.player_snakes[tmpnode.nowaction][tmpnode.nownumber];
            char nowlength = nowsnake.length() + nowsnake.length_bank;
            bool could_split = true;
            bool should_suicide = false;
            for (int i = 0; i < tmpnode.snakecount[tmpnode.nowaction]; i++) {
                if (i != tmpnode.nownumber) {
                    if (get_qi(tmpnode, tmpnode.player_snakes[tmpnode.nowaction][i].front()) == 0) {
                        could_split = false;
                        if (nowlength * 2 < tmpnode.player_snakes[tmpnode.nowaction][i].length() / 2 +
                                                tmpnode.player_snakes[tmpnode.nowaction][i].length_bank) {
                            should_suicide = true;
                            break;
                        }
                    }
                }
            }
            if (should_suicide && (solidcount > 0 || actions[6] > 0)) {
                if (solidcount > 0) {
                    if (solidcount == 1) {
                        if (actions[0] == 1) {
                            choose_action = 1;
                        } else if (actions[1] == 1) {
                            choose_action = 2;
                        } else if (actions[2] == 1) {
                            choose_action = 3;
                        } else {
                            choose_action = 4;
                        }
                    } else { //选最优的固化方式
                        MyPos targets[4];
                        char targets_actions[4];
                        char targets_count = 0;
                        if (actions[0] == 1) {
                            targets_actions[targets_count] = 1;
                            targets[targets_count++] = MyPos(nowsnake.front().x + 1, nowsnake.front().y);
                        }
                        if (actions[1] == 1) {
                            targets_actions[targets_count] = 2;
                            targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y + 1);
                        }
                        if (actions[2] == 1) {
                            targets_actions[targets_count] = 3;
                            targets[targets_count++] = MyPos(nowsnake.front().x - 1, nowsnake.front().y);
                        }
                        if (actions[3] == 1) {
                            targets_actions[targets_count] = 4;
                            targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y - 1);
                        }
                        bool findflag = false;
                        for (char k = nowsnake.tail - 1; k >= nowsnake.head; k--) {
                            for (char i = 0; i < targets_count; i++) {
                                if (nowsnake.pos_list[k] == targets[i]) {
                                    findflag = true;
                                    choose_action = targets_actions[i];
                                    break;
                                }
                            }
                            if (findflag) {
                                break;
                            }
                        }
                    }
                } else {
                    if (actions[0] == -1) {
                        choose_action = 1;
                    } else if (actions[1] == -1) {
                        choose_action = 2;
                    } else if (actions[2] == -1) {
                        choose_action = 3;
                    } else {
                        choose_action = 4;
                    }
                }
            }
            if (choose_action == -1) {
                if (could_split && tmpnode.snakecount[tmpnode.nowaction] < tmpnode.countbound[tmpnode.nowaction] &&
                    rand() % 3 == 0) { //随机分裂
                    choose_action = 6;
                } else if (rayvalue >= 3) { //高收益射线
                    choose_action = 5;
                } else if (validcount > 0) { // validcount
                    char valid_actions[4];
                    short action_count = 0;
                    if (actions[0] == 0) {
                        valid_actions[action_count] = 1;
                        action_count++;
                    }
                    if (actions[1] == 0) {
                        valid_actions[action_count] = 2;
                        action_count++;
                    }
                    if (actions[2] == 0) {
                        valid_actions[action_count] = 3;
                        action_count++;
                    }
                    if (actions[3] == 0) {
                        valid_actions[action_count] = 4;
                        action_count++;
                    }
                    //决策
                    if (action_count == 1) {
                        choose_action = valid_actions[0];
                    } else {
                        if (collision) {
                            char local_action = local_action_judge(tmpnode, valid_actions, action_count);
                            if (local_action != -1) {
                                choose_action = local_action;
                            } else {
                                char enemy_pos_action = enemy_pos_judge(tmpnode, valid_actions, action_count);
                                if (enemy_pos_action != -1) {
                                    choose_action = enemy_pos_action;
                                } else {
                                    choose_action = valid_actions[rand() % action_count];
                                }
                            }
                        } else {
                            short fieldx, fieldy;
                            fieldx = fieldy = 0;
                            MyPos nowpos = tmpnode.player_snakes[tmpnode.nowaction][tmpnode.nownumber].front();
                            char dx, dy, dis;
                            short value;
                            for (char i = 0; i < tmpnode.itemcount; i++) {
                                MyItem &c = tmpnode.itemlist[i];
                                dx = c.x - nowpos.x;
                                dy = c.y - nowpos.y;
                                dis = abs(dx) + abs(dy);
                                if ((tmpnode.map[c.x][c.y] == CHAR_MIN || tmpnode.map[c.x][c.y] == nowsnake.id) &&
                                    dis <= c.time + 16 - tmpnode.round && dis != 0 &&
                                    dis <= 6) { //有可能吃到且不在自身位置
                                    value = c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3);
                                    fieldx += value * 100 * dx / (dis * dis);
                                    fieldy += value * 100 * dy / (dis * dis);
                                }
                            }
                            for (short i = 0; i < future_item_rounds_count[tmpnode.round]; i++) { //向前搜
                                auto &c = gen_itemlist[future_item_rounds[tmpnode.round][i]];
                                dx = c.x - nowpos.x;
                                dy = c.y - nowpos.y;
                                dis = abs(dx) + abs(dy);
                                if ((tmpnode.map[c.x][c.y] == CHAR_MIN || tmpnode.map[c.x][c.y] == nowsnake.id) &&
                                    dis <= c.time + 16 - tmpnode.round && dis != 0 &&
                                    dis <= 6) { //有可能吃到且不在自身位置
                                    value = c.type == 0 ? c.param : (nowsnake.railgun_item ? 1 : 3);
                                    fieldx += value * 100 * dx / (dis * dis);
                                    fieldy += value * 100 * dy / (dis * dis);
                                }
                            }
                            //}
                            short max_value = abs(fieldx) + abs(fieldy);
                            if (max_value == 0) {
                                choose_action = valid_actions[rand() % action_count];
                            } else {
                                short action_value[4];
                                short total = 0;
                                for (char i = 0; i < action_count; i++) {
                                    if (valid_actions[i] == 1) {
                                        action_value[i] = fieldx + max_value;
                                    } else if (valid_actions[i] == 2) {
                                        action_value[i] = fieldy + max_value;
                                    } else if (valid_actions[i] == 3) {
                                        action_value[i] = -fieldx + max_value;
                                    } else {
                                        action_value[i] = -fieldy + max_value;
                                    }
                                    total += action_value[i];
                                }
                                if (total == 0) {
                                    choose_action = valid_actions[rand() % action_count];
                                } else {
                                    short rand_val = rand() % total;
                                    for (char i = 0; i < action_count; i++) {
                                        if (action_value[i] > rand_val) {
                                            choose_action = valid_actions[i];
                                            break;
                                        }
                                        rand_val -= action_value[i];
                                    }
                                }
                            }
                        }
                    }
                } else if (rayvalue + nowsnake.length() + nowsnake.length_bank > 1) {
                    choose_action = 5;
                } else if (solidcount > 0) {
                    if (tmpnode.snakecount[tmpnode.nowaction] >
                        tmpnode.countbound[tmpnode.nowaction] - 1) { //蛇够多,固化
                        if (solidcount == 1) {
                            if (actions[0] == 1) {
                                choose_action = 1;
                            } else if (actions[1] == 1) {
                                choose_action = 2;
                            } else if (actions[2] == 1) {
                                choose_action = 3;
                            } else {
                                choose_action = 4;
                            }
                        } else { //选最优的固化方式
                            MyPos targets[4];
                            char targets_actions[4];
                            char targets_count = 0;
                            if (actions[0] == 1) {
                                targets_actions[targets_count] = 1;
                                targets[targets_count++] = MyPos(nowsnake.front().x + 1, nowsnake.front().y);
                            }
                            if (actions[1] == 1) {
                                targets_actions[targets_count] = 2;
                                targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y + 1);
                            }
                            if (actions[2] == 1) {
                                targets_actions[targets_count] = 3;
                                targets[targets_count++] = MyPos(nowsnake.front().x - 1, nowsnake.front().y);
                            }
                            if (actions[3] == 1) {
                                targets_actions[targets_count] = 4;
                                targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y - 1);
                            }
                            bool findflag = false;
                            for (char k = nowsnake.tail - 1; k >= nowsnake.head; k--) {
                                for (char i = 0; i < targets_count; i++) {
                                    if (nowsnake.pos_list[k] == targets[i]) {
                                        findflag = true;
                                        choose_action = targets_actions[i];
                                        break;
                                    }
                                }
                                if (findflag) {
                                    break;
                                }
                            }
                        }
                    } else { //分裂(保证能分裂)
                        choose_action = 6;
                    }
                } else if (tmpnode.snakecount[tmpnode.nowaction] < tmpnode.countbound[tmpnode.nowaction] &&
                           tmpnode.player_snakes[tmpnode.nowaction][tmpnode.nownumber].length() > 2) {
                    choose_action = 6;
                } else {
                    if (actions[0] == -1) {
                        choose_action = 1;
                    } else if (actions[1] == -1) {
                        choose_action = 2;
                    } else if (actions[2] == -1) {
                        choose_action = 3;
                    } else {
                        choose_action = 4;
                    }
                }
            }

            bool endgame = tonext(tmpnode, choose_action);
            depth++;

            if (endgame) {
                break;
            }
        }
        float val = value_judge(tmpnode);
        float diff = val - node[root_index].origin;
        float expo = -diff / 3;
        float winval = 1 / (1 + exp(expo)); // sigmoid
        while (node_index != -1) {
            node[node_index].count += 1;
            node[node_index].win += winval;
            node_index = node[node_index].p;
        }
    } else {
        float diff = node[node_index].origin - node[root_index].origin;
        float expo = -diff / 3;
        float winval = 1 / (1 + exp(expo)); // sigmoid
        while (node_index != -1) {
            node[node_index].count += 1;
            node[node_index].win += winval;
            node_index = node[node_index].p;
        }
    }
}

void newnode(int index, char action) {
    node[nodecount] = node[index];
    node[nodecount].lc = node[nodecount].rc = -1;
    node[nodecount].p = index;
    node[nodecount].count = node[nodecount].origin = node[nodecount].win = 0;
    node[nodecount].useaction = action;
    if (node[index].lc == -1) {
        node[index].lc = node[index].rc = nodecount;
    } else {
        node[index].rc = nodecount;
    }
    Node &nownode = node[nodecount];
    nodecount++;
    MySnake &nowsnake = nownode.player_snakes[nownode.nowaction][nownode.nownumber];
    if (action <= 4) {
        //蛇头到达位置
        char t_x = nowsnake.front().x + dx[action - 1];
        char t_y = nowsnake.front().y + dy[action - 1];

        if (t_x >= 16 || t_x < 0 || t_y >= 16 || t_y < 0 ||
            (nownode.map[t_x][t_y] != CHAR_MIN && nownode.map[t_x][t_y] != nowsnake.id)) {
            for (char k = nowsnake.head; k < nowsnake.tail; k++) {
                nownode.map[nowsnake.pos_list[k].x][nowsnake.pos_list[k].y] = CHAR_MIN;
            }
            nownode.removesnake(nownode.nowaction, nownode.nownumber);
        } else if (nownode.map[t_x][t_y] == nowsnake.id &&
                   !(nowsnake.back().x == t_x && nowsnake.back().y == t_y && nowsnake.length_bank == 0)) {
            char xmin, xmax, ymin, ymax;
            xmin = ymin = 16;
            xmax = ymax = 0;
            char k = nowsnake.head;
            for (; k < nowsnake.tail; k++) {
                MyPos &c = nowsnake.pos_list[k];
                if (c.x < xmin) {
                    xmin = c.x;
                }
                if (c.x > xmax) {
                    xmax = c.x;
                }
                if (c.y < ymin) {
                    ymin = c.y;
                }
                if (c.y > ymax) {
                    ymax = c.y;
                }
                if (c.x == t_x && c.y == t_y) {
                    break;
                }
            }
            k++;
            for (; k < nowsnake.tail; k++) { //删除后面的
                nownode.map[nowsnake.pos_list[k].x][nowsnake.pos_list[k].y] = CHAR_MIN;
            }
            char id = nowsnake.id;
            nownode.removesnake(nownode.nowaction, nownode.nownumber);
            fill(nownode, xmin, xmax, ymin, ymax, id);
        } else {
            if (nowsnake.length_bank > 0) {
                nowsnake.push_front(MyPos(t_x, t_y));
                nowsnake.length_bank--;
            } else {
                nownode.map[nowsnake.back().x][nowsnake.back().y] = CHAR_MIN;
                nowsnake.pop_back();
                nowsnake.push_front(MyPos(t_x, t_y));
            }
            nownode.map[t_x][t_y] = nowsnake.id;
            for (char i = 0; i < nownode.itemcount;) {
                MyItem c = nownode.itemlist[i];
                if (c.x == t_x && c.y == t_y && c.time + 16 > nownode.round) { //同位置,且未过期
                    if (c.type == 0) {
                        nowsnake.length_bank += c.param;
                    } else {
                        nowsnake.railgun_item = true;
                    }
                    nownode.item_remove(i);
                    break;
                }
                i++;
            }
        }
    } else if (action == 5) {
        char dx = nowsnake[0].x - nowsnake[1].x;
        char dy = nowsnake[0].y - nowsnake[1].y;
        char nx = nowsnake[0].x + dx;
        char ny = nowsnake[0].y + dy;
        while (nx >= 0 && nx < 16 && ny >= 0 && ny < 16) {
            if (nownode.map[nx][ny] < 0) {
                nownode.map[nx][ny] = CHAR_MIN;
            }
            nx += dx;
            ny += dy;
        }
        nowsnake.railgun_item = false;
    } else if (action == 6) {
        nownode.split();
    }

    if (nownode.snakecount[0] == 0 && nownode.snakecount[1] == 0) {
        nownode.end = true;
    }

    if (nownode.end) {
        if (nownode.nowaction == 0 && nownode.nownumber + 1 == nownode.snakecount[0]) {
            nownode.nowaction = 1;
        } else if (nownode.nowaction == 1 && nownode.nownumber + 1 == nownode.snakecount[1]) {
            nownode.nowaction = 0;
        }
        nownode.origin = value_judge(nownode);
    } else {
        if (nownode.nowaction == 0) {
            if (nownode.nownumber + 1 == nownode.snakecount[0]) { //换人
                if (nownode.snakecount[1] > 0) {                  // 1号有蛇
                    nownode.nowaction = 1;
                    nownode.nownumber = 0;
                } else { //直接下一轮
                    nownode.round++;
                    nownode.update_items();
                    nownode.nownumber = 0;
                    if (nownode.round > max_round) { //最大回合,结束
                        nownode.end = true;
                        nownode.origin = value_judge(nownode);
                    } else {
                        //生成道具
                        if (if_gen_item[nownode.round]) {
                            auto &c = gen_itemlist[nownode.round];
                            if (nownode.map[c.x][c.y] >= 0) { //当即被吃掉
                                if (c.type == 0) {
                                    nownode.findsnake(nownode.map[c.x][c.y]).length_bank += c.param;
                                } else {
                                    nownode.findsnake(nownode.map[c.x][c.y]).railgun_item = true;
                                }
                            } else { //生成在场面上
                                nownode.item_push_back(c);
                            }
                        }
                    }
                }
            } else {
                nownode.nownumber++;
            }
        } else {
            if (nownode.nownumber + 1 == nownode.snakecount[1]) { //换人
                if (nownode.snakecount[0] > 0) {                  // 0号有蛇
                    nownode.nowaction = 0;
                    nownode.nownumber = 0;
                } else { //直接下一轮
                    nownode.nownumber = 0;
                }
                nownode.round++;
                nownode.update_items();
                if (nownode.round > max_round) { //最大回合,结束
                    nownode.end = true;
                    nownode.origin = value_judge(nownode);
                } else {
                    //生成道具
                    if (if_gen_item[nownode.round]) {
                        auto &c = gen_itemlist[nownode.round];
                        if (nownode.map[c.x][c.y] >= 0) { //当即被吃掉
                            if (c.type == 0) {
                                nownode.findsnake(nownode.map[c.x][c.y]).length_bank += c.param;
                            } else {
                                nownode.findsnake(nownode.map[c.x][c.y]).railgun_item = true;
                            }
                        } else { //生成在场面上
                            nownode.item_push_back(c);
                        }
                    }
                }

            } else {
                nownode.nownumber++;
            }
        }
    }
}

void local_mcts_prepare() {
    Node &lroot = node[local_root];
    lroot.clean();
    bool keep[2][4];
    memset(keep, 0, sizeof(keep));
    MyPos centerpos = lroot.player_snakes[lroot.nowaction][lroot.nownumber].front();
    char centerx = centerpos.x;
    char centery = centerpos.y;
    //距离<=5
    for (char i = -5; i <= 5; i++) {
        for (char j = -5; j <= 5; j++) {
            if (centerx + i < 0 || centerx + i > 15 || centery + j < 0 || centery + j > 15) {
                continue;
            }
            if (lroot.map[centerx + i][centery + j] >= 0) {
                MyPos index = lroot.snakeindex(lroot.map[centerx + i][centery + j]);
                keep[index.x][index.y] = true;
            }
        }
    }
    for (char i = 0; i < 2; i++) {
        char bias = 0;
        for (char j = 0; j < lroot.snakecount[i];) {
            if (!keep[i][j + bias]) {
                bias++;
                lroot.countbound[i]--;
                for (char k = lroot.player_snakes[i][j].head; k < lroot.player_snakes[i][j].tail; k++) {
                    lroot.map[lroot.player_snakes[i][j].pos_list[k].x][lroot.player_snakes[i][j].pos_list[k].y] =
                        i == 0 ? -1 : -2;
                }
                lroot.removesnake(i, j);
            } else {
                j++;
            }
        }
    }
    lroot.origin = value_judge(lroot);
}

void MCTS() {
    node[root].origin = value_judge(node[root]);
    int now;
    int sim_count = 0;
    char a1, a2, a3, a4;
    char validcount, solidcount;
    while (nodecount < MAX_NODE_COUNT - 20100 - snakes_left * 40000 && (time_judge() || sim_count < 100) &&
           clock() - total_t < 950000) {
        sim_count++;
        now = utcbest(root);
        bool check_preend = false;
        if (!node[now].end) { //不是终结节点
            get_current_actions(node[now]);
            a1 = actions[0];
            a2 = actions[1];
            a3 = actions[2];
            a4 = actions[3];
            validcount = actions[4];
            solidcount = actions[5];
            bool killflag = false;
            MySnake &nowsnake = node[now].player_snakes[node[now].nowaction][node[now].nownumber];
            if (validcount > 0 || solidcount > 0) {
                if (a1 == 0) {
                    newnode(now, 1);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a1 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 1);
                        killflag = true;
                    }
                }
                if (a2 == 0) {
                    newnode(now, 2);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a2 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 2);
                        killflag = true;
                    }
                }
                if (a3 == 0) {
                    newnode(now, 3);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a3 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 3);
                        killflag = true;
                    }
                }
                if (a4 == 0) {
                    newnode(now, 4);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a4 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 4);
                        killflag = true;
                    }
                }
                if (solidcount <= 1) {
                    if (a1 == 1) {
                        newnode(now, 1);
                    } else if (a2 == 1) {
                        newnode(now, 2);
                    } else if (a3 == 1) {
                        newnode(now, 3);
                    } else if (a4 == 1) {
                        newnode(now, 4);
                    }
                } else { //只拓展最优的固化方案
                    MyPos targets[4];
                    char targets_actions[4];
                    char targets_count = 0;
                    if (a1 == 1) {
                        targets_actions[targets_count] = 1;
                        targets[targets_count++] = MyPos(nowsnake.front().x + 1, nowsnake.front().y);
                    }
                    if (a2 == 1) {
                        targets_actions[targets_count] = 2;
                        targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y + 1);
                    }
                    if (a3 == 1) {
                        targets_actions[targets_count] = 3;
                        targets[targets_count++] = MyPos(nowsnake.front().x - 1, nowsnake.front().y);
                    }
                    if (a4 == 1) {
                        targets_actions[targets_count] = 4;
                        targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y - 1);
                    }
                    bool findflag = false;
                    for (char k = nowsnake.tail - 1; k >= nowsnake.head; k--) {
                        for (char i = 0; i < targets_count; i++) {
                            if (nowsnake.pos_list[k] == targets[i]) {
                                findflag = true;
                                newnode(now, targets_actions[i]);
                                break;
                            }
                        }
                        if (findflag) {
                            break;
                        }
                    }
                }
            } else {
                if (a1 == -1) {
                    newnode(now, 1);
                } else if (a2 == -1) {
                    newnode(now, 2);
                } else if (a3 == -1) {
                    newnode(now, 3);
                } else if (a4 == -1) {
                    newnode(now, 4);
                }
            }
            if (node[now].player_snakes[node[now].nowaction][node[now].nownumber].railgun_item &&
                node[now].player_snakes[node[now].nowaction][node[now].nownumber].length() >= 2) {
                newnode(now, 5);
            }
            if (node[now].snakecount[node[now].nowaction] < node[now].countbound[node[now].nowaction] &&
                node[now].player_snakes[node[now].nowaction][node[now].nownumber].length() > 2 &&
                node[now].round > 15) {
                newnode(now, 6);
            }
            if (now == root && node[now].lc == node[now].rc - 1) {                                      //两子节点
                if (node[node[now].lc].useaction <= 4 && actions[node[node[now].lc].useaction] == -1) { //有自杀
                    check_preend = true;
                }
                if (node[node[now].rc].useaction <= 4 && actions[node[node[now].rc].useaction] == -1) {
                    check_preend = true;
                }
            }
        }
        randomplay(now, max(60, (node[now].snakecount[0] + node[now].snakecount[1]) * 10), true, root);
        randomplay(now, max(60, (node[now].snakecount[0] + node[now].snakecount[1]) * 10), false, root);
        if (now == root && node[now].lc == node[now].rc) { //单分支
            if (node[now].lc != -1) {
                node[node[now].lc].count = 1;
                node[node[now].lc].win = 0.5;
            }
            break;
        }
        if (check_preend && sim_count % 500 == 0 && sim_count >= 1000) {
            float val1 = node[node[root].lc].win / node[node[root].lc].count;
            float val2 = node[node[root].rc].win / node[node[root].rc].count;
            if (abs(val1 - val2) > 0.2) {
                break;
            }
        }
    }
    // std::cerr << current_round << " " << sim_count << " " << nodecount << std::endl;
}

void LOCAL_MCTS() {
    start_t = clock();
    local_root = nodecount;
    nodecount++;
    node[local_root] = node[root];
    local_mcts_prepare();
    int now;
    int sim_count = 0;
    char a1, a2, a3, a4;
    char validcount, solidcount;
    while (nodecount < MAX_NODE_COUNT - 100 - snakes_left * 40000 && (time_judge() || sim_count < 100) &&
           clock() - total_t < 950000) {
        sim_count++;
        now = utcbest(local_root);
        bool check_preend = false;
        if (!node[now].end) { //不是终结节点
            get_current_actions(node[now]);
            a1 = actions[0];
            a2 = actions[1];
            a3 = actions[2];
            a4 = actions[3];
            validcount = actions[4];
            solidcount = actions[5];
            bool killflag = false;
            MySnake &nowsnake = node[now].player_snakes[node[now].nowaction][node[now].nownumber];
            if (validcount > 0 || solidcount > 0) {
                if (a1 == 0) {
                    newnode(now, 1);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a1 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 1);
                        killflag = true;
                    }
                }
                if (a2 == 0) {
                    newnode(now, 2);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a2 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 2);
                        killflag = true;
                    }
                }
                if (a3 == 0) {
                    newnode(now, 3);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a3 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 3);
                        killflag = true;
                    }
                }
                if (a4 == 0) {
                    newnode(now, 4);
                } else if (node[now].round == current_round && node[now].nowaction == mycamp && !killflag &&
                           solidcount == 0 && a4 == -1) {
                    short nowlength = nowsnake.length() + nowsnake.length_bank;
                    short total = 0;
                    for (char i = 0; i < node[now].snakecount[node[now].nowaction]; i++) {
                        total += node[now].player_snakes[node[now].nowaction][i].length() +
                                 node[now].player_snakes[node[now].nowaction][i].length_bank;
                    }
                    if (nowlength * 6 < total) {
                        newnode(now, 4);
                        killflag = true;
                    }
                }
                if (solidcount <= 1) {
                    if (a1 == 1) {
                        newnode(now, 1);
                    } else if (a2 == 1) {
                        newnode(now, 2);
                    } else if (a3 == 1) {
                        newnode(now, 3);
                    } else if (a4 == 1) {
                        newnode(now, 4);
                    }
                } else { //只拓展最优的固化方案
                    MyPos targets[4];

                    char targets_actions[4];
                    char targets_count = 0;
                    if (a1 == 1) {
                        targets_actions[targets_count] = 1;
                        targets[targets_count++] = MyPos(nowsnake.front().x + 1, nowsnake.front().y);
                    }
                    if (a2 == 1) {
                        targets_actions[targets_count] = 2;
                        targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y + 1);
                    }
                    if (a3 == 1) {
                        targets_actions[targets_count] = 3;
                        targets[targets_count++] = MyPos(nowsnake.front().x - 1, nowsnake.front().y);
                    }
                    if (a4 == 1) {
                        targets_actions[targets_count] = 4;
                        targets[targets_count++] = MyPos(nowsnake.front().x, nowsnake.front().y - 1);
                    }
                    bool findflag = false;
                    for (char k = nowsnake.tail - 1; k >= nowsnake.head; k--) {
                        for (char i = 0; i < targets_count; i++) {
                            if (nowsnake.pos_list[k] == targets[i]) {
                                findflag = true;
                                newnode(now, targets_actions[i]);
                                break;
                            }
                        }
                        if (findflag) {
                            break;
                        }
                    }
                }
            } else {
                if (a1 == -1) {
                    newnode(now, 1);
                } else if (a2 == -1) {
                    newnode(now, 2);
                } else if (a3 == -1) {
                    newnode(now, 3);
                } else if (a4 == -1) {
                    newnode(now, 4);
                }
            }
            if (node[now].player_snakes[node[now].nowaction][node[now].nownumber].railgun_item &&
                node[now].player_snakes[node[now].nowaction][node[now].nownumber].length() >= 2) {
                newnode(now, 5);
            }
            if (node[now].snakecount[node[now].nowaction] < node[now].countbound[node[now].nowaction] &&
                node[now].player_snakes[node[now].nowaction][node[now].nownumber].length() > 2 &&
                node[now].round > 15) {
                newnode(now, 6);
            }
            if (now == local_root && node[now].lc == node[now].rc - 1) {                                //两子节点
                if (node[node[now].lc].useaction <= 4 && actions[node[node[now].lc].useaction] == -1) { //有自杀
                    check_preend = true;
                }
                if (node[node[now].rc].useaction <= 4 && actions[node[node[now].rc].useaction] == -1) {
                    check_preend = true;
                }
            }
        }
        randomplay(now, max(40, (node[local_root].countbound[0] + node[local_root].countbound[1]) * 10), true,
                   local_root);
        randomplay(now, max(40, (node[local_root].countbound[0] + node[local_root].countbound[1]) * 10), false,
                   local_root);
        if (now == local_root && node[now].lc == node[now].rc) { //单分支
            if (node[now].lc != -1) {
                node[node[now].lc].count = 1;
                node[node[now].lc].win = 0.5;
            }
            break;
        }
        if (check_preend && sim_count % 500 == 0 && sim_count >= 1000) {
            float val1 = node[node[local_root].lc].win / node[node[local_root].lc].count;
            float val2 = node[node[local_root].rc].win / node[node[local_root].rc].count;
            if (abs(val1 - val2) > 0.2) {
                break;
            }
        }
    }
    // std::cerr << "loc" << sim_count << " " << nodecount << std::endl;
}

Operation make_your_decision(const Snake &snake_to_operate, const Context &ctx, bool useadk) {
    start_t = clock();
    snakes_left = 0;
    if (useadk) {
        current_round = ctx._current_round;
        total_t = clock();
    } else if (firstaction) {
        current_round++;
        total_t = clock();
    }

    if (current_round == 1) {
        //获取道具列表
        memset(if_gen_item, 0, sizeof(if_gen_item));
        memset(future_item_rounds_count, 0, sizeof(future_item_rounds_count));
        for (short i = 0; i < ctx._item_list.size(); i++) { //加入当回合生成的道具
            gen_itemlist[ctx._item_list[i].time] =
                MyItem(ctx._item_list[i].x, ctx._item_list[i].y, ctx._item_list[i].type, ctx._item_list[i].time,
                       ctx._item_list[i].param);
            if_gen_item[ctx._item_list[i].time] = true;
        }
        for (short i = 1; i <= max_round; i++) {
            for (short j = i + 1; j <= max_round; j++) {
                if (if_gen_item[j]) {
                    future_item_rounds[i][future_item_rounds_count[i]++] = j;
                }
                if (future_item_rounds_count[i] >= 5) {
                    break;
                }
            }
        }

        //确定阵营
        mycamp = ctx._current_player;
        //随机种子
        srand(time(0));
        main_node.round = 0;
    }
    if (current_round <= 8) { //开局
        if (mycamp == 0) {
            if (current_round % 2 == 0) {
                return OP_RIGHT;
            } else {
                return OP_DOWN;
            }
        } else {
            if (current_round % 2 == 0) {
                return OP_LEFT;
            } else {
                return OP_UP;
            }
        }
    }
    if (useadk) {
        if (current_round == 9 && firstaction) {
            main_node = Node(ctx._snake_list_0, ctx._snake_list_1, ctx._wall_map, ctx._snake_map, ctx._item_list, 0,
                             current_round);
        }
        char snake_index;
        float total_length = sqrt(snake_to_operate.length() + snake_to_operate.length_bank);
        float now_length = sqrt(snake_to_operate.length() + snake_to_operate.length_bank);
        const std::vector<Snake> &my_snakes = ctx._current_player == 0 ? ctx._snake_list_0 : ctx._snake_list_1;
        for (char i = 0; i < my_snakes.size(); i++) {
            if (snake_to_operate.id == my_snakes[i].id) {
                snake_index = i;
                break;
            }
        }
        for (char i = snake_index + 1; i < my_snakes.size(); i++) {
            total_length += sqrt(my_snakes[i].length() + my_snakes[i].length_bank);
        }
        //时间分配
        used_time_ratio = (clock() - total_t) / (CLOCKS_PER_SEC * TIME);
        snakes_left = 0;
        float ratio = 1.1 * (1.0 - used_time_ratio) * (float)now_length / total_length;
        if (used_time_ratio >= 1.0f) {
            ratio = 0;
        } else if (ratio > 1.0 - used_time_ratio) {
            ratio = 1.0 - used_time_ratio;
        }
        time_ratio = ratio;
        if (firstaction) {
            node[0] = Node(ctx._snake_list_0, ctx._snake_list_1, ctx._wall_map, ctx._snake_map, ctx._item_list,
                           snake_index, current_round);
            node[0].clean();
            root = 0;
            nodecount = 1;
        } else {
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (node[i].useaction == last_action) {
                    root = i;
                    break;
                }
            }
        }
    } else {
        if (current_round == 9 && firstaction) {
            main_node = Node(ctx._snake_list_0, ctx._snake_list_1, ctx._wall_map, ctx._snake_map, ctx._item_list, 0,
                             current_round);
        }
        MySnake &current_snake = main_node.player_snakes[main_node.nowaction][main_node.nownumber];
        float total_length = sqrt(current_snake.length() + current_snake.length_bank);
        float now_length = sqrt(current_snake.length() + current_snake.length_bank);
        for (char i = main_node.nownumber + 1; i < main_node.snakecount[main_node.nowaction]; i++) {
            total_length += sqrt(main_node.player_snakes[main_node.nowaction][i].length() +
                                 main_node.player_snakes[main_node.nowaction][i].length_bank);
        }
        //时间分配
        used_time_ratio = (clock() - total_t) / (CLOCKS_PER_SEC * TIME);
        snakes_left = main_node.snakecount[main_node.nowaction] - main_node.nownumber - 1;
        float ratio = 1.1 * (1.0 - used_time_ratio) * (float)now_length / total_length;
        if (used_time_ratio >= 1.0) {
            ratio = 0;
        } else if (ratio > 1.0 - used_time_ratio - 0.1 * snakes_left) {
            ratio = 1.0 - used_time_ratio - 0.1 * snakes_left;
        }
        time_ratio = ratio;
        //初始化
        if (firstaction) {
            node[0] = main_node;
            node[0].clean();
            root = 0;
            nodecount = 1;
        } else {
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (node[i].useaction == last_action) {
                    root = i;
                    break;
                }
            }
        }
    }

    // MCTS
    adjust_ratio = min(0.6, max(0.4, 1 - time_ratio * (snakes_left + 1) / (2 * (1 - used_time_ratio))));
    MCTS();

    float maxval = -1;
    char maxval_action;
    char action;
    float action_values[7];
    memset(action_values, 0, sizeof(action_values));

    // 选择
    std::cerr << node[root].round << std::endl;
    if (node[root].lc != -1) {
        for (int i = node[root].lc; i <= node[root].rc; i++) {
            if (mycamp == 0) {
                action_values[node[i].useaction] += node[i].win / node[i].count + 0.0001f;
            } else {
                action_values[node[i].useaction] += 1 - node[i].win / node[i].count + 0.0001f;
            }
            if (dangerous_action(node[root], node[i].useaction)) {
                action_values[node[i].useaction] = max(0.0001f, action_values[node[i].useaction] - 0.1f);
            }
            char buf[10];
            sprintf(buf, "%.3f", action_values[node[i].useaction]);
            std::cerr << (short)(node[i].useaction) << " " << buf[2] << buf[3] << buf[4] << std::endl;
        }
    }

    for (char i = 1; i <= 6; i++) {
        if (action_values[i] > maxval) {
            maxval = action_values[i];
            maxval_action = i;
        }
    }

    //选择优化
    get_current_actions(node[root]);

    if ((maxval_action <= 4 && actions[maxval_action - 1] == 0) || maxval_action == 6) { //正常
        action = maxval_action;
    } else {
        float submaxval = -1;
        char submaxval_action;
        if (maxval_action <= 4 && actions[maxval_action - 1] == -1) { //自杀
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (node[i].useaction > 4 || actions[node[i].useaction - 1] != -1) {
                    float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                    if (nowval > submaxval) {
                        submaxval = nowval;
                        submaxval_action = node[i].useaction;
                    }
                }
            }
        } else if (maxval_action <= 4 && actions[maxval_action - 1] == 1) { //固化
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (node[i].useaction > 4 || actions[node[i].useaction - 1] == 0) {
                    float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                    if (nowval > submaxval) {
                        submaxval = nowval;
                        submaxval_action = node[i].useaction;
                    }
                }
            }
        } else { //射线
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (node[i].useaction == 6 || (node[i].useaction <= 4 && actions[node[i].useaction - 1] != -1)) {
                    float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                    if (nowval > submaxval) {
                        submaxval = nowval;
                        submaxval_action = node[i].useaction;
                    }
                }
            }
        }
        if (submaxval > maxval - SUBMAXVAL_DIFF) { //原操作之外的一个操作胜率并不低
            action = submaxval_action;
        } else {
            action = maxval_action;
        }
    }

    adjust_ratio = 1 - adjust_ratio;
    bool border = danger_border(node[root].player_snakes[node[root].nowaction][node[root].nownumber].front());
    if ((action <= 4 && actions[action - 1] == 0) || action > 4) { //正常行动
        LOCAL_MCTS();
        maxval = -1;
        if (node[local_root].lc != -1) {
            for (int i = node[local_root].lc; i <= node[local_root].rc; i++) {
                if (mycamp == 0) {
                    action_values[node[i].useaction] += node[i].win / node[i].count;
                } else {
                    action_values[node[i].useaction] += 1 - node[i].win / node[i].count;
                }
                if (dangerous_action(node[local_root], node[i].useaction)) {
                    action_values[node[i].useaction] = max(0.0001f, action_values[node[i].useaction] - 0.1f);
                }
            }
        }
        for (char i = 1; i <= 6; i++) {
            if (action_values[i] > maxval) {
                maxval = action_values[i];
                action = i;
            }
        }
    } else { // 固化或者自杀
        MCTS();
        maxval = -1;
        if (node[root].lc != -1) {
            for (int i = node[root].lc; i <= node[root].rc; i++) {
                if (mycamp == 0) {
                    action_values[node[i].useaction] = node[i].win / node[i].count;
                } else {
                    action_values[node[i].useaction] = 1 - node[i].win / node[i].count;
                }
                if (dangerous_action(node[root], node[i].useaction)) {
                    action_values[node[i].useaction] = max(0.0001f, action_values[node[i].useaction] - 0.1f);
                }
            }
        }

        for (char i = 1; i <= 6; i++) {
            if (action_values[i] > maxval) {
                maxval = action_values[i];
                maxval_action = i;
            }
        }

        //再次选择优化
        get_current_actions(node[root]);
        if ((maxval_action <= 4 && actions[maxval_action - 1] == 0) || maxval_action == 6) { //正常
            action = maxval_action;
        } else {
            float submaxval = -1;
            char submaxval_action;
            if (maxval_action <= 4 && actions[maxval_action - 1] == -1) { //自杀
                for (int i = node[root].lc; i <= node[root].rc; i++) {
                    if (node[i].useaction > 4 || actions[node[i].useaction - 1] != -1) {
                        float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                        if (nowval > submaxval) {
                            submaxval = nowval;
                            submaxval_action = node[i].useaction;
                        }
                    }
                }
            } else if (maxval_action <= 4 && actions[maxval_action - 1] == 1) { //固化
                for (int i = node[root].lc; i <= node[root].rc; i++) {
                    if (node[i].useaction > 4 || actions[node[i].useaction - 1] == 0) {
                        float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                        if (nowval > submaxval) {
                            submaxval = nowval;
                            submaxval_action = node[i].useaction;
                        }
                    }
                }
            } else { //射线
                for (int i = node[root].lc; i <= node[root].rc; i++) {
                    if (node[i].useaction == 6 || (node[i].useaction <= 4 && actions[node[i].useaction - 1] != -1)) {
                        float nowval = mycamp == 0 ? node[i].win / node[i].count : 1 - node[i].win / node[i].count;
                        if (nowval > submaxval) {
                            submaxval = nowval;
                            submaxval_action = node[i].useaction;
                        }
                    }
                }
            }
            if (submaxval > maxval - SUBMAXVAL_DIFF) { //原操作之外的一个操作胜率并不低
                action = submaxval_action;
            } else {
                action = maxval_action;
            }
        }
    }

    last_action = action;
    switch (action) {
    case 1:
        return OP_RIGHT;
    case 2:
        return OP_UP;
    case 3:
        return OP_LEFT;
    case 4:
        return OP_DOWN;
    case 5:
        return OP_RAILGUN;
    case 6:
        return OP_SPLIT;
    default:
        return OP_RIGHT;
    }
}

bool node_action(char type) {
    bool end = tonext(main_node, type);
    if (main_node.round > current_round) {
        firstaction = true;
    } else {
        firstaction = false;
    }
    return !end;
}

bool ok_to_abandon() { return main_node.round > 0; }

bool myaction() { return main_node.nowaction == mycamp; }

void game_over(int gameover_type, int winner, int p0_score, int p1_score) {
    fprintf(stderr, "%d %d %d %d", gameover_type, winner, p0_score, p1_score);
}

int main(int argc, char **argv) { SnakeGoAI start(argc, argv); }