
//蜂巢的一个牌
class Card
{
private:
	//三个数字
	int nums[3];

public:
	Card(int x = 0, int y = 0, int z = 0)
	{
		nums[0] = x;
		nums[1] = y;
		nums[2] = z;
	}

	//判断为空
	bool isEmpty() const
	{
		return nums[0] == 0 && nums[1] == 0 && nums[2] == 0;
	}

	//判断为癞子
	bool isWild() const
	{
		return nums[0] == -1 && nums[1] == -1 && nums[2] == -1;
	}

	//返回单个数字
	int getNum(int id) const
	{
		return nums[id];
	}

	//返回单个得分
	int getScore() const
	{
		return (isWild() ? 30 : (nums[0] + nums[1] + nums[2]));
	}

	bool operator<(const Card& c2) const
	{
		return (getScore() < c2.getScore());
	}
};

const int m2l[][2] = { {-1,-1},{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{3,1},{0,2},{1,2},{2,2},{3,2},{4,2},{1,3},{2,3},{3,3},{4,3},{2,4},{3,4},{4,4} };

class Comb
{
private:
	//存牌的5*5数组
	Card card_matrix[5][5];
	Card bin;
public:
	Comb()
	{
		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				card_matrix[i][j] = Card(0, 0, 0);
			}
		}
		card_matrix[0][3] = Card(-1, -1, -1);
		card_matrix[0][4] = Card(-1, -1, -1);
		card_matrix[1][4] = Card(-1, -1, -1);
		card_matrix[3][0] = Card(-1, -1, -1);
		card_matrix[4][0] = Card(-1, -1, -1);
		card_matrix[4][1] = Card(-1, -1, -1);
	}

	void doMove(Card c, int move)
	{
		if (move == 0)
		{
			bin = c;
		}
		else
		{
			int x = m2l[move][0], y = m2l[move][1];
			card_matrix[x][y] = c;
		}
	}

	int getScore()
	{
		int score = 0;

		// 往右下
		const int rd[][2] = { {0,2},{0,1},{0,0},{1,0},{2,0} };
		for (int i = 0; i < 5; i++)
		{
			int x = rd[i][0], y = rd[i][1];
			int flag = -1;
			for (int j = 0; x + j < 5 && y + j < 5; j++)
			{
				if (card_matrix[x + j][y + j].getNum(0) != -1 && card_matrix[x + j][y + j].getNum(0) != flag)
				{
					if (flag == -1)
					{
						flag = card_matrix[x + j][y + j].getNum(0);
					}
					else
					{
						flag = 0;
						break;
					}
				}
			}
			score += flag * (5 - abs(i - 2));
		}

		//往下
		for (int j = 0; j < 5; j++)
		{
			int flag = -1;
			for (int i = 0; i < 5; i++)
			{
				if (card_matrix[i][j].getNum(1) != -1 && card_matrix[i][j].getNum(1) != flag)
				{
					if (flag == -1)
					{
						flag = card_matrix[i][j].getNum(1);
					}
					else
					{
						flag = 0;
						break;
					}
				}
			}
			score += flag * (5 - abs(j - 2));
		}

		//往右
		for (int i = 0; i < 5; i++)
		{
			int flag = -1;
			for (int j = 0; j < 5; j++)
			{
				if (card_matrix[i][j].getNum(2) != -1 && card_matrix[i][j].getNum(2) != flag)
				{
					if (flag == -1)
					{
						flag = card_matrix[i][j].getNum(2);
					}
					else
					{
						flag = 0;
						break;
					}
				}
			}
			score += flag * (5 - abs(i - 2));
		}

		//垃圾桶
		score += bin.getScore();

		return score;
	}
};
