#define MS_DOS_OUTPUT_CP	437
//#define CHINESE_OUTPUT_CP	936
#define _CRT_SECURE_NO_WARNINGS
#define PLAYERSHIP Head_Ship.next	//玩家飞船指针

#define ms_to_ns 1e06

#define ConsoleWriteLine( fmt, ... ) {ConsoleWrite( fmt, ##__VA_ARGS__  );OutputDebugStringA("\r\n");}
#define ConsoleWrite( fmt, ... ) {char szBuf[MAX_PATH]; sprintf_s(szBuf,fmt, ##__VA_ARGS__ ); OutputDebugStringA(szBuf);}

#include "ShipsWar.h" 



const int WEIGHT = 256;
const int HEIGHT = 192;
const int SYNC_FPS = 40;


const int SHIP_EFX_WIDTH_M = 16;		//船和特效模型每一行最大宽度
const int PROJ_WIDTH_M = 9;	//弹药模型每一行最大宽度
const int TIME_STEP_PROJ_MOVE_Y = 40* ms_to_ns;	// Y方向时间步进单位
const int TIME_STEP_PROJ_MOVE_X = 100 * ms_to_ns;	// X方向时间步进单位
const int TIME_STEP_SPAWN = 1000 * ms_to_ns;
const int TIME_STEP_ENEMY_MOVE = 150 * ms_to_ns;
const int TIME_STEP_ENEMY_FIRE = 200 * ms_to_ns;
const int TIME_STEP_P_FIRE = 150 * ms_to_ns;
const int PLAYER_MAX_CAPACITY = 150;	//最大装弹量

const BOOL ENEMY_MOVE = TRUE;
const BOOL COLLISION_DETECTION = TRUE;
const BOOL DEBUG_SHOW_COLLISION_RANGE = FALSE;
const BOOL ENABLE_ENEMY_SPAWN = TRUE;
const BOOL ENABLE_ENEMY_FIRE = TRUE;

BOOL GAME_OVER = FALSE;
BOOL PLAYER_FIRE = FALSE;


char** data;
int** data_collision;

int i = 1, j = 1;		//Debug 碰撞指示数字显示坐标
long long time1_Ship_Spawn, time2_Ship_Spawn;
long long time_diff_Ship_Spawn;

int Spawn_point_x[9] = { 1,8,17,31,42,54,68,78,85 };
int SP_x_index = rand() % 9;
int scope = 0;

int count = 0;
bool exec_loop = 1;
char kb_inp;




HANDLE hScrBuf_1, hScrBuf_2;

void screen_print_row(char** data, const char* row, COORD xy)
{
	DWORD bytes = 0;
	for (int x = 0; row[x] != '\0'; x++)
	{
		data[xy.Y][xy.X + x] = row[x];
	}

}

void write_show(HANDLE hScrBuf_1, HANDLE hScrBuf_2, char** data, DWORD Weight, DWORD Height)	//交换链
{
	COORD coord = { 0,0 };
	DWORD bytes = 0;
	for (int y = 0; y < (int)Height; y++)
	{
		WriteConsoleOutputCharacterA(hScrBuf_1, data[y], Weight, coord, &bytes);
		coord.Y++;
	}
	coord.Y = 0;
	SetConsoleActiveScreenBuffer(hScrBuf_1);
	//Sleep(200);

	for (int y = 0; y < (int)Height; y++)
	{

		WriteConsoleOutputCharacterA(hScrBuf_2, data[y], Weight, coord, &bytes);
		coord.Y++;
	}
	SetConsoleActiveScreenBuffer(hScrBuf_2);
	coord.Y = 0;
	//Sleep(200);



}


void place(char** data, int** data_colli, char* geometry_y, int* colli_y, int width_m, COORD xy, BOOL place_colli = TRUE)		//通过传入模型首行,每一行最大宽度,来注入模型 //weight_max：模型允许的最大宽度	height//模型实际高度
{
	int X, Y;
	X = xy.X;
	Y = xy.Y;


	for (int y = 0; (geometry_y + (y * width_m))[0] != '\0'; y++)		//遍历模型每一行
	{
		for (int x = 0; (geometry_y + (y * width_m))[x] != '\0'; x++)	//遍历模型每一行的每一个元素
		{
			if (X < 0 || Y < 0)
			{
				continue;
			}
			data[y + Y][x + X] = (geometry_y + (y * width_m))[x];		//放置模型	
			if (place_colli == TRUE)
			{
				data_colli[y + Y][x + X] = (colli_y + (y * width_m))[x];		//放置碰撞模型
			}

		}
	}


}//通用放置函数

void clear_data(char** data)
{
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WEIGHT; x++)
		{
			data[y][x] = ' ';
		}
	}
}

void delay(int timeout_ms)
{
	auto start = std::chrono::system_clock::now();
	while (true)
	{
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
		if (duration > timeout_ms)
		{
			//printf("timeout occurred,timeout %d ms", timeout_ms);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
													////////Projectile/////////

struct Projectile
{
	int damege;
	char geometry[9][9];			//子弹大小不超过8x8,以字符串列表形式储存,最后一行为占位标识
	int collision[9][9];			//同上
	int id;
	Projectile* next;
	COORD position;
	Projectile* Type;
	int height;
	int direction;					//0下1上
	long long time_updated;
	long long time_moved_Y;
	long long time_moved_X;
	COORD move_vector;				//移动矢量
};
int Proj_width_m = PROJ_WIDTH_M;
void Proj_f_spwan(Projectile* Last_proj, Projectile* Type_proj, COORD xy)//生成子弹	将子弹加进链表(上一子弹,子弹类型,子弹id,坐标,)
{
	Projectile* New_proj;
	New_proj = (Projectile*)malloc(sizeof(Projectile));
	if (New_proj != NULL)		//复制Projectile
	{
		int id = (int)(New_proj);		//取地址为什么不会变?
		for (int y = 0; ; y++)
		{
			for (int x = 0; ; x++)
			{
				New_proj->geometry[y][x] = Type_proj->geometry[y][x];
				New_proj->collision[y][x] = id;
				if (Type_proj->geometry[y][x] == '\0')
				{
					New_proj->geometry[y][x + 1] = '\0';
					break;
				}
			}
			if (Type_proj->geometry[y + 1][0] == '\0')
			{
				New_proj->geometry[y + 1][0] = '\0';
				break;
			}
		}
		New_proj->damege = Type_proj->damege;
		New_proj->id = id;
		New_proj->position = xy;
		New_proj->Type = Type_proj;
		New_proj->next = Last_proj->next;
		New_proj->height = Type_proj->height;
		New_proj->direction = Type_proj->direction;
		New_proj->time_updated = Type_proj->time_updated;
		New_proj->time_moved_Y = Type_proj->time_moved_Y;
		New_proj->time_moved_X = Type_proj->time_moved_X;
		New_proj->move_vector = Type_proj->move_vector;
		Last_proj->next = New_proj;


	}



}
void Proj_f_clear_data(char** data, int** data_colli, int weight_max, Projectile* node_Proj)
{
	int X, Y;
	COORD xy = node_Proj->position;
	Projectile* Type = node_Proj->Type;
	Projectile empty_Proj;
	X = xy.X;
	Y = xy.Y;
	for (int y = 0; ; y++)
	{
		for (int x = 0; ; x++)
		{
			empty_Proj.geometry[y][x] = ' ';
			empty_Proj.collision[y][x] = -1;
			if (Type->geometry[y][x + 1] == '\0')
			{
				empty_Proj.geometry[y][x + 1] = '\0';
				break;
			}
		}
		if (Type->geometry[y + 1][0] == '\0')
		{
			empty_Proj.geometry[y + 1][0] = '\0';
			break;
		}
	}
	//空模型替换
	place(data, data_colli, empty_Proj.geometry[0], empty_Proj.collision[0], Proj_width_m, xy);

}
void Proj_f_remove(char** data, int** data_colli, int weight_max, Projectile* Last_proj)
{
	Projectile* Type = Last_proj->next->Type;
	Projectile* node_Proj;

	node_Proj = Last_proj->next;

	if (node_Proj->next == NULL)
	{
		Proj_f_clear_data(data, data_colli, weight_max, node_Proj);
		Last_proj->next = NULL;
		free(node_Proj);
	}
	else
	{
		Proj_f_clear_data(data, data_colli, weight_max, node_Proj);
		Last_proj->next = node_Proj->next;
		node_Proj->next = NULL;
		free(node_Proj);
	}


}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								// Projectile Type定义
Projectile Player_Projectile_normal
{
	20,					//子弹伤害
	{254},				// 模型 
	{-1},				//碰撞模型初始化
	-1,					//id
	NULL,				//下一节点指针初始化
	{0,0},				//坐标
	&Player_Projectile_normal,	//类型
	1,					//高度
	1,					//向上移动
	0,					//更新时间
	0,					//移动时间_Y
	0,					//移动时间_X
	{0,-1}				//移动矢量
};

Projectile Player_Projectile_slant_L
{
	10,					//子弹伤害
	{254},				// 模型 
	{-1},				//碰撞模型初始化
	-1,					//id
	NULL,				//下一节点指针初始化
	{0,0},				//坐标
	&Player_Projectile_slant_L,	//类型
	1,					//高度
	1,					//向上移动
	0,					//更新时间
	0,					//移动时间_Y
	0,					//移动时间_X
	{-1,-1}				//移动矢量
};

Projectile Player_Projectile_slant_R
{
	10,					//子弹伤害
	{254},				// 模型 
	{-1},				//碰撞模型初始化
	-1,					//id
	NULL,				//下一节点指针初始化
	{0,0},				//坐标
	&Player_Projectile_slant_R,	//类型
	1,					//高度
	1,					//向上移动
	0,					//更新时间
	0,					//移动时间_Y
	0,					//移动时间_X
	{1,-1}				//移动矢量
};

Projectile Enemy_Projectile_A
{
	10,					//子弹伤害
	{254},				// 模型 
	{-1},				//碰撞模型初始化
	-1,					//id
	NULL,				//下一节点指针初始化
	{0,0},				//坐标
	&Enemy_Projectile_A,	//类型
	1,					//高度
	0,					//向上移动
	0,					//更新时间
	0,					//移动时间_Y
	0,					//移动时间_X
	{0,1}
};
//////////////////////////////////////Projectile 类//////////////////////////////////////////////////////////////////////

struct Frame_Effect
{
	char geometry[6][16];

};

struct Effect
{
	Frame_Effect Frame[10];
	long long lifetime;
	long long num_frame;
	int id;
	COORD center_position;
	Effect* Type;
	Effect* next;
	long long time_updated;
	long long time_next_frame;
	int present_frame;
};

void Effect_f_trigger(char** data, Effect* Type_Effect, Effect* Last_Effect, COORD poistion)	//特效触发
{
	Effect* New_Effect;
	New_Effect = (Effect*)malloc(sizeof(Effect));
	if (New_Effect != NULL)		//复制Effect
	{

		for (int frame = 0; frame < Type_Effect->num_frame; frame++)
		{
			for (int y = 0; ; y++)
			{
				for (int x = 0; ; x++)
				{
					New_Effect->Frame[frame].geometry[y][x] = Type_Effect->Frame[frame].geometry[y][x];
					if (Type_Effect->Frame[frame].geometry[y][x] == '\0')
					{
						New_Effect->Frame[frame].geometry[y][x] = '\0';
						break;
					}
				}
				if (Type_Effect->Frame[frame].geometry[y + 1][0] == '\0')
				{
					New_Effect->Frame[frame].geometry[y + 1][0] = '\0';
					break;
				}
			}
		}

		New_Effect->id = (int)New_Effect;
		New_Effect->lifetime = Type_Effect->lifetime;
		New_Effect->num_frame = Type_Effect->num_frame;
		New_Effect->Type = Type_Effect->Type;
		New_Effect->time_next_frame = Type_Effect->time_next_frame;
		New_Effect->time_updated = Type_Effect->time_updated;
		New_Effect->center_position.X = poistion.X - Type_Effect->center_position.X;
		New_Effect->center_position.Y = poistion.Y - Type_Effect->center_position.Y;
		New_Effect->present_frame = Type_Effect->present_frame;
		New_Effect->next = Last_Effect->next;
		Last_Effect->next = New_Effect;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								// Effect Type定义
Effect expolde_ship
{
	{
		{	249,
			'\0'
		},
		{
			"         ",
			"      x  ",
			"     x#x   ",
			"      x  ",
			'\0'
		},
		{
			"         ",
			"    * x *",
			"  * xx&xx *",
			"    * x *",
			'\0'
		},
		{
			"      *  ",
			"   *  #  *",
			"*   # & #   *",
			"   *  #  *",
			"      * ",
			'\0'
		},
		{
			"         ",
			"      %   ",
			"    % 0 %    ",
			"      %   ",
			"        ",
			'\0'
		},
		{
			"        ",
			"          ",
			"             ",
			"          ",
			"        ",
			'\0'
		}

},	//帧动画
	200*ms_to_ns,		//持续时间
	6,			//帧数
	-1,			//id
	{5,3},		//中心坐标
	&expolde_ship,//特效类型
	NULL,		//下一个特效
	0,			//时间初始化
	0,			//时间初始化
	0,			//当前帧
};

Effect fire_player
{
	{
		{
			"             ",
			"      #      ",
			'\0'
		},
		{
			"      &      ",
			"  &  &#&  &  ",
			'\0'
		},
		{
			"      &      ",
			"&&         &&",
			'\0'
		},
		{
			"             ",
			"&           &",
			'\0'
		},
		{
			"             ",
			"             ",
			'\0'
		},
	},	//帧动画
	110*ms_to_ns,		//持续时间
	5,			//帧数
	-1,			//id
	{3,2},		//中心坐标
	&expolde_ship,//特效类型
	NULL,		//下一个特效
	0,			//时间初始化
	0,			//时间初始化
	0,			//当前帧
};
//////////////////////////////////////Effect 类//////////////////////////////////////////////////////////////////////


struct Ship
{
	char geometry[6][16];		//飞船大小不超过15x5,以字符串列表形式储存,最后一行为占位标识
	int collision[6][16];		//同上
	int hp;
	int camp;			//阵营 1 玩家 2 敌对
	int id;				//结构体标识
	Ship* next;			//Ship链表	下一个节点
	COORD position;
	Ship* Type;			//船只类型
	int colli_dect_range[2];
	Projectile* bullet;
	int height;			//高度
	long long time_updated;
	long long time_fired;
	long long time_moved_Y;
	COORD center_offset;
	int capacity;		//弹量
};
int Ship_width_m = SHIP_EFX_WIDTH_M;
void Ship_f_spawn(Ship* Last_ship, Ship* Type_ship, COORD poistion)//生成飞船	将飞船加进链表(上一飞船,飞船类型,飞船id,坐标,)
{
	Ship* New_ship;
	New_ship = (Ship*)malloc(sizeof(Ship));
	int id = (int)(New_ship);
	if (New_ship != NULL)		//复制Ship
	{

		for (int y = 0; ; y++)
		{
			for (int x = 0; ; x++)
			{
				New_ship->geometry[y][x] = Type_ship->geometry[y][x];
				New_ship->collision[y][x] = id;
				if (Type_ship->geometry[y][x] == '\0')
				{
					New_ship->geometry[y][x + 1] = '\0';
					break;
				}
			}
			if (Type_ship->geometry[y + 1][0] == '\0')
			{
				New_ship->geometry[y + 1][0] = '\0';
				break;
			}
		}
		New_ship->hp = Type_ship->hp;
		New_ship->camp = Type_ship->camp;
		New_ship->id = id;
		New_ship->position = poistion;
		New_ship->bullet = Type_ship->bullet;
		New_ship->Type = Type_ship;
		New_ship->colli_dect_range[0] = Type_ship->colli_dect_range[0];
		New_ship->colli_dect_range[1] = Type_ship->colli_dect_range[1];
		New_ship->time_fired = Type_ship->time_fired;
		New_ship->time_moved_Y = Type_ship->time_moved_Y;
		New_ship->time_updated = Type_ship->time_updated;
		New_ship->center_offset = Type_ship->center_offset;
		New_ship->capacity = Type_ship->capacity;
		New_ship->next = Last_ship->next;
		Last_ship->next = New_ship;

	}



}
void Ship_f_clear_data(char** data, int** data_colli, int weight_max, int height, COORD xy, Ship* Type)		//清除模型与碰撞数据，用于移动
{
	int X, Y;
	X = xy.X;
	Y = xy.Y;

	Ship empty_ship;
	for (int y = 0; ; y++)
	{
		for (int x = 0; ; x++)
		{
			empty_ship.geometry[y][x] = ' ';
			empty_ship.collision[y][x] = -1;
			if (Type->geometry[y][x + 1] == '\0')
			{
				empty_ship.geometry[y][x + 1] = '\0';
				break;
			}
		}
		if (Type->geometry[y + 1][0] == '\0')
		{
			empty_ship.geometry[y + 1][0] = '\0';
			break;
		}
	}


	//空模型替换
	place(data, data_colli, empty_ship.geometry[0], empty_ship.collision[0], Ship_width_m, xy);

}
void Ship_f_remove(char** data, int** data_colli, int weight_max, int height, COORD xy, Ship* Type, Ship* node_Ship, Ship* last_Ship, Effect* Head_Efx, int* count)
{
	COORD position_Efx;

	position_Efx.X = node_Ship->position.X + node_Ship->center_offset.X;
	position_Efx.Y = node_Ship->position.Y + node_Ship->center_offset.Y;

	Ship_f_clear_data(data, data_colli, Ship_width_m, node_Ship->height, node_Ship->position, node_Ship->Type);
	Effect_f_trigger(data, &expolde_ship, Head_Efx, position_Efx);
	if (node_Ship->camp == 1)
	{
		GAME_OVER = TRUE;
	}
	if (node_Ship->camp == 2 && node_Ship->hp <= 0)
	{
		*count += 1;
	}
	last_Ship->next = node_Ship->next;
	node_Ship->next = NULL;
	free(node_Ship);
	node_Ship = last_Ship;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								// Ship Type定义
Ship Player_ship = {
	{
	"  /|\\",
	"<</|\\>>",
	 "\0"},			//5x1
	{-1},			//碰撞箱
	1000,			//hp
	1,				//阵营1：玩家阵营
	-1,				//对象标识
	NULL,			//下一艘船
	{21,11},		//坐标
	&Player_ship,	//船只类型
	{9,4},			//碰撞检测范围	8x3
	&Player_Projectile_normal,			//弹药类型
	2,				//高度
	0,				//更新时间
	0,				//开火时间
	0,				//移动时间
	{3,1},			//中心偏移量
	48				//弹量
};
Ship Enemy_ship_A = {
	{"<<II>>",
	 "\0"},			//5x1
	{-1},			//碰撞箱
	100,			//hp
	2,				//阵营2：地方阵营
	-1,				//对象标识id
	NULL,			//下一艘船
	{21,11},		//坐标
	&Enemy_ship_A,	//船只类型
	{8,3},			//碰撞检测范围	8x3
	&Enemy_Projectile_A,			//弹药类型
	2,				//高度
	0,				//更新时间
	0,				//开火时间
	0,				//移动时间
	{2,1},			//中心偏移量
	250				//弹量
};
Ship Enemy_ship_B = {
	{"<<HH>>",
	 "\0"},			//5x1
	{-1},			//碰撞箱
	100,			//hp
	2,				//阵营2：地方阵营
	-1,				//对象标识id
	NULL,			//下一艘船
	{21,11},		//坐标
	&Enemy_ship_A,	//船只类型
	{8,3},			//碰撞检测范围	8x3
	&Enemy_Projectile_A,			//弹药类型
	2,				//高度
	0,				//更新时间
	0,				//开火时间
	0,				//移动时间
	{2,1},			//中心偏移量
	250				//弹量
};
//////////////////////////////////////Ship 类//////////////////////////////////////////////////////////////////////


void collision_calulate(char** data, int** data_colli, COORD xy, int range_x, int range_y, Ship* node_Ship, Projectile* Head_Proj)
{
	for (int x = xy.X - 1; x < xy.X - 1 + range_x; x++)
	{
		for (int y = xy.Y - 1; y < xy.Y - 1 + range_y; y++)
		{
			if (data_colli[y][x] != -1 && data_colli[y][x] != node_Ship->id)
			{
				Projectile* node_Proj = Head_Proj;
				BOOL is_projectile_collision = FALSE;
				while (node_Proj->next != NULL)		///待优化 
				{
					if (node_Proj->next->id == data_colli[y][x])
					{
						node_Ship->hp -= node_Proj->next->damege;
						is_projectile_collision = TRUE;
						Proj_f_remove(data, data_colli, Proj_width_m, node_Proj);
					}
					else
					{
						node_Proj = node_Proj->next;		//子弹节点步进
					}

					if (node_Proj == NULL)
					{
						break;
					}
				}
				if (is_projectile_collision == FALSE)
				{
					node_Ship->hp = 0;
				}
			}

		}
	}
}
void debug_collision_range(char** data, int** data_colli, COORD xy, int range_x, int range_y, int source_id)
{
	for (int x = xy.X - 1; x < xy.X - 1 + range_x; x++)
	{
		for (int y = xy.Y - 1; y < xy.Y - 1 + range_y; y++)
		{
			if (data_colli[y][x] == -1)
			{
				data[y][x] = 'D';
			}

		}
	}
}


int main()
{
	SetConsoleOutputCP(437);

	FrameTimer Timer;
	Timer.sample_nframe = 30;//平均帧计算采样帧的数量

	long long tick_sync1;
	long long tick_sync2;

	Ship Head_Ship;//船链表头声明
	Projectile Head_Proj;			//船链表头声明
	Effect Head_Efx;			//特效链表头声明

	COORD SP_xy;
	SP_xy.Y = 1;
	SP_xy.X = Spawn_point_x[SP_x_index];


	Head_Ship.next = NULL;		//链表头初始化
	Head_Proj.next = NULL;
	Head_Efx.next = NULL;
	Head_Ship.position = { 1,1 };
	Head_Proj.position = { 1,1 };
	Head_Efx.center_position = { 1,1 };

	data = (char**)malloc(WEIGHT * sizeof(char*));
	data_collision = (int**)malloc(WEIGHT * sizeof(int*));

	if (data == NULL || data_collision == NULL)
	{
		return 0;
	}
	//图像空间创建
	for (int y = 0; y < HEIGHT; y++)		//创建一个二维数组data，储存图像帧数据
	{
		data[y] = (char*)malloc(WEIGHT * sizeof(char));


		if (data[y] != NULL)		//初始帧：空白
		{
			for (int x = 0; x < WEIGHT; x++)
			{
				data[y][x] = ' ';

			}
		}

	}
	//碰撞空间创建
	for (int y = 0; y < HEIGHT; y++)		//创建一个二维数组data，储存碰撞数据
	{
		data_collision[y] = (int*)malloc(WEIGHT * sizeof(int));


		if (data_collision[y] != NULL)		//初始帧：空白
		{
			for (int x = 0; x < WEIGHT; x++)
			{
				data_collision[y][x] = -1;

			}
		}

	}

	//创建两个缓冲区
	hScrBuf_1 = CreateConsoleScreenBuffer(GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	hScrBuf_2 = CreateConsoleScreenBuffer(GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									//以下运行区//

	BOOL destory = FALSE;
	char COUNT[17] = "COUNT:";
	char HP[9] = "HP:";
	char Proj_num[17] = "Proj_n =xxxx";
	char CAPACITY[27] = "NUMBER OF BULLETS:";
	char FPS[8] = "FPS:";

	Ship_f_spawn(&Head_Ship, &Player_ship, { 72,25 });				//生成玩家飞船	(链表头,玩家船类型,标识符,坐标)
	//Ship_f_spawn(Head_Ship.next, &Enemy_ship_A, { 45,1 });		//生成敌方飞船	(链表头,敌方船类型,标识符,坐标)
	//Ship_f_spawn(Head_Ship.next->next, &Enemy_ship_A, { 25,1 });		//生成敌方飞船	(链表头,敌方船类型,标识符,坐标)2
	time1_Ship_Spawn = Timer.GetTimeExists();
	while (exec_loop)
	{
		Ship* node_Ship = &Head_Ship;
		Projectile* node_Proj = &Head_Proj;
		Effect* node_Efx = &Head_Efx;

		int once_Spawn = 0;
		long long time_start, time_end;
		time_point<steady_clock> timePoint_sync = steady_clock::now();
		
		time_start = timePoint_sync.time_since_epoch().count();

		Timer.UpdateTimer();
		//Timer.FrameSync(30);

		time2_Ship_Spawn = Timer.GetTimeExists();
		time_diff_Ship_Spawn = time2_Ship_Spawn - time1_Ship_Spawn;


		double averFps;
		averFps = Timer.GetAverageFps();
		if(averFps != -1)
		{
			sprintf(FPS + 4, "%.2lf", averFps);
			screen_print_row(data, "       ", { 1,1 });
			screen_print_row(data, FPS, { 1,1 });
		}


		if (PLAYERSHIP != NULL && PLAYERSHIP->camp == 1)
		{
			screen_print_row(data, "       ", { 98,1 });
			screen_print_row(data, "                             ", { 98,2 });

			sprintf(COUNT + 6, "%d", count);
			screen_print_row(data, COUNT, { 98,0 });

			sprintf(HP + 3, "%d", PLAYERSHIP->hp);
			screen_print_row(data, HP, { 98,1 });

			sprintf(CAPACITY + 18, "%d/%d", PLAYERSHIP->capacity, PLAYER_MAX_CAPACITY);
			screen_print_row(data, CAPACITY, { 98,2 });
		}
		/*else		PLAYERSHIP == NULL时 清屏函数接管
		{
			screen_print_row(data, "       ", { 98,0 });

			screen_print_row(data, "       ", { 98,1 });

			screen_print_row(data, "                             ", { 98,2 });
		}*/




		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
												//飞船增改, 刷新,发射子弹//

		int range_w;
		int range_h;
		while (node_Ship != NULL && node_Ship->next != NULL)
		{

			if (node_Ship->next->hp <= 0 || node_Ship->next->position.Y > 108)	//销毁飞船node_Ship.next
			{
				Ship_f_remove(data, data_collision, Ship_width_m, node_Ship->height, node_Ship->position, node_Ship->Type, node_Ship->next/*节点飞船*/, node_Ship/*上一飞船*/, &Head_Efx, &count);
				range_w = node_Ship->colli_dect_range[0];
				range_h = node_Ship->colli_dect_range[1];
			}
			else
			{
				node_Ship = node_Ship->next;	//链表节点前进
				range_w = node_Ship->colli_dect_range[0];
				range_h = node_Ship->colli_dect_range[1];
				place(data, data_collision, node_Ship->geometry[0], node_Ship->collision[0], Ship_width_m, node_Ship->position);		//hp>0则更新数据
				node_Ship->time_updated = Timer.GetTimeExists();
			}



			////////////////////////////////////刷新飞船////////////////////////////////////////////////////////////////////////////]

			if (COLLISION_DETECTION)	//碰撞检测 
			{
				collision_calulate(data, data_collision, node_Ship->position, range_w, range_h, node_Ship, &Head_Proj);
				if (DEBUG_SHOW_COLLISION_RANGE)
				{
					debug_collision_range(data, data_collision, node_Ship->position, range_w, range_h, node_Ship->id);
				}
			}

			if (ENEMY_MOVE && node_Ship->camp == 2 && (node_Ship->time_updated - node_Ship->time_moved_Y) > TIME_STEP_ENEMY_MOVE)			//敌方飞船移动
			{
				node_Ship->time_moved_Y = Timer.GetTimeExists();
				Ship_f_clear_data(data, data_collision, Ship_width_m, node_Ship->height, node_Ship->position, node_Ship->Type);
				node_Ship->position.Y++;
			}

			if (
				ENABLE_ENEMY_SPAWN 
				&& time_diff_Ship_Spawn > TIME_STEP_SPAWN 
				&& node_Ship->next == NULL
				&& once_Spawn == 0
				)
			{
				SP_x_index = rand() % 9;
				SP_xy.X = Spawn_point_x[SP_x_index];
				once_Spawn = 1;
				time1_Ship_Spawn += TIME_STEP_SPAWN;
				ConsoleWriteLine("%x", node_Ship->next);
				Ship_f_spawn(node_Ship, &Enemy_ship_A, SP_xy);
				ConsoleWriteLine("%x", node_Ship->next);
			}

			if (ENABLE_ENEMY_FIRE && node_Ship->camp != 1 && (node_Ship->time_updated - node_Ship->time_fired) > TIME_STEP_ENEMY_FIRE)		//敌方飞船开火
			{
				node_Ship->time_fired = Timer.GetTimeExists();
				COORD Enemy_A_Proj_xy;
				Enemy_A_Proj_xy.X = node_Ship->position.X + 3;
				Enemy_A_Proj_xy.Y = node_Ship->position.Y + 2;
				Proj_f_spwan(&Head_Proj, &Enemy_Projectile_A, Enemy_A_Proj_xy);
			}

			if (node_Ship->camp == 1 && PLAYER_FIRE == TRUE && node_Ship->capacity >= 3)		//玩家发射子弹
			{

				COORD Player_Proj_xy_L;
				COORD Player_Proj_xy_R;
				COORD Player_Proj_xy_C;

				Player_Proj_xy_L.X = PLAYERSHIP->position.X + 0;
				Player_Proj_xy_L.Y = PLAYERSHIP->position.Y - 2;
				Player_Proj_xy_R.X = PLAYERSHIP->position.X + +6;
				Player_Proj_xy_R.Y = PLAYERSHIP->position.Y - 2;
				Player_Proj_xy_C.X = PLAYERSHIP->position.X + 3;
				Player_Proj_xy_C.Y = PLAYERSHIP->position.Y - 2;
				node_Ship->capacity -= 3;
				node_Ship->time_fired = Timer.GetTimeExists();
				PLAYER_FIRE = FALSE;

				PlaySound(TEXT("launch.wav"), NULL, SND_FILENAME | SND_ASYNC);

				Effect_f_trigger(data, &fire_player, &Head_Efx, PLAYERSHIP->position);

				Proj_f_spwan(&Head_Proj, &Player_Projectile_slant_L, Player_Proj_xy_L);
				Proj_f_spwan(Head_Proj.next, &Player_Projectile_slant_R, Player_Proj_xy_R);
				Proj_f_spwan(Head_Proj.next->next, &Player_Projectile_normal, Player_Proj_xy_C);

			}

		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
											///////刷新子弹(移动)//////////
		while (node_Proj->next != NULL)
		{

			node_Proj = node_Proj->next;
			node_Proj->time_updated = Timer.GetTimeExists();
			if (
				node_Proj->time_updated - node_Proj->time_moved_Y > TIME_STEP_PROJ_MOVE_Y
				&& node_Proj->position.Y < 128 && node_Proj->position.Y>0
				&& node_Proj->position.X > 0 && node_Proj->position.X < 256
				)		//子弹移动Y/
			{
				node_Proj->time_moved_Y = Timer.GetTimeExists();
				Proj_f_clear_data(data, data_collision, Proj_width_m, node_Proj);
				node_Proj->position.Y += node_Proj->move_vector.Y;
			}


			if (node_Proj->time_updated - node_Proj->time_moved_X > TIME_STEP_PROJ_MOVE_X - scope
				&& node_Proj->position.Y < 128 && node_Proj->position.Y>0
				&& node_Proj->position.X > 0 && node_Proj->position.X < 256
				)		//子弹移动Y/
			{
				node_Proj->time_moved_X = Timer.GetTimeExists();
				Proj_f_clear_data(data, data_collision, Proj_width_m, node_Proj);
				node_Proj->position.X += node_Proj->move_vector.X;
			}


			place(data, data_collision, node_Proj->geometry[0], node_Proj->collision[0], Proj_width_m, node_Proj->position);

		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
											///////销毁子弹//////////
		node_Proj = &Head_Proj;
		while (node_Proj != NULL && node_Proj->next != NULL)
		{

			if (node_Proj->next != NULL && node_Proj->next->position.Y <= 0)
			{
				Proj_f_remove(data, data_collision, Proj_width_m, node_Proj);
			}
			if (node_Proj->next != NULL && node_Proj->next->position.Y >= 64)
			{
				Proj_f_remove(data, data_collision, Proj_width_m, node_Proj);
			}

			node_Proj = node_Proj->next;


		}


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
												//////特效更新//////////

		while (node_Efx != NULL && node_Efx->next != NULL)
		{

			node_Efx->next->time_updated = Timer.GetTimeExists();
			if (node_Efx->next->time_updated - node_Efx->next->time_next_frame > node_Efx->next->lifetime / node_Efx->next->num_frame)
			{
				place(data, NULL, node_Efx->next->Frame[node_Efx->next->present_frame].geometry[0], NULL, SHIP_EFX_WIDTH_M, node_Efx->next->center_position, FALSE);
				node_Efx->next->present_frame++;
				node_Efx->next->time_next_frame = Timer.GetTimeExists();
			}
			if (node_Efx->next->present_frame >= node_Efx->next->num_frame)
			{
				Effect* temp;
				temp = node_Efx->next;
				node_Efx->next = node_Efx->next->next;
				free(temp);
				temp = NULL;
			}
			node_Efx = node_Efx->next;

		}

		///////////////////////////////////////////////////////游戏更新////////////////////////////////////////////////////////////


		while (_kbhit() && GAME_OVER != TRUE)		//移动实现
		{
			kb_inp = _getch();		//键盘监听

			if (PLAYERSHIP->position.X > 0 && PLAYERSHIP->position.Y > 1 && PLAYERSHIP != NULL)
			{

				Ship_f_clear_data(data, data_collision, Ship_width_m, PLAYERSHIP->height, PLAYERSHIP->position, &Player_ship);
				switch (kb_inp)
				{
				case '+':
					scope += 10000000;
					break;
				case '-':
					scope -= 10000000;
					break;
				case '8':
					PLAYERSHIP->position.Y--;
					break;
				case '2':
					PLAYERSHIP->position.Y++;
					break;
				case '4':
					PLAYERSHIP->position.X--;
					break;
				case '6':
					PLAYERSHIP->position.X++;
					break;
				case '5':
					if (PLAYERSHIP != NULL
						&& PLAYERSHIP->capacity>=3
						&& (PLAYERSHIP->time_updated - PLAYERSHIP->time_fired) > TIME_STEP_P_FIRE
						)
					{

						PLAYER_FIRE = TRUE;
					}
					break;
				case 'r':
					if (PLAYERSHIP->capacity < PLAYER_MAX_CAPACITY) { PLAYERSHIP->capacity += 1; }
					break;
				}


			}
			else {
				Ship_f_clear_data(data, data_collision, Ship_width_m, PLAYERSHIP->height, PLAYERSHIP->position, &Player_ship);
				PLAYERSHIP->position.Y++;
				PLAYERSHIP->position.X++;
			}

		}
		////////////////////////////////////键盘移动////////////////////////////////////////////////////////
		if (GAME_OVER)
		{
			break;

		}

		write_show(hScrBuf_1, hScrBuf_2, data, WEIGHT, HEIGHT);

		timePoint_sync = steady_clock::now();
		time_end = timePoint_sync.time_since_epoch().count();

		delay(1000 / SYNC_FPS + (time_start - time_end)/ms_to_ns);

	}

	char gameover[6][57] =
	{
		"~####~~~####~~##~~~#~#####~~~~~####~~##~~##~#####~#####~",
		"##~~~~~##~~##~###~##~##~~~~~~~##~~##~##~~##~##~~~~##~~##",
		"##~###~######~##~#~#~####~~~~~##~~##~##~~##~####~~#####~",
		"##~~##~##~~##~##~~~#~##~~~~~~~##~~##~~####~~##~~~~##~~##",
		"~####~~##~~##~##~~~#~#####~~~~~####~~~~##~~~#####~##~~##",
		'\0'

	};


	clear_data(data);

	place(data, NULL, gameover[0], NULL, 57, { 40,14 }, FALSE);


	sprintf(COUNT + 6, "%d", count);
	screen_print_row(data, COUNT, { 98,0 });


	write_show(hScrBuf_1, hScrBuf_2, data, WEIGHT, HEIGHT);

	while (true)
	{}
	return 0;

}