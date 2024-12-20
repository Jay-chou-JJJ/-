#include<stdio.h>
#include<graphics.h>
#include"tools.h"
#include<time.h>

#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")

#define width 900
#define hight 600//背景长宽
#define zmmaxacount 10//最多僵尸

enum { pea, sunflower, nut, plant_count };
enum {going,win,fail};
int kill_count;
int zm_count;
int gamestatus;

struct {
	int msg;//0表示没有选中，1第一种，2第二种，类推
	int x;
	int y;
}current_index;//选中植物移动的坐标

struct zhiwu {
	int type;//0无植物；1第一种
	int frameindex;//帧
	int blood;
	int timer;//被吃后的扣血间隔
	int x;
	int y;
	int peatimer;//发射豌豆的间隔
};
struct zhiwu map[3][9];//草坪

struct sunshine {
	int x, y;
	int frameindex;
	int aim_y;//停下的位置.垂直飘落
	bool used=false;
	int timer;//计时器，停止之后要等一段时间在消失
};//飘落的阳光
sunshine balls[10];

struct Zombies {
	int x, y;
	int frameindex;
	int row;
	bool used;
	int blood;
	int speed;//行走速度
	bool dead;
	int timer;
	bool ate;
};//僵尸
Zombies zms[10];

struct bullet {
	int x, y;
	bool used;
	int row;
	int speed;
	bool boom;//是否爆炸
	int frameindex;//爆炸动画
};//豌豆
struct bullet bullets[30];

IMAGE backgroud;//背景
IMAGE imgbar;//工具栏
IMAGE plant[plant_count];//植物卡片
IMAGE *plt[plant_count][20];//纯植物
IMAGE imgsunshine[29];//阳光
IMAGE imgZombies[21];//僵尸
IMAGE imgzmsdeath[20];//死掉的僵尸
IMAGE imgzmeat[21];
IMAGE imgzmstand[11];
IMAGE imgNormalBullet;
IMAGE imgboombullet[4];

int sunshine_amount=150;//初始阳光150
int bulletscount = sizeof(bullets) / sizeof(bullets[0]);//子弹池数
int zmmax = sizeof(zms) / sizeof(zms[0]);//僵尸池数
int ballmax = sizeof(balls) / sizeof(balls[0]);//阳光池数

bool file_exist(char* plt_name);

void gameinit();

void startUI();

void viewscence();

void updatewindow();

void collectsunshine(ExMessage* msg);

void click();

void createsunshine();

void sunflowersunshine();

void updatesunshine();

void createzombies();

void updatezombies();

void shoot();

void updatepea();

void checkbullet2zm();

void checkzm2plant();

void collisioncheck();

void update();

bool checkover();

int main() {
	int timer=0;//计时器，计算更新帧数的时间

	gameinit();

	mciSendString("play ../res/res/bg.mp3", 0, 0, 0);//背景音乐

	startUI();

	viewscence();

	while (1) {
		click();
		updatewindow();

		timer += getDelay();

		if(timer>70){
			timer = 0;
			update();
			if (checkover())break;
		}//70ms更新一次，防止鬼畜
	}
	system("pause");
}

bool file_exist(char* plt_name) {
	FILE* file = fopen(plt_name, "r");
	if (file == NULL) {
		return false;
	}
	else {
		fclose(file);
		return true;
	}
}//文件是否存在

void gameinit() {
	char plant_name[64], plt_name[64], sunshinename[64], zombies_name[64];

	memset(plt, 0, sizeof(plt));
	memset(map, 0, sizeof(map));
	memset(balls, 0, sizeof(balls));
	memset(zms, 0, sizeof(zms));
	memset(bullets, 0, sizeof(bullets));//数据的初始化
	srand(time(0));

	kill_count = 0;
	zm_count = 0;
	gamestatus = going;

	loadimage(&imgNormalBullet, "../res/res/bullets/PeaNormal/PeaNormal_0.png");
	loadimage(&imgboombullet[3], "../res/res/bullets/PeaNormalExplode/PeaNormalExplode_0.png");

	for (int i = 0;i < 3;i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgboombullet[i], "../res/res/bullets/PeaNormalExplode/PeaNormalExplode_0.png",
			imgboombullet[3].getwidth() * k,
			imgboombullet[3].getheight() * k, true);
	}//子弹的加载

	loadimage(&backgroud, "../res/res/bg.jpg");
	loadimage(&imgbar, "../res/res/bar5.png");//背景，工具栏加载

	for (int i = 0;i < plant_count;i++) {
		sprintf_s(plant_name, sizeof(plant_name), "../res/res/Cards/card_%d.png", i + 1);
		loadimage(&plant[i], plant_name);
		for (int j = 0;j < 20;j++) {
			sprintf_s(plt_name, sizeof(plt_name), "../res/res/zhiwu/%d/%d.png", i, j + 1);
			if (file_exist(plt_name)) {
				plt[i][j] = new IMAGE;
				loadimage(plt[i][j], plt_name);
			}
			else {
				break;
			}
		}
	}//植物的加载

	for (int i = 0;i < 21;i++) {
		sprintf_s(zombies_name, sizeof(zombies_name), "../res/res/zm/%d.png", i);
		loadimage(&imgZombies[i], zombies_name);
	}
	for (int i = 0;i < 20;i++) {
		sprintf_s(zombies_name, sizeof(zombies_name), "../res/res/zm_dead/%d.png", i + 1);
		loadimage(&imgzmsdeath[i], zombies_name);
	}
	
	for (int i = 0;i < 21;i++) {
		sprintf_s(zombies_name, sizeof(zombies_name), "../res/res/zm_eat/%d.png", i + 1);
		loadimage(&imgzmeat[i], zombies_name);
	}
	
	for (int i = 0;i < 11;i++) {
		sprintf_s(zombies_name, sizeof(zombies_name), "../res/res/zm_stand/%d.png", i + 1);
		loadimage(&imgzmstand[i], zombies_name);
	}
	//僵尸加载

	for (int i = 0;i < 29;i++) {
		sprintf_s(sunshinename, sizeof(sunshinename), "../res/res/sunshine/%d.png", i + 1);
		loadimage(&imgsunshine[i], sunshinename);
	}//阳光加载

	initgraph(width, hight);//加载显示窗口
}

void startUI() {
	IMAGE UI, menu1, menu2;
	bool flag = false;
	ExMessage cur_msg;

	loadimage(&UI, "../res/res/Screen/mainmenu.png");
	loadimage(&menu1, "../res/res/menu2.png");
	loadimage(&menu2, "../res/res/menu1.png");

	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &UI);

		if (peekmessage(&cur_msg)) {
			if (cur_msg.message == WM_LBUTTONDOWN) {
				if (cur_msg.x > 474 && cur_msg.x < 474 + 300 && cur_msg.y < 75 + 140 && cur_msg.y>75) {
					flag = true;
				}
			}
			else if (cur_msg.message == WM_LBUTTONUP) {
				if (cur_msg.x > 474 && cur_msg.x < 474 + 300 && cur_msg.y < 75 + 140 && cur_msg.y>75) {
					EndBatchDraw();
					break;
				}
			}
		}

		if (!flag) {
			putimagePNG(474, 75, &menu1);
		}
		else {
			putimagePNG(474, 75, &menu2);
		}
		EndBatchDraw();
	}
}

void viewscence() {
	int xmin = width - backgroud.getwidth();
	int index[9];
	static int timer=0;
	
	for (int i = 0;i < 9;i++) {
		index[i] = rand() % 11;
	}

	for (int x = 0;x >= xmin;x--) {
		timer++;

		BeginBatchDraw();
		putimage(x, 0, &backgroud);

			putimagePNG(550 - xmin + x, 80, &imgzmstand[index[0]]);
			putimagePNG(530 - xmin + x, 160, &imgzmstand[index[1]]);
			putimagePNG(630 - xmin + x, 170, &imgzmstand[index[2]]);
			putimagePNG(530 - xmin + x, 200, &imgzmstand[index[3]]);
			putimagePNG(515 - xmin + x, 270, &imgzmstand[index[4]]);
			putimagePNG(565 - xmin + x, 370, &imgzmstand[index[5]]);
			putimagePNG(605 - xmin + x, 340, &imgzmstand[index[6]]);
			putimagePNG(705 - xmin + x, 280, & imgzmstand[index[7]]);
			putimagePNG(690 - xmin + x, 340, &imgzmstand[index[8]]);

			if (timer > 3) {
				timer = 0;
				for (int i = 0;i < 9;i++) {
					index[i] = (index[i] + 1) % 11;
				}
			}
			
		EndBatchDraw();
		Sleep(5);
	}

	for (int i = 0;i < 100;i++) {
		timer++;
		BeginBatchDraw();

		putimage(xmin, 0, &backgroud);

		putimagePNG(550, 80, &imgzmstand[index[0]]);
		putimagePNG(530, 160, &imgzmstand[index[1]]);
		putimagePNG(630, 170, &imgzmstand[index[2]]);
		putimagePNG(530, 200, &imgzmstand[index[3]]);
		putimagePNG(515, 270, &imgzmstand[index[4]]);
		putimagePNG(565, 370, &imgzmstand[index[5]]);
		putimagePNG(605, 340, &imgzmstand[index[6]]);
		putimagePNG(705, 280, &imgzmstand[index[7]]);
		putimagePNG(690, 340, &imgzmstand[index[8]]);

		if (timer > 1) {
			timer = 0;
			for (int i = 0;i < 9;i++) {
				index[i] = (index[i] + 1) % 11;
			}
		}

		EndBatchDraw();
		Sleep(20);
	}

	for (int x = xmin;x <= 0;x++) {
		timer++;

		BeginBatchDraw();
		putimage(x, 0, &backgroud);

		putimagePNG(550 - xmin + x, 80, &imgzmstand[index[0]]);
		putimagePNG(530 - xmin + x, 160, &imgzmstand[index[1]]);
		putimagePNG(630 - xmin + x, 170, &imgzmstand[index[2]]);
		putimagePNG(530 - xmin + x, 200, &imgzmstand[index[3]]);
		putimagePNG(515 - xmin + x, 270, &imgzmstand[index[4]]);
		putimagePNG(565 - xmin + x, 370, &imgzmstand[index[5]]);
		putimagePNG(605 - xmin + x, 340, &imgzmstand[index[6]]);
		putimagePNG(705 - xmin + x, 280, &imgzmstand[index[7]]);
		putimagePNG(690 - xmin + x, 340, &imgzmstand[index[8]]);

		if (timer > 3) {
			timer = 0;
			for (int i = 0;i < 9;i++) {
				index[i] = (index[i] + 1) % 11;
			}
		}
	
		EndBatchDraw();
		Sleep(5);
	}
}

void updatewindow() {
	BeginBatchDraw();
	putimage(0, 0, &backgroud);
	putimagePNG(250, 0, &imgbar);//渲染背景，状态栏

	int x;
	int y = 6;

	for (int i = 0;i < plant_count;i++) {
		x = 338 + i * 65;
		putimagePNG(x, y, &plant[i]);
	}//渲染植物卡片

	for (int i = 0;i < 3;i++) {
		for (int j = 0;j < 9;j++) {
			if (map[i][j].type > 0) {
				int cur_x = 256 + j * 81;
				int cur_y = 179 + i * 102 + 14;
				putimagePNG(cur_x, cur_y, plt[map[i][j].type - 1][map[i][j].frameindex]);
			}
		}
	}//摇晃的植物

	if (current_index.msg > 0) {
		IMAGE* img = plt[current_index.msg - 1][0];
		putimagePNG(current_index.x - img->getwidth() / 2, current_index.y - img->getheight() / 2, img);
	}//拖动时

	char s_amount[16];
	sprintf_s(s_amount, sizeof(s_amount), "%d", sunshine_amount);
	outtextxy(283, 76, s_amount);//阳光值

	for (int i = 0;i < zmmax;i++) {
		if (zms[i].used) {
			if (zms[i].dead) {
				IMAGE* img = &imgzmsdeath[zms[i].frameindex];
				putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
			}
			else if(!zms[i].ate && !zms[i].dead) {
				IMAGE* img = &imgZombies[zms[i].frameindex];
				putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
			}
			else if (zms[i].ate && !zms[i].dead) {
				IMAGE* img = &imgzmeat[zms[i].frameindex];
				putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
			}
		}
	}//渲染僵尸

	for (int i = 0;i < ballmax;i++) {
		if (balls[i].used) {
			putimagePNG(balls[i].x, balls[i].y, &imgsunshine[balls[i].frameindex]);
		}
	}//渲染阳光

	for (int i = 0;i < bulletscount;i++) {
		if (bullets[i].used) {
			if (bullets[i].boom) {
				putimagePNG(bullets[i].x, bullets[i].y, &imgboombullet[bullets[i].frameindex]);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgNormalBullet);
			}
		}

	}//豌豆

	EndBatchDraw();
}

void collectsunshine(ExMessage* msg) {
	int ballmax = sizeof(balls) / sizeof(balls[0]);
	int w = imgsunshine[0].getwidth();
	int h = imgsunshine[0].getheight();
	for (int i = 0;i < ballmax;i++) {
		if (balls[i].used) {
			if (balls[i].x<msg->x && balls[i].x + w>msg->x) {
				if (balls[i].y<msg->y && balls[i].y + h>msg->y) {
					balls[i].used = false;
					sunshine_amount += 50;
					mciSendString("play ../res/res/sunshine.mp3", 0, 0, 0);//加载收集阳光的音乐
				}
			}
		}
	}
}

void click() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 338 && msg.x < 338 + 65 * plant_count && msg.y < 96 && msg.y>6) {
				int index = (msg.x - 338) / 65;
				if (index == 0 && sunshine_amount >= 100) {
					status = 1;
					current_index.x = msg.x;
					current_index.y = msg.y;
					current_index.msg = index + 1;
				}
				if ((index == 1||index==2) && sunshine_amount >= 50) {
					status = 1;
					current_index.x = msg.x;
					current_index.y = msg.y;
					current_index.msg = index + 1;
				}
			}
			else {
				collectsunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			current_index.x = msg.x;
			current_index.y = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP) {
			if (msg.x > 256 && msg.y > 179 && msg.y < 489) {
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 256) / 81;

				if (map[row][col].type == 0) {
					map[row][col].type = current_index.msg;
					map[row][col].frameindex = 0;
					map[row][col].blood = 100;
					map[row][col].timer = 0;
					map[row][col].x = msg.x;
					map[row][col].y = msg.y;

					if (current_index.msg == 1) {
						sunshine_amount -= 100;
					}
					if (current_index.msg == 2 || current_index.msg == 3) {
						sunshine_amount -= 50;
					}
				}
			}
			status = 0;
			current_index.msg = 0;
		}
	}
}

void createsunshine() {
	static int count = 0;
	static int fre = 100;
	count++;
	if (count >= fre) {
		fre = 50 + rand() % 100;//随机生成阳光
		count = 0;

		int i = 0;
		for (i = 0;i < ballmax && balls[i].used;i++);//没被使用
		if (i >= ballmax)return;
		balls[i].used = true;
		balls[i].frameindex = 0;
		balls[i].x = 260 + rand() % (850 - 260);
		balls[i].y = 0;
		balls[i].aim_y = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;
	}
}

void sunflowersunshine() {
	static int count = 0;
	static int fre = 100;
	for (int k = 0;k < 3;k++) {
		for (int j = 0;j < 9;j++) {
			if (map[k][j].type == 2) {
				count++;
				if (count >= fre) {
					fre = 50 + rand() % 100;//随机生成阳光
					count = 0;

					int i = 0;
					for (i = 0;i < ballmax && balls[i].used;i++);//没被使用
					if (i >= ballmax)return;
					balls[i].used = true;
					balls[i].frameindex = 0;
					balls[i].x = map[k][j].x;
					balls[i].aim_y = map[k][j].y;
					balls[i].y = map[k][j].y;
					balls[i].timer = 0;
				}
			}
		}
	}
}

void updatesunshine() {
	int i = 0;
	for (i = 0;i < ballmax;i++) {
		if (balls[i].used) {
			balls[i].frameindex = (balls[i].frameindex + 1) % 29;//防止越界
			if (balls[i].y <= balls[i].aim_y) {
				balls[i].y += 3;//更新位置

			}
			else {
				balls[i].timer++;
				if (balls[i].timer > 40) {
					balls[i].used = false;
				}
			}
		}
	}
}

void createzombies() {
	if (zm_count >= zmmaxacount)return;

	static int zmcount = 0;
	zmcount++;
	static int zmfre = 100;
	int i;

	if (zmcount > zmfre) {
		zmcount = 0;
		zmfre = rand() % 20 + 100;
		for (i = 0;i < zmmax && zms[i].used;i++);
		if (i < zmmax) {
			zms[i].used = true;
			zms[i].x = width;
			zms[i].row = rand() % 3;
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].frameindex = 0;
			zms[i].blood = 100;
			zms[i].dead = false;
			zms[i].speed = 2;//僵尸速度不同
			zms[i].timer = 0;
			zms[i].ate = false;
			zm_count++;
		}
	}
}

void updatezombies() {
	static int count = 0;
	count++;
	if (count > 1) {
		count = 0;
		for (int i = 0;i < zmmax;i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;//更新位置
				if (zms[i].x < 170) {
					gamestatus = fail;
				}
			}
		}
	}

	for (int i = 0;i < zmmax;i++) {
		if (zms[i].used && !zms[i].dead&&!zms[i].ate) {
			zms[i].timer++;
				if (zms[i].timer > 1) {
					zms[i].timer = 0;
					zms[i].frameindex++;
					if (zms[i].frameindex > 20) {
						zms[i].frameindex = 0;
					}
				}
			}
			else if (zms[i].used && zms[i].dead) {
				zms[i].frameindex++;
				if (zms[i].frameindex >= 20) {
					zms[i].used = false;
					kill_count++;
					if (kill_count == zmmaxacount) {
						gamestatus = win;
					}
				}
			}
			else if (zms[i].used && !zms[i].dead && zms[i].ate) {
			    zms[i].timer++;
				if (zms[i].timer > 1) {
					zms[i].timer = 0;
					zms[i].frameindex = (zms[i].frameindex + 1) % 21;
				}
		}
		}//分开更新帧数和位置
	}

void shoot() {
	int lines[3] = { 0,0,0 };
	int dangerx = width - imgZombies[0].getwidth() + 50;
	for (int i = 0;i < zmmax;i++) {
		if (zms[i].used && zms[i].x < dangerx) {
			lines[zms[i].row] = 1;
		}
	}

	int k = 0;
	for (int i = 0;i < 3;i++) {
		for (int j = 0;j < 9;j++) {
			if (map[i][j].type == 1 && lines[i]==1) {
				map[i][j].peatimer++;
				if (map[i][j].peatimer > 50) {
					map[i][j].peatimer = 0;
					for (k = 0;k < bulletscount && bullets[k].used;k++);
					if (k < bulletscount) {
						bullets[k].used = true;
						bullets[k].speed = 4;
						bullets[k].row = i;
						bullets[k].x = 256 + j * 81 + plt[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = 179 + i * 102 + 14 + 5;
						bullets[k].boom = false;
						bullets[k].frameindex = 0;

						lines[i] = 0;
					}
				}
			}
		}//创建豌豆并初始化
	}
}

void updatepea() {
	for (int i = 0;i < bulletscount;i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > width) {
				bullets[i].used = false;
			}

			if (bullets[i].boom) {
				bullets[i].frameindex++;
				if (bullets[i].frameindex > 3) {
					bullets[i].used = false;
				}
			}
		}
	}//更新豌豆位置.
}

void checkbullet2zm() {
	for (int i = 0;i < bulletscount;i++) {
		if (bullets[i].used && !bullets[i].boom) {
			for (int j = 0;j < zmmax;j++) {
				if (zms[j].used) {
					int x_1 = zms[j].x + 80;
					int x_2 = zms[j].x + 110;
					int x = bullets[i].x;
					if (bullets[i].row == zms[j].row && x > x_1 && x < x_2 && !zms[j].dead) {
						zms[j].blood -= 20;
						if (zms[j].blood <= 0) {
							zms[j].dead = true;
							zms[j].speed = 0;
							zms[j].frameindex = 0;
						}//判断僵尸的死亡

						bullets[i].boom = true;
						bullets[i].speed = 0;
						break;
					}
				}
			}
		}
	}
}

void checkzm2plant() {
	for (int i = 0;i < zmmax;i++) {
		if (zms[i].used) {
			if (zms[i].dead)continue;
			int row = zms[i].row;
			for (int j=0;j < 9;j++) {
				if (map[row][j].type == 0)continue;

				int plantX = 256 + j * 81;
				int x1 = plantX + 10;
				int x2 = plantX + 60;
				int x3 = zms[i].x+80;
				if (x3 > x1 && x3 < x2) {
					if (zms[i].ate) {
						map[row][j].timer++;
						if (map[row][j].type == 1 || map[row][j].type == 2) {
							if (map[row][j].timer > 10) {
								map[row][j].blood -= 20;
								map[row][j].timer = 0;
							}
						}
						if (map[row][j].type == 3) {
							if (map[row][j].timer > 10) {
								map[row][j].blood -= 10;
								map[row][j].timer = 0;
							}
						}
						if (map[row][j].blood <= 0) {
							map[row][j].type = 0;
							zms[i].ate = false;
							zms[i].frameindex = 0;
							zms[i].speed = 2;
							continue;
						}
					}
					if (!zms[i].ate) {
						zms[i].ate = true;
						zms[i].frameindex = 0;
						zms[i].speed = 0;
					}
				}
			}
		}
	}
}

void collisioncheck() {
	checkbullet2zm();
	checkzm2plant();
}

void update() {
	for (int i = 0;i < 3;i++) {
		for (int j = 0;j < 9;j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameindex++;
				if (plt[map[i][j].type - 1][map[i][j].frameindex] == NULL) {
					map[i][j].frameindex = 0;
				}
			}
		}
	}//更新摇晃植物帧数

	createsunshine();
	sunflowersunshine();
	updatesunshine();//阳光

	createzombies();
	updatezombies();//僵尸

	shoot();//发射
	updatepea();

	collisioncheck();//碰撞检测
}

bool checkover() {
	bool status = false;
	if (gamestatus == win) {
	    Sleep(2000);
		loadimage(0, "../res/res/gamewin.png");
		mciSendString("stop ../res/res/bg.mp3", 0, 0, 0);
		mciSendString("play ../res/res/win.mp3", 0, 0, 0);
		status = true;
	}
	else if (gamestatus == fail) {
		Sleep(2000);
		loadimage(0, "../res/res/gameFail.png");
		mciSendString("stop ../res/res/bg.mp3", 0, 0, 0);
		mciSendString("play ../res/res/lose.mp3", 0, 0, 0);
		status = true;
	}
	return status;
}