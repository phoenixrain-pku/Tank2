#include <bits/stdc++.h>
#include "jsoncpp/json.h"

using namespace std;

template<typename T> inline T operator~ (T a) { return (T)~(int)a; }
template<typename T> inline T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<typename T> inline T operator& (T a, T b) { return (T)((int)a & (int)b); }
template<typename T> inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
template<typename T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<typename T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<typename T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

#define For(i,a,b) for(int i = a;i <= b; i++)
#define Rep(i,a,b) for(int i = a;i >= b; i--)
#define REP(i, n) for(int i = 0; i < n; i++)
#define FOR(i, f) for(auto i : f)
#define fi first
#define se second
#define pb push_back
#define eb emplace_back
#define mp make_pair
#define BUG(x) (cerr << #x << " = " << x << "\n")
#define sz(s) int(s.size())
#define reset(f, x) memset(f, x, sizeof(f))
#define all(x) x.begin(), x.end()
#define two(x) (1LL << x)
#define bit(x, i) ((x >> (i)) & 1LL)
#define onbit(x, i) (x | (1LL << (i)))
#define offbit(x, i) (x & ~(1 << (i)))

int tx, ty;

enum GameResult
{
    NotFinished = -2, Draw = -1, Blue = 0, Red = 1
};

enum FieldItem
{
    None = 0, Brick = 1, Steel = 2, Base = 4, Blue0 = 8, Blue1 = 16, Red0 = 32, Red1 = 64, Water = 128, Forest = 256
};

enum Action
{
    Invalid = -3, Unknown = -2, Stay = -1, Up, Right, Down, Left, UpShoot, RightShoot, DownShoot, LeftShoot
};

const int fieldHeight = 9, fieldWidth = 9, sideCount = 2, tankPerSide = 2;

// 鍩哄湴鐨勬í鍧愭爣
const int baseX[sideCount] = { fieldWidth / 2, fieldWidth / 2 };

// 鍩哄湴鐨勭旱鍧愭爣
const int baseY[sideCount] = { 0, fieldHeight - 1 };

const int N = 9;
const int INF = 1000;

const int dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };
const FieldItem tankItemTypes[sideCount][tankPerSide] = {
    { Blue0, Blue1 },{ Red0, Red1 }
};

int maxTurn = 100;

inline bool ActionIsMove(Action x)
{
    return x >= Up && x <= Left;
}

inline bool ActionIsShoot(Action x)
{
    return x >= UpShoot && x <= LeftShoot;
}

inline bool ActionDirectionIsOpposite(Action a, Action b)
{
    return a >= Up && b >= Up && (a + 2) % 4 == b % 4;
}

inline bool CoordValid(int x, int y)
{
    return x >= 0 && x < fieldWidth && y >= 0 && y < fieldHeight;
}

inline int count(FieldItem item)
{
    int cnt = 0;
    while(item)cnt += 1, item =(FieldItem)(((int)item) & (((int)item) - 1));
    return cnt;
}
// 鍒ゆ柇 item 鏄笉鏄彔鍦ㄤ竴璧风殑澶氫釜鍧﹀厠
inline bool HasMultipleTank(FieldItem item) // changed in tank2s
{
    int cnt = count(item);
    cnt -= !!(item & Forest);//闄ゅ幓forest
    return cnt == 2;
//      return !!(item & (item - 1));
}

inline int GetTankSide(FieldItem item)
{
    return item == Blue0 || item == Blue1 ? Blue : Red;
}

inline int GetTankID(FieldItem item)
{
    return item == Blue0 || item == Red0 ? 0 : 1;
}

// 鑾峰緱鍔ㄤ綔鐨勬柟鍚?
inline int ExtractDirectionFromAction(Action x)
{
    if (x >= Up)
        return x % 4;
    return -1;
}

// 娓告垙鍦哄湴涓婄殑鐗╀欢锛堜竴涓牸瀛愪笂鍙兘鏈夊涓潶鍏嬶級
FieldItem gameField[fieldHeight][fieldWidth] = {};

// 鍧﹀厠鏄惁瀛樻椿
bool tankAlive[sideCount][tankPerSide] = { { true, true },{ true, true } };

// 鍩哄湴鏄惁瀛樻椿
bool baseAlive[sideCount] = { true, true };

// 鍧﹀厠妯潗鏍囷紝-1琛ㄧず鍧﹀厠宸茬偢
int tankX[sideCount][tankPerSide] = {
    { fieldWidth / 2 - 2, fieldWidth / 2 + 2 },{ fieldWidth / 2 + 2, fieldWidth / 2 - 2 }
};

// 鍧﹀厠绾靛潗鏍囷紝-1琛ㄧず鍧﹀厠宸茬偢
int tankY[sideCount][tankPerSide] = { { 0, 0 },{ fieldHeight - 1, fieldHeight - 1 } };

// 褰撳墠鍥炲悎缂栧彿
int currentTurn = 1;

// 鎴戞槸鍝竴鏂?
int mySide;

// 杩囧線鍔ㄤ綔锛坧reviousActions[x] 琛ㄧず鎵€鏈変汉鍦ㄧ x 鍥炲悎鐨勫姩浣滐紝绗?0 鍥炲悎鐨勫姩浣滄病鏈夋剰涔夛級
Action previousActions[101][sideCount][tankPerSide] = { { { Stay, Stay },{ Stay, Stay } } };

/* 涓変釜 int 琛ㄧず鍦哄湴 01 鐭╅樀锛堟瘡涓?int 鐢?27 浣嶈〃绀?3 琛岋級
   initialize gameField[][]
   brick>water>steel
*/

void InitField(int hasBrick[3],int hasWater[3],int hasSteel[3],int hasForest[3], int _mySide)
{
    mySide = _mySide;
    for (int i = 0; i < 3; i++)
    {
        int mask = 1;
        for (int y = i * 3; y < (i + 1) * 3; y++)
        {
            for (int x = 0; x < fieldWidth; x++)
            {
                if (hasBrick[i] & mask)
                    gameField[y][x] = Brick;
                else if(hasWater[i] & mask)
                    gameField[y][x] = Water;
                else if(hasSteel[i] & mask)
                    gameField[y][x] = Steel;
                else if(hasForest[i] & mask)
                    gameField[y][x] = Forest;
                mask <<= 1;
            }
        }
    }
    for (int side = 0; side < sideCount; side++)
    {
       // for (int tank = 0; tank < tankPerSide; tank++)
       //     gameField[tankY[side][tank]][tankX[side][tank]] = tankItemTypes[side][tank];
        gameField[baseY[side]][baseX[side]] = Base;
    }
}
/* Internals */
Json::Reader reader;
#ifdef _BOTZONE_ONLINE
Json::FastWriter writer;
#else
Json::StyledWriter writer;
#endif

int enemyLastX[101][2], enemyLastY[101][2];

void _processRequestOrResponse(Json::Value& value, bool isOpponent, bool first)
{
    if (!first)
    {
        if (!isOpponent)
        {
            for (int tank = 0; tank < sideCount; tank++)
            if (tankAlive[mySide][tank])
            {
                Action act = (Action)value[tank].asInt();
                previousActions[currentTurn][mySide][tank] = act;
                if (ActionIsMove(act))
                {
                    tankX[mySide][tank] += dx[act];
                    tankY[mySide][tank] += dy[act];
                }
            }
        }
        else
        {
            for (int tank = 0; tank < tankPerSide; tank++)
            if (tankAlive[1-mySide][tank])
            {
                previousActions[currentTurn][1 - mySide][tank] = (Action)value["action"][tank].asInt();
                tankX[1-mySide][tank] = enemyLastX[currentTurn][tank] = value["final_enemy_positions"][tank*2].asInt();
                tankY[1-mySide][tank] = enemyLastY[currentTurn][tank] = value["final_enemy_positions"][tank*2+1].asInt();
            }
            int n = value["destroyed_blocks"].size();
            for (int i = 0; i < n; i += 2)
            {
                int x = value["destroyed_blocks"][i].asInt();
                int y = value["destroyed_blocks"][i+1].asInt();
                gameField[y][x] = None;
            }
            n = value["destroyed_tanks"].size();
            for (int i = 0; i < n; i += 2)
            {
                int x = value["destroyed_tanks"][i].asInt();
                int y = value["destroyed_tanks"][i+1].asInt();
                REP(side, 2) REP(tank, 2)
                if (tankX[side][tank] == x && tankY[side][tank] == y)
                    tankAlive[side][tank] = false;
            }
            currentTurn++;
        }
    }
    else
    {
        // 鏄涓€鍥炲悎锛岃鍒ゅ湪浠嬬粛鍦哄湴
        int hasBrick[3],hasWater[3],hasSteel[3],hasForest[3];
        for (int i = 0; i < 3; i++){//Tank2 feature(???????????????)
            hasWater[i] = value["waterfield"][i].asInt();
            hasBrick[i] = value["brickfield"][i].asInt();
            hasSteel[i] = value["steelfield"][i].asInt();
            hasForest[i] = value["forestfield"][i].asInt();
        }
        InitField(hasBrick,hasWater,hasSteel,hasForest,value["mySide"].asInt());
    }
}

void ReadInput(istream& in, string& outData, string& outGlobalData)
{
    Json::Value input;
    string inputString;
    do
    {
        getline(in, inputString);
    } while (inputString.empty());
#ifndef _BOTZONE_ONLINE
    // 鐚滄祴鏄崟琛岃繕鏄琛?
    char lastChar = inputString[inputString.size() - 1];
    if (lastChar != '}' && lastChar != ']')
    {
        // 绗竴琛屼笉浠鎴朷缁撳熬锛岀寽娴嬫槸澶氳
        string newString;
        do
        {
            getline(in, newString);
            inputString += newString;
        } while (newString != "}" && newString != "]");
    }
#endif
    reader.parse(inputString, input);

    if (input.isObject())
    {
        Json::Value requests = input["requests"], responses = input["responses"];
        if (!requests.isNull() && requests.isArray())
        {
            int i, n = requests.size();
            for (i = 0; i < n; i++)
            {
                _processRequestOrResponse(requests[i], true, !i);
                if (i < n - 1)
                    _processRequestOrResponse(responses[i], false, false);
            }
            outData = input["data"].asString();
            outGlobalData = input["globaldata"].asString();
            return;
        }
    }
    //Internals::_processRequestOrResponse(input, true);
}

// 璇蜂娇鐢?SubmitAndExit 鎴栬€?SubmitAndDontExit
void _submitAction(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
{
    Json::Value output(Json::objectValue), response(Json::arrayValue);
    response[0U] = tank0;
    response[1U] = tank1;
    Json::Value a = Json::Value(Json::arrayValue);
    a.append(tankX[mySide][0]);a.append(tankY[mySide][0]);
    a.append(tankX[mySide][1]);a.append(tankY[mySide][1]);
    output["debug"] = a;
    output["response"] = response;
    if (!debug.empty())
        output["debug"] = debug;
    if (!data.empty())
        output["data"] = data;
    if (!globalData.empty())
        output["globalData"] = globalData;
    cout << writer.write(output) << endl;
}

// 鎻愪氦鍐崇瓥骞堕€€鍑猴紝涓嬪洖鍚堟椂浼氶噸鏂拌繍琛岀▼搴?
void SubmitAndExit(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
{
    _submitAction(tank0, tank1, debug, data, globalData);
    exit(0);
}

/* Our code start here */

const Action moveOrder[2][2][4] = {
    {{ Down, Left, Right, Up },
    { Down, Right, Left, Up }},
    {{ Up, Right, Left, Down },
    { Up, Left, Right, Down }}
};

Action finalAct[2] = {Stay, Stay};
bool danger[fieldHeight][fieldWidth];
bool tankCanShoot[2][2];

// enemy tank is visible
inline bool tankVisible(int side, int tank)
{
    if (!tankAlive[side][tank]) return false;
    if (side == 1-mySide)
        return tankX[1-mySide][tank] >= 0;
    else return gameField[tankY[side][tank]][tankX[side][tank]] != Forest;
}

// check if nothing between (x1, y1) and (x2, y2)
bool NothingBetween(int x1, int y1, int x2, int y2)
{
    if (x1 == x2)
    {
        if (y1 > y2) swap(y1, y2);
        For(yy, y1+1, y2-1)
        if (gameField[yy][x1] != None && gameField[yy][x1] != Water && gameField[yy][x1] != Forest) return false;
    } else // y1 == y2
    {
        if (x1 > x2) swap(x1, x2);
        For(xx, x1+1, x2-1)
        if (gameField[y1][xx] != None && gameField[y1][xx] != Water && gameField[y1][xx] != Forest) return false;
    }
    return true;
}

// check if tank at (fromX, fromY) can destroy object at (desX, desY)
Action CanDestroy(int fromX, int fromY, int desX, int desY)
{
    if (fromX == desX && NothingBetween(fromX, fromY, desX, desY))
    {
        if (fromY < desY) return DownShoot;
        else return UpShoot;
    }
    if (fromY == desY && NothingBetween(fromX, fromY, desX, desY))
    {
        if (fromX < desX) return RightShoot;
        else return LeftShoot;
    }
    return Invalid;
}

// mark all the cell a tank (side, tank) can shoot with direction dir as dangerous zone
void InitDangerZone(int side, int tank, int dir)
{
    int x = tankX[side][tank], y = tankY[side][tank];
    while (true)
    {
        x += dx[dir]; y += dy[dir];
        if (!CoordValid(x, y)) break;
        FieldItem item = gameField[y][x];
        if (item != None && item != Water && item != Forest) break;
        danger[y][x] = true;
    }
}

// check if tank (side, tank) can avoid shoot
bool CanAvoid(int side, int tank, Action shoot)
{
    if (tankCanShoot[side][tank]) return true; // can shoot to defend
    int x = tankX[side][tank], y = tankY[side][tank];
    if (shoot == UpShoot || shoot == DownShoot) // has to move left or right
    {
        for (int xx = x-1; xx <= x+1; xx += 2)
            if (CoordValid(xx, y) && (gameField[y][xx] == None || gameField[y][xx] == Forest)) return true;
    } else // move up or down
    {
        for (int yy = y-1; yy <= y+1; yy += 2)
            if (CoordValid(x, yy) && (gameField[yy][x] == None || gameField[yy][x] == Forest)) return true;
    }
    return false;
}

// check (x3, y3) is between (x1, y1) and (x2, y2)
bool InBetween(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (x1 == x2)
    {
        if (x3 != x1) return false;
        return (y3 > min(y1, y2) && y3 < max(y1, y2));
    } else if (y1 == y2)
    {
        if (y3 != y1) return false;
        return (x3 > min(x1, x2) && x3 < max(x1, x2));
    }
    return false;
}

// return previous t turn action of a tank
inline Action prevAct(int t, int side, int tank)
{
    return previousActions[currentTurn-t][side][tank];
}

// return true if (x1, y1) < (x2, y2)
bool smaller(int x1, int y1, int x2, int y2)
{
    return tie(x1, y1) < tie(x2, y2);
}

struct state
{
    // f : total cost, g : number of forest
    int f, g, x, y, timer;
    state() { }
    state(int f, int g, int y, int x, int timer) : f(f), g(g), x(x), y(y), timer(timer) { }
    bool operator < (const state &other) const
    {
        if (f != other.f) return f < other.f;
        if (g != other.g) return g > other.g;
        return timer < other.timer;
    }
};

int dis[fieldHeight][fieldWidth];
int fr[fieldHeight][fieldWidth];
set<state> pq;
Action firstAct[fieldHeight][fieldWidth];

// find path for a tank to destroy object in (desX, desY) loc
// return path length and the first action

pair<int, Action> FindPath(int side, int tank, int desX, int desY, bool watchDanger)
{
    pq.clear();
    REP(i, N) REP(j, N)
    {
        dis[i][j] = fr[i][j] = INF;
        firstAct[i][j] = Invalid;
    }
    bool saveDanger[2][2];
    REP(side1, 2) REP(tank1, 2)
    {
        int x = tankX[side1][tank1], y = tankY[side1][tank1];
        if (tank != tank1 || side != side1)
        {
            saveDanger[side1][tank1] = danger[y][x];
            danger[y][x] = true;
        }
    }
    int x = tankX[side][tank], y = tankY[side][tank];
    dis[y][x] = 0;
    pq.insert(state(0, 0, y, x, 0));
    pq.insert(state(1, 0, y, x, 0));
    firstAct[y][x] = Stay;
    int timer = 0;
    int minPath = INF, maxForest = INF;
    Action shouldAct = Stay;
    while (!pq.empty())
    {
        int f = pq.begin()->f;
        int g = pq.begin()->g;
        x = pq.begin()->x; y = pq.begin()->y;
        pq.erase(pq.begin());
        if (f && CanDestroy(x, y, desX, desY) != Invalid) // not first move
        {
            if ((f+1 < minPath) || (f+1 == minPath && g > maxForest))
            {
                minPath = f+1; maxForest = g;
                shouldAct = firstAct[y][x];
            }
            continue;
        }
        // if tank in same row with enemy base then stay here and shoot

        if (x == desX || y == desY)
        if (!(!f && watchDanger && danger[y][x]))
        {
            int dir; // which direction should I shoot
            int cntBrick = 0; // count how many brick between tank and base
            if (x == desX)
            {
                if (y < desY) dir = (int)Down;
                else dir = (int)Up;
                int y1 = min(y, desY), y2 = max(y, desY);
                For(yy, y1+1, y2-1)
                if (gameField[yy][x] == Brick) cntBrick++;
                else if (gameField[yy][x] == Steel)
                {
                    cntBrick = -1; break;
                }
            }
            if (y == desY)
            {
                if (x < desX) dir = (int)Right;
                else dir = (int)Left;
                int x1 = min(x, desX), x2 = max(x, desX);
                For(xx, x1+1, x2-1)
                if (gameField[y][xx] == Brick) cntBrick++;
                else if (gameField[y][xx] == Steel)
                {
                    cntBrick = -1; break;
                }
            }
            if (cntBrick != -1) // if there is steel between tank and base
            {
	            int timeNeed = cntBrick*2+1;
	            bool needToStay = false;
	            // if this is first move and last turn is shoot then stay for one turn and begin shooting
	            if (!f && !tankCanShoot[side][tank])
	            {
	                timeNeed++;
	                needToStay = true;
	            }
	            if ((f+timeNeed < minPath) || (f+timeNeed == minPath && g > maxForest))
	            {
	                minPath = f+timeNeed; maxForest = g;
	                if (needToStay) shouldAct = Stay;
	                else if (!f) shouldAct = Action(dir+4); // shoot
	                else shouldAct = firstAct[y][x];
	            }
			}
        }
        // only consider move action, if moving in brick then shoot to destroy
        REP(i, 4)
        {
            int act = moveOrder[side][tank][i];
            int cost = 1, forest = 0; Action nextAct = (Action)act;
            int u = x + dx[act], v = y + dy[act];
            if (CoordValid(u, v) && (gameField[v][u] == None || gameField[v][u] == Brick || gameField[v][u] == Forest))
            {
                if (!f && watchDanger && danger[v][u]) continue;
                if (gameField[v][u] == Brick)
                {
                    if (!f && ActionIsShoot(previousActions[currentTurn-1][side][tank])) continue;
                    nextAct = Action(act+4);
                    cost = 2;
                }
                if (gameField[v][u] == Forest) forest = 1;
                // minimize total cost, if total cost equal then maximize number of forest
                if (dis[v][u] > f+cost || (dis[v][u] == f+cost && fr[v][u] < g+forest))
                {
                    timer++;
                    dis[v][u] = f+cost;
                    fr[v][u] = g+forest;
                    pq.insert(state(f+cost, g+forest, v, u, timer));
                    if (!f) firstAct[v][u] = nextAct;
                    else firstAct[v][u] = firstAct[y][x];
                }
            }
        }
    }
    REP(side1, 2) REP(tank1, 2)
    {
        int x = tankX[side1][tank1], y = tankY[side1][tank1];
        if (tank != tank1 || side != side1)
        {
            danger[y][x] = saveDanger[side1][tank1];
        }
    }
    // can't find a path
    return make_pair(minPath, shouldAct);
}

void Greedy()
{
    REP(side, 2) REP(tank, 2)
    {
        tankCanShoot[side][tank] = (tankAlive[side][tank] && !ActionIsShoot(prevAct(1, side, tank)));
    }

    memset(danger, false, sizeof danger);
    for (int tank = 0; tank < 2; tank++)
    if (tankCanShoot[1-mySide][tank] && tankVisible(1-mySide, tank))
    {
        // if enemy tank stay for 2 continuos turns then assume this turn also stay
        if (currentTurn >= 4 && prevAct(1, 1-mySide, tank) == Stay
            && prevAct(2, 1-mySide, tank)) continue;
        int x = tankX[1-mySide][tank], y = tankY[1-mySide][tank];
        // if enemy tank and our tank in a same cell then ignore enemy tank shoot
        bool hasMultipleTankWithMe = false;
        REP(tank2, 2)
            if (tankX[mySide][tank2] == tankX[1-mySide][tank] && tankY[mySide][tank2] == tankY[1-mySide][tank])
                hasMultipleTankWithMe = true;
        if (hasMultipleTankWithMe) continue;
        for (int dir = 0; dir < 4; dir++)
            InitDangerZone(1-mySide, tank, dir);
    }
    for (int tank = 0; tank < 2; tank++)
    if (tankAlive[mySide][tank])
    {
        if (tankCanShoot[mySide][tank])
        {
            // can shoot base
            auto act = CanDestroy(tankX[mySide][tank], tankY[mySide][tank], baseX[1-mySide], baseY[1-mySide]);
            if (act != Invalid)
            {
                finalAct[tank] = act;
                continue;
            }
            // can kill enemy's tank, in case enemy's tank can't avoid the shoot
            REP(tank2, 2)
            {
                act = CanDestroy(tankX[mySide][tank], tankY[mySide][tank], tankX[1-mySide][tank2], tankY[1-mySide][tank2]);
                if (act != Invalid && (!CanAvoid(1-mySide, tank2, act) || !tankVisible(mySide, tank)))
                {
                    finalAct[tank] = act;
                    break;
                }
            }
        }
        if (finalAct[tank] != Stay) continue;
        // find shortest path to destroy base
        auto path = FindPath(mySide, tank, baseX[1-mySide], baseY[1-mySide], true);
        int pathLength = path.first;
        auto act = path.second;
        // if between this tank and enemy's tank only have one brick then don't destroy
        if (ActionIsShoot(act) && tankVisible(mySide, tank))
        {
            int dir = ExtractDirectionFromAction(act);
            int xx = tankX[mySide][tank], yy = tankY[mySide][tank];
            bool flag = true;
            int xB = xx+dx[dir], yB = yy+dy[dir];
            if (gameField[yB][xB] == Brick)
            {
                gameField[yB][xB] = None;
                REP(tank2, 2)
                if (tankVisible(1-mySide, tank2))
                {
                    int x1 = tankX[1-mySide][tank2], y1 = tankY[1-mySide][tank2];
                    if (CanDestroy(x1, y1, xx, yy) != Invalid && InBetween(xx, yy, x1, y1, xB, yB))
                    {
                        flag = false; break;
                    }
                    REP(dir2, 4)
                    {
                        int x2 = x1+dx[dir2], y2 = y1+dy[dir2];
                        if (CoordValid(x2, y2) && (gameField[y2][x2] == None || gameField[y2][x2] == Forest)
                            && CanDestroy(x2, y2, xx, yy) != Invalid && InBetween(xx, yy, x2, y2, xB, yB))
                        {
                            flag = false; break;
                        }
                    }
                }
                gameField[yy+dy[dir]][xx+dx[dir]] = Brick;
                Action actBefore = act;
                if (!flag) act = Stay;
                if (act == Stay && prevAct(1, mySide, tank) == Stay && prevAct(2, mySide, tank) == Stay)
                {
                    REP(i, 3)
                    {
                        int dir = moveOrder[mySide][tank][i];
                        Action act2 = Action(dir+4);
                        if (act2 == actBefore) continue;
                        int x1 = xx+dx[dir], y1 = yy+dy[dir];;
                        if (!CoordValid(x1, y1)) continue;
                        if (gameField[y1][x1] == Brick)
                        {
                            act = act2;
                            break;
                        }
                    }
                }
            }
        }
        // if tank action is move then don't need to check
        // if tank stay in current location, check if enemy tank can kill this tank and defend
        if (tankCanShoot[mySide][tank] && tankVisible(mySide, tank) && (act == Stay || ActionIsShoot(act)))
        {
            REP(tank2, 2)
            if (tankVisible(1-mySide, tank2) && tankCanShoot[1-mySide][tank2])
            {
                Action enemyShoot = CanDestroy(tankX[1-mySide][tank2], tankY[1-mySide][tank2], tankX[mySide][tank], tankY[mySide][tank]);
                if (enemyShoot != Invalid)
                {
                    cerr << "Our tank shoot to defend\n";
                    // shoot opposite direction
                    if (enemyShoot > RightShoot) act = Action((int)enemyShoot-2);
                    else act = Action((int)enemyShoot+2);
                }
            }
        }
        finalAct[tank] = act;
        cerr << pathLength << " " << act << "\n";
    }
}

/* Our code end here */

int main()
{
    srand((unsigned)time(nullptr));
#ifndef _BOTZONE_ONLINE
    freopen("data.json", "r", stdin);
    //freopen("reponse.json", "w", stdout);
#endif
    string data, globaldata;
    ReadInput(cin, data, globaldata);
    Greedy();
    SubmitAndExit(finalAct[0], finalAct[1]);
}
