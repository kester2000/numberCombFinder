#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include "comb.h"
//#include "createProccess.h"

using namespace std;

string seed;
int threadCount = 12;
int zeroCount = 2;
int SIZE = 20;
const char* outputFile = NULL;

vector<Card> cardList;

bool cmp(int i, int j) { return cardList[i] < cardList[j]; }

vector<Card> getCardsBySeed(string seedName)
{
    vector<Card> ret;
    vector<Card> card;
    const int x0[] = {3, 4, 8};
    const int x1[] = {1, 5, 9};
    const int x2[] = {2, 6, 7};
    for (int i0 : x0) {
        for (int i1 : x1) {
            for (int i2 : x2) {
                card.push_back(Card(i0, i1, i2));
                card.push_back(Card(i0, i1, i2));
            }
        }
    }
    card.push_back(Card(-1, -1, -1));
    card.push_back(Card(-1, -1, -1));
    seed_seq seed(seedName.begin(), seedName.end());
    mt19937 g(seed);
    shuffle(card.begin(), card.end(), g);
    int begin = 0;
    bool flag = true;
    for (int i = 0; i < 20; i++) {
        if (card[i].isWild()) {
            flag = false;
            break;
        }
    }
    if (flag) {
        begin += 20;
    }
    for (int i = begin; i < begin + 20; i++) {
        ret.push_back(card[i]);
    }
    return ret;
};

bool combineNext(int* choice, int n, int m)
{
    for (int i = m - 1, nxt = 19; i >= 0; i--, nxt--)
        if (choice[i] != nxt) {
            choice[i]++;
            for (int j = i + 1; j < m; j++) choice[j] = choice[j - 1] + 1;
            return true;
        }
    return false;
}

vector<int> reverseVec(vector<int> v)
{
    vector<int> r;
    r.resize(15);
    for (int i = 0; i < 15; i++) r[i] = v[i + 4 - 2 * (i % 5)];
    return r;
}

class MaxRecorder {
public:
    bool smallerThan(int val)
    {
        shared_lock<shared_mutex> _tmp(mMutex);
        return MX < val;
    }

    void set(int val, vector<int> vec)
    {
        lock_guard<shared_mutex> _tmp(mMutex);
        if (MX < val) {
            mSeed = seed;
            MX = val;
            best = vec;
            if (outputFile) {
                FILE* file = fopen(outputFile, "a");
                fprintf(file, "seed: %s %d [", seed.c_str(), MX);
                for (int i = 0; i < best.size(); i++) {
                    fprintf(file, "%d%c", best[i], ((i == best.size() - 1) ? ']' : ','));
                }
                fprintf(file, "\n");
                fclose(file);
            }
        }
    }

    void print()
    {
        shared_lock<shared_mutex> _tmp(mMutex);
        cout << "seed: " << mSeed << " ";
        cout << MX << " [";
        for (int i = 0; i < best.size(); i++) {
            cout << best[i] << ((i == best.size() - 1) ? ']' : ',');
        }
        cout << endl;
    }

    int getMax()
    {
        shared_lock<shared_mutex> _tmp(mMutex);
        return MX;
    }
    void preSetMX(int val) { MX = val; }

private:
    int MX = 0;
    vector<int> best;
    shared_mutex mMutex;
    string mSeed = "";
} maxRecorder;

int getScore(vector<int>& perm)
{
    Comb comb;
    for (int i = 0; i < SIZE; i++) {
        comb.doMove(cardList[i], perm[i]);
    }
    int r = comb.getScore();
    if (maxRecorder.smallerThan(r)) {
        maxRecorder.set(r, perm);
        maxRecorder.print();
    }
    return r;
}

int getPossibleScore(vector<int>& lines)
{
    int r = 0;
    for (int i = 0; i < 5; i++) {
        r += (lines[i] + lines[i + 5] + lines[i + 10]) * (5 - abs(i - 2));
    }
    r += 30;
    return r;
}

vector<int> findPieces(int x)
{
    vector<int> r;
    for (int i = 0; i < cardList.size(); i++) {
        int y = x;
        bool flag = true;
        if (!cardList[i].isWild()) {
            while (y) {
                if (cardList[i].getNum(0) != y % 10 && cardList[i].getNum(1) != y % 10 &&
                    cardList[i].getNum(2) != y % 10) {
                    flag = false;
                    break;
                }
                y /= 10;
            }
        }
        if (flag) {
            r.push_back(i);
        }
    }
    return r;
}

bool place(vector<int>& perm, vector<int> cardId, vector<int> places)
{
    sort(cardId.begin(), cardId.end(), cmp);
    int i, j;
    for (i = 0, j = 0; i < cardId.size() && j < places.size(); i++) {
        if (perm[cardId[i]] == -1) {
            perm[cardId[i]] = places[j];
            j++;
        }
    }
    return j >= places.size();
}

void placeRest(vector<int>& perm)
{
    int id = -1, mx = -1;
    bool flag[20];
    memset(flag, false, sizeof(flag));
    for (int i = 0; i < perm.size(); i++) {
        if (perm[i] == -1) {
            if (cardList[i].getScore() > mx) {
                id = i;
                mx = cardList[i].getScore();
            }
        } else {
            flag[perm[i]] = true;
        }
    }
    perm[id] = 0;
    flag[0] = true;
    int j = 0;
    for (int i = 0; i < perm.size(); i++) {
        if (perm[i] == -1) {
            for (; j < 20; j++) {
                if (!flag[j]) {
                    perm[i] = j;
                    flag[j] = true;
                }
            }
        }
    }
}

int realGetPerm(vector<int>& lines)
{
    if (getPossibleScore(lines) < maxRecorder.getMax()) {
        return 0;
    }

    map<int, vector<int> > mp;
    const int index[][3] = {{-1, -1, -1}, {2, 0, 0}, {3, 0, 1}, {4, 0, 2}, {1, 1, 0}, {2, 1, 1}, {3, 1, 2},
                            {4, 1, 3},    {0, 2, 0}, {1, 2, 1}, {2, 2, 2}, {3, 2, 3}, {4, 2, 4}, {0, 3, 1},
                            {1, 3, 2},    {2, 3, 3}, {3, 3, 4}, {0, 4, 2}, {1, 4, 3}, {2, 4, 4}};

    for (int i = 1; i <= 19; i++) {
        int key = 0;
        for (int j = 0; j < 3; j++) {
            if (lines[j * 5 + index[i][j]]) {
                key = key * 10 + lines[j * 5 + index[i][j]];
            }
        }
        if (mp.find(key) == mp.end()) {
            mp[key] = vector<int>();
        }
        mp[key].push_back(i);
    }

    vector<int> perm;
    perm.resize(20, -1);
    auto t = mp.end();
    do {
        t--;
        if (!place(perm, findPieces(t->first), t->second)) {
            return 2;
        }
    } while (t != mp.begin());

    placeRest(perm);
    getScore(perm);
    return 1;
}

void getPerm(vector<int>& lines, int dep)
{
    if (dep > zeroCount) {
        return;
    }
    int b = realGetPerm(lines);
    if (b == 2) {
        for (int i = 0; i < lines.size() && lines[i]; i++) {
            vector<int> cp = lines;
            cp[i] = 0;
            getPerm(cp, dep + 1);
        }
    }
}

vector<vector<int> > realGetLines(vector<vector<int> > lines, int* choice, int dep)
{
    if (dep >= 3) return lines;
    const int nums[3][3] = {{8, 4, 3}, {9, 5, 1}, {7, 6, 2}};
    const int index[3][5] = {{3, 4, 0, 1, 2}, {0, 1, 2, 3, 4}, {0, 3, 1, 4, 2}};
    for (int i = 0; i < 5; i++) {
        int num = cardList[choice[index[dep][i]]].getNum(dep);
        if (num == -1) {
            vector<vector<int> > new_strategys;
            for (vector<int>& strategy : lines) {
                for (int j = 0; j < 3; j++) {
                    vector<int> new_strategy = strategy;
                    new_strategy.push_back(nums[dep][j]);
                    new_strategys.push_back(new_strategy);
                }
            }
            lines = new_strategys;
        } else {
            for (vector<int>& strategy : lines) {
                strategy.push_back(num);
            }
        }
    }
    return realGetLines(lines, choice, dep + 1);
}

mutex idMutex;
vector<vector<int> > strategyVec;

static int strategyId = 0;
int getNextId()
{
    lock_guard<mutex> _tmp(idMutex);
    int ret;
    ret = (strategyId < strategyVec.size()) ? (strategyId++) : -1;
    return ret;
}

void runThread()
{
    int id = getNextId();
    while (id != -1) {
        getPerm(strategyVec[id], 0);
        id = getNextId();
    }
}

void getLines()
{
    int choice[] = {0, 1, 2, 3, 4};
    set<vector<int> > strategySet;
    strategySet.clear();
    do {
        vector<vector<int> > strategyTmp = realGetLines({{}}, choice, 0);
        for (auto strategy : strategyTmp) {
            if (strategySet.find(reverseVec(strategy)) == strategySet.end()) {
                strategySet.insert(strategy);
            }
        }
    } while (combineNext(choice, SIZE, 5));
    strategyVec = vector<vector<int> >(strategySet.begin(), strategySet.end());
    cout << strategyVec.size() << endl;
    vector<thread> threads;
    threads.resize(threadCount);
    for (int i = 0; i < threadCount; i++) {
        threads[i] = move(thread(runThread));
    }
    for (int i = 0; i < threadCount; i++) {
        threads[i].join();
    }
}

int sum(vector<Card> cards)
{
    int ret = 0;
    for (Card& c : cards) {
        for (int i = 0; i < 3; i++) {
            ret += (c.getNum(i) == -1 ? 10 : c.getNum(i));
        }
    }
    return ret;
}

int wild(vector<Card> cards)
{
    int ret = 0;
    for (Card& c : cards) {
        if (c.isWild()) {
            ret++;
        }
    }
    return ret;
}

void usage(int argc, char** argv)
{
    printf("Please use \"%s [options]\" to start this program.\n", argv[0]);
    printf(
        "The following three options should be set once and only once:\n"
        "\t-s <seed>    find the max score of this seed.\n"
        "\t-r           randomly set seed and find max score.\n"
        "\t-S <start>   find the max score starts at <start>\n"
        "The following option is optional\n"
        "\t-z <zero>    the number of lines allow get zero score.   default: 2\n"
        "\t-t <thread>  the number of threads.                      default: 12\n"
        "\t-p <pre>     only print the score bigger than pre.       default: 0\n"
        "\t-o <file>    print to the file if find a bigger score.\n");
    printf("\n");
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        usage(argc, argv);
        return 0;
    }
    char c;
    int mode = 0, startNum, preMax = 0;
    do {
        c = getopt(argc, argv, "s:rS:z:t:p:o:h");
        if (c == EOF) break;
        switch (c) {
            case 's':
                if (mode) {
                    usage(argc, argv);
                    return 0;
                }
                mode = 1;
                seed = optarg;
                break;
            case 'r':
                if (mode) {
                    usage(argc, argv);
                    return 0;
                }
                mode = 2;
                break;
            case 'S':
                if (mode) {
                    usage(argc, argv);
                    return 0;
                }
                mode = 3;
                startNum = atoi(optarg);
                break;
            case 'z':
                zeroCount = atoi(optarg);
                break;
            case 't':
                threadCount = atoi(optarg);
                break;
            case 'p':
                preMax = atoi(optarg);
                break;
            case 'o':
                outputFile = optarg;
                break;
            case 'h':
                usage(argc, argv);
                return 0;
        }
    } while (1);
    if (!mode) {
        usage(argc, argv);
        return 0;
    }
    maxRecorder.preSetMX(preMax);
    if (mode == 1) {
        cardList = getCardsBySeed(seed);
        cout << cardList.size() << endl;
        for (Card card : cardList) {
            cout << card.getNum(0) << ' ' << card.getNum(1) << ' ' << card.getNum(2) << endl;
        }
        strategyId = 0;
        getLines();
    } else if (mode == 2) {
        srand(time(0));
        while (1) {
            seed = to_string(rand());
            cout << "try seed: " << seed << " ";
            cardList = getCardsBySeed(seed);
            int tot = sum(cardList);
            int wildCnt = wild(cardList);
            cout << "wildCnt: " << wildCnt << " ";
            if (wildCnt == 2) {
                tot -= 6;
            }
            cout << "tot: " << tot << endl;
            if (maxRecorder.smallerThan(tot)) {
                strategyId = 0;
                getLines();
            }
            maxRecorder.print();
        }
    } else if (mode == 3) {
        while (1) {
            seed = to_string(startNum++);
            cout << "try seed: " << seed << " ";
            cardList = getCardsBySeed(seed);
            int tot = sum(cardList);
            int wildCnt = wild(cardList);
            cout << "wildCnt: " << wildCnt << " ";
            if (wildCnt == 2) {
                tot -= 6;
            }
            cout << "tot: " << tot << endl;
            if (maxRecorder.smallerThan(tot)) {
                strategyId = 0;
                getLines();
            }
            maxRecorder.print();
        }
    }
}