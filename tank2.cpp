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
template<typename T> inline T operator- (T a, T b) { return (T)((int)a-(int)b); }

enum GameResult
{
    NotFinished = -2, Draw = -1, Blue = 0, Red = 1
};

enum FieldItem
{
    None = 0, Brick = 1, Steel = 2, Base = 4, Blue0 = 8, Blue1 = 16, Red0 = 32, Red1 = 64, Water = 128
};

enum Action
{
    Invalid = -2, Stay = -1, Up, Right, Down, Left, UpShoot, RightShoot, DownShoot, LeftShoot
};

const int fieldHeight = 9, fieldWidth = 9, sideCount = 2, tankPerSide = 2;

// 基地的横坐标
const int baseX[sideCount] = { fieldWidth / 2, fieldWidth / 2 };

// 基地的纵坐标
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

// 判断 item 是不是叠在一起的多个坦克
inline bool HasMultipleTank(FieldItem item)
{
    // 如果格子上只有一个物件，那么 item 的值是 2 的幂或 0
    // 对于数字 x，x & (x - 1) == 0 当且仅当 x 是 2 的幂或 0
    return !!(item & (item - 1));
}

inline int GetTankSide(FieldItem item)
{
    return item == Blue0 || item == Blue1 ? Blue : Red;
}

inline int GetTankID(FieldItem item)
{
    return item == Blue0 || item == Red0 ? 0 : 1;
}

// 获得动作的方向
inline int ExtractDirectionFromAction(Action x)
{
    if (x >= Up)
        return x % 4;
    return -1;
}

// 物件消失的记录，用于回退
struct DisappearLog
{
    FieldItem item;

    // 导致其消失的回合的编号
    int turn;

    int x, y;
    bool operator< (const DisappearLog& b) const
    {
        if (x == b.x)
        {
            if (y == b.y)
                return item < b.item;
            return y < b.y;
        }
        return x < b.x;
    }
};

// 游戏场地上的物件（一个格子上可能有多个坦克）
FieldItem gameField[fieldHeight][fieldWidth] = {};

// 坦克是否存活
bool tankAlive[sideCount][tankPerSide] = { { true, true },{ true, true } };

// 基地是否存活
bool baseAlive[sideCount] = { true, true };

// 坦克横坐标，-1表示坦克已炸
int tankX[sideCount][tankPerSide] = {
    { fieldWidth / 2 - 2, fieldWidth / 2 + 2 },{ fieldWidth / 2 + 2, fieldWidth / 2 - 2 }
};

// 坦克纵坐标，-1表示坦克已炸
int tankY[sideCount][tankPerSide] = { { 0, 0 },{ fieldHeight - 1, fieldHeight - 1 } };

// 当前回合编号
int currentTurn = 1;

// 我是哪一方
int mySide;

// 用于回退的log
stack<DisappearLog> logs;

// 过往动作（previousActions[x] 表示所有人在第 x 回合的动作，第 0 回合的动作没有意义）
Action previousActions[101][sideCount][tankPerSide] = { { { Stay, Stay },{ Stay, Stay } } };

//!//!//!// 以上变量设计为只读，不推荐进行修改 //!//!//!//

// 本回合双方即将执行的动作，需要手动填入
Action nextAction[sideCount][tankPerSide] = { { Invalid, Invalid },{ Invalid, Invalid } };

// 判断行为是否合法（出界或移动到非空格子算作非法）
// 未考虑坦克是否存活
bool ActionIsValid(int side, int tank, Action act)
{
    if (act == Invalid)
        return false;
    if (act > Left && previousActions[currentTurn - 1][side][tank] > Left) // 杩炵画涓ゅ洖鍚堝皠鍑?
        return false;
    if (act == Stay || act > Left)
        return true;
    int x = tankX[side][tank] + dx[act],
        y = tankY[side][tank] + dy[act];
    return CoordValid(x, y) && gameField[y][x] == None;// water cannot be stepped on
}

// 判断 nextAction 中的所有行为是否都合法
// 忽略掉未存活的坦克
bool ActionIsValid()
{
    for (int side = 0; side < sideCount; side++)
        for (int tank = 0; tank < tankPerSide; tank++)
            if (tankAlive[side][tank] && !ActionIsValid(side, tank, nextAction[side][tank]))
                return false;
    return true;
}

void _destroyTank(int side, int tank)
{
    tankAlive[side][tank] = false;
    tankX[side][tank] = tankY[side][tank] = -1;
}

void _revertTank(int side, int tank, DisappearLog& log)
{
    int &currX = tankX[side][tank], &currY = tankY[side][tank];
    if (tankAlive[side][tank])
        gameField[currY][currX] &= ~tankItemTypes[side][tank];
    else
        tankAlive[side][tank] = true;
    currX = log.x;
    currY = log.y;
    gameField[currY][currX] |= tankItemTypes[side][tank];
}

// 执行 nextAction 中指定的行为并进入下一回合，返回行为是否合法
bool DoAction()
{
    if (!ActionIsValid())
        return false;

    // 1 移动
    for (int side = 0; side < sideCount; side++)
        for (int tank = 0; tank < tankPerSide; tank++)
        {
            Action act = nextAction[side][tank];

            // 保存动作
            previousActions[currentTurn][side][tank] = act;
            if (tankAlive[side][tank] && ActionIsMove(act))
            {
                int &x = tankX[side][tank], &y = tankY[side][tank];
                FieldItem &items = gameField[y][x];

                // 记录 Log
                DisappearLog log;
                log.x = x;
                log.y = y;
                log.item = tankItemTypes[side][tank];
                log.turn = currentTurn;
                logs.push(log);

                // 变更坐标
                x += dx[act];
                y += dy[act];

                // 更换标记（注意格子可能有多个坦克）
                gameField[y][x] |= log.item;
                items &= ~log.item;
            }
        }

    // 2 射♂击!
    set<DisappearLog> itemsToBeDestroyed;
    for (int side = 0; side < sideCount; side++)
        for (int tank = 0; tank < tankPerSide; tank++)
        {
            Action act = nextAction[side][tank];
            if (tankAlive[side][tank] && ActionIsShoot(act))
            {
                int dir = ExtractDirectionFromAction(act);
                int x = tankX[side][tank], y = tankY[side][tank];
                bool hasMultipleTankWithMe = HasMultipleTank(gameField[y][x]);
                while (true)
                {
                    x += dx[dir];
                    y += dy[dir];
                    if (!CoordValid(x, y))
                        break;
                    FieldItem items = gameField[y][x];
                    //tank will not be on water, and water will not be shot, so it can be handled as None
                    if (items != None && items != Water)
                    {
                        // 对射判断
                        if (items >= Blue0 &&
                            !hasMultipleTankWithMe && !HasMultipleTank(items))
                        {
                            // 自己这里和射到的目标格子都只有一个坦克
                            Action theirAction = nextAction[GetTankSide(items)][GetTankID(items)];
                            if (ActionIsShoot(theirAction) &&
                                ActionDirectionIsOpposite(act, theirAction))
                            {
                                // 而且我方和对方的射击方向是反的
                                // 那么就忽视这次射击
                                break;
                            }
                        }

                        // 标记这些物件要被摧毁了（防止重复摧毁）
                        for (int mask = 1; mask <= Red1; mask <<= 1)
                            if (items & mask)
                            {
                                DisappearLog log;
                                log.x = x;
                                log.y = y;
                                log.item = (FieldItem)mask;
                                log.turn = currentTurn;
                                itemsToBeDestroyed.insert(log);
                            }
                        break;
                    }
                }
            }
        }

    for (auto& log : itemsToBeDestroyed)
    {
        switch (log.item)
        {
        case Base:
        {
            int side = log.x == baseX[Blue] && log.y == baseY[Blue] ? Blue : Red;
            baseAlive[side] = false;
            break;
        }
        case Blue0:
            _destroyTank(Blue, 0);
            break;
        case Blue1:
            _destroyTank(Blue, 1);
            break;
        case Red0:
            _destroyTank(Red, 0);
            break;
        case Red1:
            _destroyTank(Red, 1);
            break;
        case Steel:
            continue;
        default:
            ;
        }
        gameField[log.y][log.x] &= ~log.item;
        logs.push(log);
    }

    for (int side = 0; side < sideCount; side++)
        for (int tank = 0; tank < tankPerSide; tank++)
            nextAction[side][tank] = Invalid;

    currentTurn++;
    return true;
}

// 游戏是否结束？谁赢了？
GameResult GetGameResult()
{
    bool fail[sideCount] = {};
    for (int side = 0; side < sideCount; side++)
        if ((!tankAlive[side][0] && !tankAlive[side][1]) || !baseAlive[side])
            fail[side] = true;
    if (fail[0] == fail[1])
        return fail[0] || currentTurn > maxTurn ? Draw : NotFinished;
    if (fail[Blue])
        return Red;
    return Blue;
}

/* 三个 int 表示场地 01 矩阵（每个 int 用 27 位表示 3 行）
   initialize gameField[][]
   brick>water>steel
*/



void InitField(int hasBrick[3],int hasWater[3],int hasSteel[3], int _mySide)
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
                mask <<= 1;
            }
        }
    }
    for (int side = 0; side < sideCount; side++)
    {
        for (int tank = 0; tank < tankPerSide; tank++)
            gameField[tankY[side][tank]][tankX[side][tank]] = tankItemTypes[side][tank];
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

void _processRequestOrResponse(Json::Value& value, bool isOpponent)
{
    if (value.isArray())
    {
        if (!isOpponent)
        {
            for (int tank = 0; tank < tankPerSide; tank++)
                nextAction[mySide][tank] = (Action)value[tank].asInt();
        }
        else
        {
            for (int tank = 0; tank < tankPerSide; tank++)
                nextAction[1 - mySide][tank] = (Action)value[tank].asInt();
            DoAction();
        }
    }
    else
    {
        // 鏄涓€鍥炲悎锛岃鍒ゅ湪浠嬬粛鍦哄湴
        int hasBrick[3],hasWater[3],hasSteel[3];
        for (int i = 0; i < 3; i++){//Tank2 feature(???????????????)
            hasWater[i] = value["waterfield"][i].asInt();
            hasBrick[i] = value["brickfield"][i].asInt();
            hasSteel[i] = value["steelfield"][i].asInt();
        }
        InitField(hasBrick,hasWater,hasSteel,value["mySide"].asInt());
    }
}

void _submitAction(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
{
    Json::Value output(Json::objectValue), response(Json::arrayValue);
    response[0U] = tank0;
    response[1U] = tank1;
    output["response"] = response;
    if (!debug.empty())
        output["debug"] = debug;
    if (!data.empty())
        output["data"] = data;
    if (!globalData.empty())
        output["globalData"] = globalData;
    cout << writer.write(output) << endl;
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
  // 猜测是单行还是多行
    char lastChar = inputString[inputString.size() - 1];
    if (lastChar != '}' && lastChar != ']')
    {
        // 第一行不以}或]结尾，猜测是多行
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
                _processRequestOrResponse(requests[i], true);
                if (i < n - 1)
                    _processRequestOrResponse(responses[i], false);
            }
            outData = input["data"].asString();
            outGlobalData = input["globaldata"].asString();
            return;
        }
    }
    _processRequestOrResponse(input, true);
}

// 提交决策并退出，下回合时会重新运行程序
void SubmitAndExit(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
{
    _submitAction(tank0, tank1, debug, data, globalData);
    exit(0);
}

/* Our code start here */
const Action moveOrder[2][2][4] = 
{
    {{ Down, Left, Right, Up },
    { Down, Right, Left, Up }},
    {{ Up, Right, Left, Down },
    { Up, Left, Right, Down }}
};
inline Action prevAct(int t, int side, int tank)
{
    return previousActions[currentTurn - t][side][tank];
}//获取上一次动作
bool CanAvoid(int side, int tank, Action shoot)
{
    if (tankAlive[side][tank] && !ActionIsShoot(prevAct(1, side, tank)))
     return true; // 对方的坦克存活并且上一次不是射击
    int x = tankX[side][tank], y = tankY[side][tank];
    if (shoot == UpShoot || shoot == DownShoot) // has to move left or right
    {
        for (int xx = x - 1; xx <= x + 1; xx += 2)
            if (CoordValid(xx, y) && gameField[y][xx] == None) return true;
    }
    else // move up or down
    {
        for (int yy = y - 1; yy <= y + 1; yy += 2)
            if (CoordValid(x, yy) && gameField[yy][x] == None) return true;
    }
    return false;
}//检查对方是否能躲避我们的这次射击，如果不能就射！

bool NothingBetween(int x1, int y1, int x2, int y2)
{
    if (x1 == x2)
    {
        if (y1 > y2) swap(y1, y2);
        for (int yy = y1 + 1; yy <= y2 - 1; yy++)
           if (gameField[yy][x1] != None && gameField[yy][x1] != Water) return false;
    } 
    else // y1 == y2
    {
        if (x1 > x2) swap(x1, x2);
        for (int xx = x1 + 1; xx <= x2 - 1; xx++)
           if (gameField[y1][xx] != None && gameField[y1][xx] != Water) return false;
    }
    return true;
}

bool InBetween(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (x1 == x2)
    {
        if (x3 != x1) return false;
        return (y3 > min(y1, y2) && y3 < max(y1, y2));
    }
    else if (y1 == y2)
    {
        if (y3 != y1) return false;
        return (x3 > min(x1, x2) && x3 < max(x1, x2));
    }
    return false;
}//查看x3, y3是否在x1, y1与x2, y2之间

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
//是否从xy能射向目标xy，如果可以就返回某个shoot，不行就返回incalid



Action finalAct[2] = {Stay, Stay};
bool danger[fieldHeight][fieldWidth];
bool CanShoot[2][2];//四辆坦克能否射击

void initDangerZone(int side, int tank, int direction)
{
    int x = tankX[side][tank];//x坦克，谁那边，哪一辆
    int y = tankY[side][tank];
    while(1)
    {
        x += dx[direction];
        y += dy[direction];
        if(!CoordValid(x,y))//x,y在区域内
            break;
        FieldItem item = gameField[y][x];
        if (item != None && item != Water && item != tankItemTypes[mySide][0]
            && item != tankItemTypes[mySide][1])break;
            danger[y][x] = 1;
    }
}//danger:一个坦克可以射到的所有位置。
//传入参数:某方的某个坦克某个方向


int mahatonDis(int x, int y, int desX, int desY) {
	return abs(x - desX) + abs(desY - y);
}
class nods {
public:
	int cost_so_far, destroyed_bricks, x, y;
	nods(int c, int d, int x_, int y_) :cost_so_far(c), destroyed_bricks(d), x(x_), y(y_) {}
	bool operator <(const nods& other)const {
		if (cost_so_far == other.cost_so_far) {
			if (destroyed_bricks == other.destroyed_bricks)
				return mahatonDis(x, y, baseX[1 - mySide], baseY[1 - mySide]) < mahatonDis(other.x, other.y, baseX[1 - mySide], baseY[1 - mySide]);
			return destroyed_bricks < other.destroyed_bricks;
		}
		return cost_so_far < other.cost_so_far;
	}
};

inline int Min(int x, int y) {
	if (x < y)return x;
	return y;
}
inline int Max(int x, int y) {
	if (x > y)return x;
	return y;
}

int TankPlace[fieldHeight][fieldWidth] = {0};

multiset<nods> nods_to_open;
int Distance_From_Start[fieldHeight][fieldWidth] = {};
int Bricks_To_Destroy_From_Start[fieldHeight][fieldWidth] = {};


bool check_Des_Have_Tank(int desX, int desY) {
	for (int side = 0; side < 2; side++)
		for (int tank = 0; tank < 2; tank++)
			if (tankX[side][tank] == desX && tankY[side][tank] == desY)
				return true;
	return false;
}
Action findPath(int tank, int side, int desX, int desY) {
	nods_to_open.clear();
	for (int i = 0; i < fieldHeight; i++)
		for (int j = 0; j < fieldWidth; j++)
			Distance_From_Start[i][j] = Bricks_To_Destroy_From_Start[i][j] = 1000;
	int minCost = 1000, minBricks = 1000;
	Action my_Choice_This_Turn = Stay;
	Action FirstAction[fieldHeight][fieldWidth];
	int now_tankx = tankX[side][tank];
	int now_tanky = tankY[side][tank];
	bool memoryTankDanger[sideCount][tankPerSide];

	for (int side1 = 0; side1 < 2; side1++)
		for (int tank1 = 0; tank1 < 2; tank1++) {
			int x = tankX[side1][tank1];
			int y = tankY[side1][tank1];
			gameField[y][x] = None;//把有坦克的地方变成0
			TankPlace[y][x] = 1;
			if (!(x == now_tankx && y == now_tanky)) {//不是当前坦克
				memoryTankDanger[side1][tank1] = danger[y][x];
				danger[y][x] = true;
			}
		}
	Distance_From_Start[now_tanky][now_tankx] = 0;
	Bricks_To_Destroy_From_Start[now_tanky][now_tankx] = 0;
	nods_to_open.insert(nods(0, 0, now_tankx, now_tanky));
	while (!(nods_to_open.empty())) {
		nods now_checking = *nods_to_open.begin();
		nods_to_open.erase(nods_to_open.begin());
		if (now_checking.cost_so_far >= minCost)break;
		if (now_checking.destroyed_bricks >= minBricks && now_checking.cost_so_far == minCost)break;
		bool firstMove = !((bool)now_checking.cost_so_far);//true代表这是第一步


		if (firstMove&&CanDestroy(now_checking.x, now_checking.y, desX, desY) != Invalid) {
			if(!ActionIsShoot(prevAct(1,side,tank)))
				my_Choice_This_Turn = CanDestroy(now_checking.x, now_checking.y, desX, desY);//可以打
			break;
		}
		if (!firstMove&&CanDestroy(now_checking.x, now_checking.y, desX, desY) != Invalid) {
			if (1 + now_checking.cost_so_far < minCost || ((1 + now_checking.cost_so_far == minCost) && (now_checking.destroyed_bricks < minBricks))) {
				minCost = 1 + now_checking.cost_so_far;
				minBricks = now_checking.destroyed_bricks;
				my_Choice_This_Turn = FirstAction[now_checking.y][now_checking.x];
				continue;
			}
			else continue;
		}


		if (now_checking.x == desX || now_checking.y == desY) {
			int cntBricks = 0;
			int dir;
			if (now_checking.x == desX) {
				if (desY > now_checking.y)dir = 2;
				else dir = 0;
				int start = Min(now_checking.y, desY), end = Max(now_checking.y, desY);
				for (int i = start + 1; i < end; i++) {
					if (gameField[i][desX] == Brick)cntBricks++;
					if (gameField[i][desX] == Steel) { cntBricks = -1; break; }
				}
			}
			if (now_checking.y == desY) {
				if (now_checking.x > desX)dir = 3;//记录一下方向
				else dir = 1;
				int start = Min(now_checking.x, desX), end = Max(now_checking.x, desX);
				for (int i = start + 1; i < end; i++) {
					if (gameField[desY][i] == Brick)cntBricks++;
					if (gameField[desY][i] == Steel) { cntBricks = -1; break; }
				}
			}
			if (cntBricks != -1) {
				int stepsNeed = 2 * cntBricks + 1;
				if (!firstMove) { //找到了一条路径 就是沿着这个方向一直打
					if (stepsNeed + now_checking.cost_so_far < minCost || ((stepsNeed + now_checking.cost_so_far == minCost) && (now_checking.destroyed_bricks + cntBricks < minBricks))) {
						minCost = stepsNeed + now_checking.cost_so_far;
						minBricks = now_checking.destroyed_bricks + cntBricks;
						my_Choice_This_Turn = FirstAction[now_checking.y][now_checking.x];
						continue;
					}
					else continue;
				}
				else {//这是第一步
					if (ActionIsShoot(prevAct(1, side, tank))) {
						if (gameField[now_checking.y + dy[dir]][now_checking.x + dx[dir]] == None) {//可以前进
							my_Choice_This_Turn = (Action)dir;
							break;
						}
						else break;//别动
					}
					else {//我可以打 那就打
						my_Choice_This_Turn = (Action)(dir + 4);
						break;
					}
				}
			}
		}
		for (int dir = 0; dir < 4; dir++) {//枚举四个方向
			int cost = 1, brick = 0; bool needToShoot = false;
			Action nextAct = (Action)dir;
			int nextx = now_checking.x + dx[dir], nexty = now_checking.y + dy[dir];
			if (!CoordValid(nextx, nexty))continue;
			if (firstMove&&gameField[nexty][nextx] == Brick && ActionIsShoot(prevAct(1, side, tank)))continue;//我需要打掉这个砖块且上个回合我打过了
			if (baseX[mySide] == nextx && baseY[mySide] == nexty)continue;
			if (gameField[nexty][nextx] == Steel || gameField[nexty][nextx] == Water)continue;
			if (firstMove&&check_Des_Have_Tank(nextx, nexty))continue;//这是第一步且要走到的地方有坦克
			if (gameField[nexty][nextx] == Brick) { cost++; brick++; needToShoot = true; }

			if (now_checking.cost_so_far + cost < Distance_From_Start[nexty][nextx] || (now_checking.destroyed_bricks + brick < Bricks_To_Destroy_From_Start[nexty][nextx] && now_checking.cost_so_far + cost == Distance_From_Start[nexty][nextx])) {
				if (needToShoot)  nextAct = (Action)(nextAct + 4); 
				Distance_From_Start[nexty][nextx] = now_checking.cost_so_far + cost;
				Bricks_To_Destroy_From_Start[nexty][nextx] = now_checking.destroyed_bricks + brick;
				nods_to_open.insert(nods(now_checking.cost_so_far + cost, now_checking.destroyed_bricks + brick, nextx, nexty));
				if (firstMove)FirstAction[nexty][nextx] = nextAct;
				else FirstAction[nexty][nextx] = FirstAction[now_checking.y][now_checking.x];
			}
		}
	}
	for (int side1 = 0; side1 < 2; side1++)
		for (int tank1 = 0; tank1 < 2; tank1++) {
			int x = tankX[side1][tank1];
			int y = tankY[side1][tank1];
			if (!(x == now_tankx && y == now_tanky)) //不是当前坦克
				danger[y][x] = memoryTankDanger[side1][tank1];
		}
	return my_Choice_This_Turn;
}

void solve()
{
	memset(TankPlace, 0 ,sizeof(TankPlace));
    for (int side = 0; side < 2; side++)
            for (int tank = 0; tank < 2; tank++)
            CanShoot[side][tank] = (tankAlive[side][tank] && 
                !ActionIsShoot(prevAct(1, side, tank)));//存下来是否能进行射击
    memset(danger, 0, sizeof(danger));
    for(int tank = 0; tank < 2; tank++)
        if (CanShoot[1 - mySide][tank])//如果对方可以射击
        {
            bool hasMultipleTankWithMe = false;
            for(int tank2 = 0; tank2 < 2; tank2++)
                if (tankX[mySide][tank2] == tankX[1-mySide][tank] && tankY[mySide][tank2] == tankY[1-mySide][tank])
                     hasMultipleTankWithMe = true;
            if (hasMultipleTankWithMe && prevAct(1, mySide, tank) == Stay) continue;
            for(int direction = 0; direction < 4; direction++)
            {
                initDangerZone(1 - mySide, tank, direction);
            }//对面的坦克可射到的区域标为危险区域
        }//初始化过程完毕，以下开始进行动作决策

    for (int tank = 0; tank < 2; tank++)
        if (tankAlive[mySide][tank])//只要我的这辆坦克还活着
        {
            if (CanShoot[mySide][tank])//如果我的这辆坦克可以射击
            {
                auto act = CanDestroy (tankX[mySide][tank], tankY[mySide][tank],
                    baseX[1 - mySide], baseY[1 - mySide]);//是否能直接击杀对方基地
                if (act != Invalid)
                {
                    finalAct[tank] = act;
                    continue;//直接击杀！！
                }
                //如果不能直接摧毁对方基地
                for(int tank2 = 0; tank2 < 2; tank2++)
                  if (tankX[1 - mySide][tank2] >= 0)
                  {
                    act = CanDestroy(tankX[mySide][tank], tankY[mySide][tank],
                        tankX[1 - mySide][tank2], tankY[1 - mySide][tank2]);//如果我能射杀对方坦克
                    if (act != Invalid && !CanAvoid(1 - mySide, tank2, act))
                            {
                                finalAct[tank] = act;
                                break;
                            }
                    //需要判断我会不会把自己射死，保证我的另一辆坦克不在我和对方之间
                  }
            }

            if (finalAct[tank] != Stay) continue;//只要不是stay就执行这个操作
            Action act = findPath(tank, mySide, baseX[1 - mySide], baseY[1 - mySide]);//如果是stay,寻找路径
            if (ActionIsShoot(act))
            {
                int dir = ExtractDirectionFromAction(act);//当前执行动作的方向
                int xx = tankX[mySide][tank], yy = tankY[mySide][tank];
                bool flag = true;//下一局对方是否有可能摧毁我的坦克?
                int xB = xx + dx[dir];
                int yB = yy + dy[dir];//之所以加一个就行，是因为
                                      //如果射击，一定是摧毁离我最近的东西
                if (gameField[yB][xB] == Brick)//如果我的摧毁目标是砖块
                {
                    gameField[yB][xB] = None;//场地上清除砖块
                    for (int tank2 = 0; tank2 < 2; tank2++)
                        if(tankX[1 - mySide][tank2] >= 0)
                        {
                            int x1 = tankX[1 - mySide][tank2], y1 = tankY[1 - mySide][tank2];
                            if (CanDestroy(x1, y1, xx, yy)!= Invalid && InBetween(xx, yy, x1, y1, xB, yB))
                            {
                                flag  = false; 
                                gameField[yy + dy[dir]][xx + dx[dir]] = Brick;
                                break;
                            }//说明如果我打碎这个砖块，对方有可能击毙我
                           for (int dir2 = 0; dir2 < 4; dir2++)
                           {
                                int x2 = x1 + dx[dir2], y2 = y1 + dy[dir2];
                                if(CoordValid(x2, y2) && gameField[y2][x2] == None &&
                                CanDestroy(x2, y2, xx, yy) != Invalid && InBetween(xx, yy, x2, y2, xB, yB))
                                {
                                     flag = false; 
                                     gameField[yy + dy[dir]][xx + dx[dir]] = Brick;
                                     break;
                                }
                           }//如果对方移动后，有可能击毙我，也不破坏这块砖
                        }
                    gameField[yy + dy[dir]][xx + dx[dir]] = Brick;//复原，是砖块
                    Action initAct = act;//先存下来我的射击动作
                    if (!flag) act = Stay;
                if (act == Stay && prevAct(1, mySide, tank) == Stay && prevAct(2, mySide, tank) == Stay)
                {
                    for(int i = 0; i < 3; i++)
                    {
                        int dir = moveOrder[mySide][tank][i];
                        Action act2 = Action(dir+4);
                        if (act2 == initAct) continue;
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
        // if tank stay in current location, check if enemy tank can kill this tankand defend
        if (CanShoot[mySide][tank])
        {
            for(int tank2 = 0; tank2 < 2; tank2++)
            if (tankX[1-mySide][tank2] >= 0 && CanShoot[1-mySide][tank2])
            {
                Action enemyShoot = CanDestroy(tankX[1-mySide][tank2], tankY[1-mySide][tank2], tankX[mySide][tank], tankY[mySide][tank]);
                if (enemyShoot != Invalid)
                {
                    cerr << "Our tank shoot to defend\n";
                    // shoot opposite direction对射
                    if (enemyShoot > RightShoot) act = Action((int)enemyShoot-2);
                    else act = Action((int)enemyShoot+2);
                }
            }
        }
        finalAct[tank] = act;
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
   // cerr << tankY[mySide][0] << ' ' << tankX[mySide][0] << ' ' << tankY[mySide][1] << ' ' << tankX[mySide][1] << "\n";
    solve();
    SubmitAndExit(finalAct[0], finalAct[1]);
}
