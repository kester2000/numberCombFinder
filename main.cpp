#include <time.h>
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
typedef long long ll;

// string seed;
int calThread = 12;
int zeroCount = 2;
int SIZE = 20;
const char* outputFile = NULL;
bool printTime = false;

vector<Card> getCardsBySeed(string seedName)
{
    vector<Card> ret;
    vector<Card> card;
    static const int x0[] = {3, 4, 8};
    static const int x1[] = {1, 5, 9};
    static const int x2[] = {2, 6, 7};
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

int getPossibleScore(vector<int>& lines)
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
    for (pair<int, int>& it : pairVec) {
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

bool isTrying(vector<int>& lines)
{
    for (int i = 5; i < 15; i++) {
        if (lines[i]) {
            return false;
        }
    }
    return true;
}

int realGetPerm(string& seed, vector<Card>& cardList, vector<int>& lines)
{
    if (!isTrying(lines) && getPossibleScore(lines) < maxRecorder.getMax()) {
        return 0;
    }

    map<int, vector<int> > mp;
    static const int index[][3] = {{-1, -1, -1}, {2, 0, 0}, {3, 0, 1}, {4, 0, 2}, {1, 1, 0}, {2, 1, 1}, {3, 1, 2},
                                   {4, 1, 3},    {0, 2, 0}, {1, 2, 1}, {2, 2, 2}, {3, 2, 3}, {4, 2, 4}, {0, 3, 1},
                                   {1, 3, 2},    {2, 3, 3}, {3, 3, 4}, {0, 4, 2}, {1, 4, 3}, {2, 4, 4}};

    for (int i = 1; i <= 19; i++) {
        int key = 0;
        for (int j = 0; j < 3; j++) {
            if (lines[j * 5 + index[i][j]]) {
                key = key * 10 + lines[j * 5 + index[i][j]];
            }
        }
        if (!mp.count(key)) {
            mp[key] = vector<int>();
        }
        mp[key].push_back(i);
    }

    vector<int> perm(20, -1);
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
    // if (lines.empty()) {
    //     return false;
    // }
    // if (dep > zeroCount) {
    //     return true;
    // }
    // int b = realGetPerm(seed, cardList, lines);
    // if (b == 2) {
    //     for (int i = 0; i < lines.size() && lines[i]; i++) {
    //         vector<int> cp = lines;
    //         cp[i] = 0;
    //         getPerm(seed, cardList, cp, dep + 1);
    //     }
    // }
    // return true;
    return realGetPerm(seed, cardList, lines) == 1;
}

class LinesVec {
public:
    LinesVec(int x) : mZeroCnt(x) {}
    vector<int> getNext()
    {
        static const int x[] = {8, 4, 3, 0};
        vector<int> ret;
        int cnt;
        do {
            ret.clear();
            cnt = 0;
            int id = getId();
            for (int i = 0; i < 5; i++) {
                ret.push_back(x[id % 4]);
                id /= 4;
                if (x[id % 4] == 0) cnt++;
            }
            if (id) return {};
        } while (cnt > zeroCount);
        return ret;
    }

private:
    mutex mMutex;
    int mZeroCnt, mId = 0;
    int getId()
    {
        unique_lock<mutex> _tmp(mMutex);
        return mId++;
    }
};

mutex pMtx;
void print(vector<int> v, string str)
{
    unique_lock<mutex> _tmp(pMtx);
    for (int i = 0; i < 5; i++) {
        cout << v[i] << ' ';
    }
    cout << str << endl;
}

void runThread(string& seed, vector<Card>& cardList, LinesVec& linesVec)
{
    static const int nums[][4] = {{9, 5, 1, 0}, {7, 6, 2, 0}};
    vector<int> lineFirst;
    while (1) {
        lineFirst = linesVec.getNext();
        if (lineFirst.empty()) break;

        vector<int> cp = lineFirst;
        for (int i = 0; i < 10; i++) cp.push_back(0);
        if (!getPerm(seed, cardList, cp, 0)) {
            // print(cp, "fail!");
            continue;
        }

        for (int k = 0;; k++) {
            cp = lineFirst;
            int l = k;
            for (int i = 0; i < 10; i++) {
                cp.push_back(nums[i / 5][l % 4]);
                l /= 4;
            }
            if (l) break;
            int cnt = 0;
            for (int& i : cp) {
                if (i == 0) {
                    cnt++;
                }
            }
            if (cnt > zeroCount) continue;
            getPerm(seed, cardList, cp, 0);
        }

        // print(cp, "finish!");
        continue;
    }
}

// void insertSetByLine(set<vector<int> >& linesSet, vector<Card>& cardList, vector<int>& choice, vector<int>& line,
//                      int dep)
// {
//     static const int nums[3][3] = {{8, 4, 3}, {9, 5, 1}, {7, 6, 2}};
//     static const int index[5][3] = {{3, 0, 0}, {4, 1, 3}, {0, 2, 1}, {1, 3, 4}, {2, 4, 2}};
//     if (dep >= 15) {
//         if (linesSet.find(reverseVec(line)) == linesSet.end()) {
//             linesSet.insert(line);
//         }
//         return;
//     }
//     int i = dep / 3, j = dep % 3;
//     int num = cardList[choice[i]].getNum(j);
//     if (num != -1) {
//         line[j * 5 + index[i][j]] = num;
//         insertSetByLine(linesSet, cardList, choice, line, dep + 1);
//     } else {
//         for (int l = 0; l < 3; l++) {
//             line[j * 5 + index[i][j]] = nums[j][l];
//             insertSetByLine(linesSet, cardList, choice, line, dep + 1);
//         }
//     }
// }

// void insertSet(set<vector<int> >& linesSet, vector<Card>& cardList, vector<int>& choice, int vis[20], int dep)
// {
//     if (dep >= 5) {
//         // cout << endl;
//         // for (auto i : choice) cout << i << ' ';
//         // cout << endl;
//         vector<int> line(15, -1);
//         insertSetByLine(linesSet, cardList, choice, line, 0);
//         return;
//     }
//     for (int i = 0; i < 20; i++) {
//         if (!vis[i]) {
//             vis[i] = 1;
//             choice.push_back(i);
//             insertSet(linesSet, cardList, choice, vis, dep + 1);
//             choice.pop_back();
//             vis[i] = 0;
//         }
//     }
// }

void getMaxScore(string& seed, vector<Card>& cardList)
{
    auto t0 = chrono::system_clock::now();
    LinesVec linesVec(zeroCount);
    vector<thread> threads;
    threads.resize(calThread);
    for (int i = 0; i < calThread; i++) {
        threads[i] = move(thread(runThread, ref(seed), ref(cardList), ref(linesVec)));
    }
    for (int i = 0; i < calThread; i++) {
        threads[i].join();
    }
    auto t1 = chrono::system_clock::now();
    if (printTime) {
        double duration = (t1 - t0).count() / (1e9);
        printf("duration %lf seconds\n", duration);
    }
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

int sum(vector<Card>& cards)
{
    int ret = 0;
    for (Card& c : cards) {
        for (int i = 0; i < 3; i++) {
            ret += (c.getNum(i) == -1 ? 10 : c.getNum(i));
        }
    }

    int wildCnt = wild(cards);
    if (wildCnt == 2) {
        ret -= 6;
    }
    return ret;
}

void randomFind()
{
    while (1) {
        string seed = to_string(rand());
        vector<Card> cardList = getCardsBySeed(seed);
        int tot = sum(cardList);
        if (maxRecorder.smallerThan(tot)) {
            cout << "try seed: " << seed << " tot: " << tot << endl;
            getMaxScore(seed, cardList);
        }
    }
}

int printNum;
mutex startNumMutex;
ll startNum = 0;
ll getNextNum()
{
    unique_lock<mutex> _tmp(startNumMutex);
    return startNum++;
}

void numberFind()
{
    while (1) {
        ll num = getNextNum();
        string seed = to_string(num);
        vector<Card> cardList = getCardsBySeed(seed);
        int tot = sum(cardList);
        if (maxRecorder.smallerThan(tot)) {
            time_t now_time = time(NULL);
            tm* t_tm = localtime(&now_time);
            cout << asctime(t_tm);
            cout << "try seed: " << seed << " tot: " << tot << endl;
            getMaxScore(seed, cardList);
        }
        // else if (num % printNum == 0) {
        //     cout << "try seed: " << seed << endl;
        // }
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
        "\t-t           print duration time.\n"
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
        c = getopt(argc, argv, "s:rS:p:z:f:c:o:m:th");
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
                startNum = strtoull(optarg, NULL, 0);
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
            case 't':
                printTime = true;
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
        int tot = sum(cardList);
        cout << "try seed: " << seed << " tot: " << tot << endl;
        getMaxScore(seed, cardList);
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