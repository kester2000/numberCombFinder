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

// string seed;
int calThread = 12;
int zeroCount = 2;
int SIZE = 20;
const char* outputFile = NULL;

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

vector<int> reverseVec(vector<int>& v)
{
    vector<int> r;
    r.resize(15);
    for (int i = 0; i < 15; i++) r[i] = v[i + 4 - 2 * (i % 5)];
    return r;
}

class MaxRecorder {
public:
    void set(string seed, int val, vector<int> vec)
    {
        writing = true;
        unique_lock<mutex> _write(writeMutex);
        unique_lock<shared_mutex> _tmp(mMutex);
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
        writing = false;
    }

    bool smallerThan(int val)
    {
        if (writing) {
            unique_lock<mutex> _write(writeMutex);
        }
        shared_lock<shared_mutex> _tmp(mMutex);
        return MX < val;
    }

    void print()
    {
        if (writing) {
            unique_lock<mutex> _write(writeMutex);
        }
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
        if (writing) {
            unique_lock<mutex> _write(writeMutex);
        }
        shared_lock<shared_mutex> _tmp(mMutex);
        return MX;
    }
    void preSetMX(int val) { MX = val; }

private:
    string mSeed = "";
    int MX = 0;
    vector<int> best;
    bool writing = false;
    mutex writeMutex;
    shared_mutex mMutex;
} maxRecorder;

int getScore(string& seed, vector<Card>& cardList, vector<int>& perm)
{
    Comb comb;
    for (int i = 0; i < SIZE; i++) {
        comb.doMove(cardList[i], perm[i]);
    }
    int r = comb.getScore();
    if (maxRecorder.smallerThan(r)) {
        maxRecorder.set(seed, r, perm);
        maxRecorder.print();
    }
    return r;
}

int getPossibleScore(vector<Card>& cardList, vector<int>& lines)
{
    int r = 0;
    for (int i = 0; i < 5; i++) {
        r += (lines[i] + lines[i + 5] + lines[i + 10]) * (5 - abs(i - 2));
    }
    r += 30;
    return r;
}

vector<int> findPieces(vector<Card>& cardList, int x)
{
    vector<pair<int, int> > pairVec;
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
            pairVec.push_back({cardList[i].getScore(), i});
        }
    }
    sort(pairVec.begin(), pairVec.end());
    vector<int> r;
    for (auto it : pairVec) {
        r.push_back(it.second);
    }
    return r;
}

bool place(vector<Card>& cardList, vector<int>& perm, vector<int> cardId, vector<int> places)
{
    int i, j;
    for (i = 0, j = 0; i < cardId.size() && j < places.size(); i++) {
        if (perm[cardId[i]] == -1) {
            perm[cardId[i]] = places[j];
            j++;
        }
    }
    return j >= places.size();
}

void placeRest(vector<Card>& cardList, vector<int>& perm)
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

int realGetPerm(string& seed, vector<Card>& cardList, vector<int>& lines)
{
    if (getPossibleScore(cardList, lines) < maxRecorder.getMax()) {
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
        if (!place(cardList, perm, findPieces(cardList, t->first), t->second)) {
            return 2;
        }
    } while (t != mp.begin());

    placeRest(cardList, perm);
    getScore(seed, cardList, perm);
    return 1;
}

bool getPerm(string& seed, vector<Card>& cardList, vector<int> lines, int dep)
{
    if (lines.empty()) {
        return false;
    }
    if (dep > zeroCount) {
        return true;
    }
    int b = realGetPerm(seed, cardList, lines);
    if (b == 2) {
        for (int i = 0; i < lines.size() && lines[i]; i++) {
            vector<int> cp = lines;
            cp[i] = 0;
            getPerm(seed, cardList, cp, dep + 1);
        }
    }
    return true;
}

vector<vector<int> > realGetLines(vector<Card>& cardList, vector<vector<int> > lines, int* choice, int dep)
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
    return realGetLines(cardList, lines, choice, dep + 1);
}
class LinesVec {
public:
    vector<int> getNext()
    {
        lock_guard<mutex> _tmp(idMutex);
        if (mId < vec.size()) return vec[mId++];
        return {};
    }
    void set(set<vector<int> >& lineSet) { vec = vector<vector<int> >(lineSet.begin(), lineSet.end()); }

private:
    mutex idMutex;
    int mId = 0;
    vector<vector<int> > vec;
};

void runThread(string& seed, vector<Card>& cardList, LinesVec& linesVec)
{
    while (getPerm(seed, cardList, linesVec.getNext(), 0))
        ;
}

void getLines(string& seed, vector<Card>& cardList)
{
    int choice[] = {0, 1, 2, 3, 4};
    set<vector<int> > linesSet;
    linesSet.clear();
    do {
        vector<vector<int> > linesTmp = realGetLines(cardList, {{}}, choice, 0);
        for (auto strategy : linesTmp) {
            if (linesSet.find(reverseVec(strategy)) == linesSet.end()) {
                linesSet.insert(strategy);
            }
        }
    } while (combineNext(choice, SIZE, 5));
    LinesVec linesVec;
    linesVec.set(linesSet);
    vector<thread> threads;
    threads.resize(calThread);
    for (int i = 0; i < calThread; i++) {
        threads[i] = move(thread(runThread, ref(seed), ref(cardList), ref(linesVec)));
    }
    for (int i = 0; i < calThread; i++) {
        threads[i].join();
    }
}

int sum(vector<Card>& cards)
{
    int ret = 0;
    for (Card& c : cards) {
        for (int i = 0; i < 3; i++) {
            ret += (c.getNum(i) == -1 ? 10 : c.getNum(i));
        }
    }
    return ret;
}

int wild(vector<Card>& cards)
{
    int ret = 0;
    for (Card& c : cards) {
        if (c.isWild()) {
            ret++;
        }
    }
    return ret;
}

void randomFind()
{
    while (1) {
        string seed = to_string(rand());
        // cout << "try seed: " << seed << " ";
        vector<Card> cardList = getCardsBySeed(seed);
        int tot = sum(cardList);
        int wildCnt = wild(cardList);
        // cout << "wildCnt: " << wildCnt << " ";
        if (wildCnt == 2) {
            tot -= 6;
        }
        // cout << "tot: " << tot << endl;
        if (maxRecorder.smallerThan(tot)) {
            getLines(seed, cardList);
        }
        // maxRecorder.print();
    }
}

int printNum;
typedef long long ll;
mutex startNumMutex;
ll startNum = 0;
ll getNextNum()
{
    lock_guard<mutex> _tmp(startNumMutex);
    return startNum++;
}

void numberFind()
{
    while (1) {
        ll num = getNextNum();
        string seed = to_string(num);
        vector<Card> cardList = getCardsBySeed(seed);
        int tot = sum(cardList);
        int wildCnt = wild(cardList);
        if (wildCnt == 2) {
            tot -= 6;
        }
        if (num % printNum == 0) {
            cout << "try seed: " << seed << endl;
        }
        if (maxRecorder.smallerThan(tot)) {
            getLines(seed, cardList);
        }
        // maxRecorder.print();
    }
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
        "\t-p <pre>     only print the score bigger than pre.       default: 0\n"
        "\t-z <zero>    the number of lines allow get zero score.   default: 2\n"
        "\t-f <find>    the number of threads to find seed.\n"
        "\t-c <cal>     the number of threads to calculate the score.\n"
        "\t-o <file>    print to the file if find a bigger score.\n"
        "\t-m <number>  print when mod l equals 0.\n"
        "\t-h           return this text.\n");
    printf("\n");
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        usage(argc, argv);
        return 0;
    }
    char c;
    string seed = "";
    int mode = 0, preMax = 0, findThread = 0;
    printNum = 1;
    calThread = 0;
    do {
        c = getopt(argc, argv, "s:rS:p:z:t:o:m:h");
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
            case 'p':
                preMax = atoi(optarg);
                break;
            case 'z':
                zeroCount = atoi(optarg);
                break;
            case 'f':
                findThread = atoi(optarg);
                break;
            case 'c':
                calThread = atoi(optarg);
                break;
            case 'o':
                outputFile = optarg;
                break;
            case 'm':
                printNum = atoi(optarg);
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
        if (!calThread) {
            calThread = 12;
        }
        vector<Card> cardList = getCardsBySeed(seed);
        cout << cardList.size() << endl;
        for (Card card : cardList) {
            cout << card.getNum(0) << ' ' << card.getNum(1) << ' ' << card.getNum(2) << endl;
        }
        getLines(seed, cardList);
    } else {
        if (!calThread) {
            calThread = 1;
        }
        if (!findThread) {
            findThread = 12;
        }
        vector<thread> threads;
        threads.resize(findThread);

        if (mode == 2) {
            srand(time(0));
            for (int i = 0; i < findThread; i++) {
                threads[i] = move(thread(randomFind));
            }
            for (int i = 0; i < findThread; i++) {
                threads[i].join();
            }
        } else if (mode == 3) {
            for (int i = 0; i < findThread; i++) {
                threads[i] = move(thread(numberFind));
            }
            for (int i = 0; i < findThread; i++) {
                threads[i].join();
            }
        }
    }
}