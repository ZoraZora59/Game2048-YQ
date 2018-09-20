#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <termios.h>
#include <string.h>
#include <time.h> //时间作为随机数种子

#define GAP 5//方块与边界间隔，方块间间隔为2*GAP
#define SQUARE_NUM 4//4*4个方块的游戏界面

#define UNMOVE 2
#define TAKEOUT 1
#define PUTBACK -1
#define SUCCESS 0
#define ERROR -1
#define GAMEOVER 1
#define FULL 2
#define TRUE 0
#define FALSE 1
#define U 11
#define D 22
#define L 33
#define R 44
#define GAMESAVEPATH "/home/Save.data"

int *plcd;//内存映射mmap
int fd_screen;//显示屏
int fd_touch;//触摸屏
int fd_save;//即时存档
int fd_score;//分数记录
int block_len;//方块边长
int Score;//得分

int Data[SQUARE_NUM][SQUARE_NUM]={0};//输出数据方格
int temp[SQUARE_NUM][SQUARE_NUM]={0};//用于合并时临时处理
int isGameOver;

void lcdClear(int color);
void initTable();
void showTable();
void drawPoint(int x,int y,int color);
void drawBlock(int x0,int y0,int w,int h,int color);
void drawPic(char *file,int x0,int y0);
void putLeft(int value);
void putRight(int value);
void putDown(int value);
void putUp(int value);
void lcdClose();
void setInit(int number1,int number2,int number3,int number4);
void drawWord(int x0,int y0,char ch[],int w,int h,int color);
void drawNum(int x0,int y0,int num);
void drawScore();
void drawHighest();
void scoreUpdate();
void gameSave();
void gameSaveReset();
int gameLoad();
int doMix();
int getTouch();
int gameLogic(int value);
int newBlock();
int lcdInit();
int checkGame();
int scoreRead();

char scoreNum[10][144]={//0~9
{/*--  文字:  0  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x01,0xFF,0x00,0x03,0xEF,
0xC0,0x07,0xC3,0xC0,0x0F,0x81,0xE0,0x0F,0x01,0xF0,0x1F,0x00,0xF0,0x1F,0x00,0xF8,
0x3E,0x00,0xF8,0x3E,0x00,0xF8,0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x7C,
0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,
0x7C,0x7C,0x00,0x7C,0x7E,0x00,0x7C,0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x3E,0x00,0x78,
0x3E,0x00,0xF8,0x3E,0x00,0xF8,0x1F,0x00,0xF8,0x1F,0x00,0xF0,0x0F,0x01,0xF0,0x0F,
0x81,0xE0,0x07,0xC3,0xC0,0x03,0xFF,0x80,0x01,0xFF,0x00,0x00,0x3C,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  1  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x1C,0x00,0x00,0x3C,
0x00,0x0F,0xFC,0x00,0x0F,0xFC,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,
0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,
0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,
0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,
0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,
0x3C,0x00,0x00,0x7E,0x00,0x07,0xFF,0xE0,0x0F,0xFF,0xF0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  2  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x03,0xFF,0xC0,0x07,0xC7,
0xE0,0x0F,0x01,0xF0,0x1E,0x00,0xF0,0x1E,0x00,0xF8,0x3E,0x00,0xF8,0x3E,0x00,0xF8,
0x3F,0x00,0xF8,0x3F,0x00,0xF8,0x3F,0x00,0xF8,0x0E,0x00,0xF8,0x00,0x00,0xF0,0x00,
0x01,0xF0,0x00,0x01,0xE0,0x00,0x03,0xE0,0x00,0x07,0xC0,0x00,0x07,0x80,0x00,0x0F,
0x00,0x00,0x1E,0x00,0x00,0x3C,0x00,0x00,0x78,0x00,0x00,0xF0,0x00,0x01,0xE0,0x00,
0x03,0xC0,0x00,0x07,0x80,0x18,0x0F,0x00,0x18,0x0E,0x00,0x38,0x1C,0x00,0x38,0x38,
0x00,0xF8,0x3F,0xFF,0xF8,0x7F,0xFF,0xF8,0x7F,0xFF,0xF8,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  3  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x03,0xFF,0x00,0x07,0x8F,
0xC0,0x0E,0x03,0xE0,0x1E,0x01,0xE0,0x1E,0x01,0xF0,0x3E,0x01,0xF0,0x3F,0x00,0xF0,
0x1F,0x00,0xF0,0x0E,0x00,0xF0,0x00,0x01,0xF0,0x00,0x01,0xF0,0x00,0x01,0xE0,0x00,
0x03,0xC0,0x00,0x0F,0x80,0x00,0xFE,0x00,0x00,0xFF,0x80,0x00,0x07,0xC0,0x00,0x01,
0xE0,0x00,0x00,0xF0,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0x78,0x00,0x00,0x7C,
0x1E,0x00,0x7C,0x3E,0x00,0x78,0x3F,0x00,0x78,0x3F,0x00,0xF8,0x3E,0x00,0xF8,0x3E,
0x01,0xF0,0x1E,0x03,0xE0,0x0F,0x8F,0xC0,0x03,0xFF,0x80,0x00,0xFC,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  4  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xC0,0x00,0x03,0xC0,0x00,0x07,
0xC0,0x00,0x07,0xC0,0x00,0x0F,0xC0,0x00,0x1F,0xC0,0x00,0x1F,0xC0,0x00,0x3F,0xC0,
0x00,0x7F,0xC0,0x00,0x77,0xC0,0x00,0xE7,0xC0,0x00,0xE7,0xC0,0x01,0xC7,0xC0,0x03,
0x87,0xC0,0x03,0x87,0xC0,0x07,0x07,0xC0,0x0F,0x07,0xC0,0x0E,0x07,0xC0,0x1C,0x07,
0xC0,0x1C,0x07,0xC0,0x38,0x07,0xC0,0x70,0x07,0xC0,0x7F,0xFF,0xFE,0x7F,0xFF,0xFC,
0x00,0x07,0xC0,0x00,0x07,0xC0,0x00,0x07,0xC0,0x00,0x07,0xC0,0x00,0x07,0xC0,0x00,
0x07,0xC0,0x00,0x07,0xC0,0x00,0x7F,0xFC,0x00,0xFF,0xFC,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  5  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xF8,0x0F,0xFF,
0xF8,0x0F,0xFF,0xF0,0x0E,0x00,0x00,0x0E,0x00,0x00,0x0E,0x00,0x00,0x0E,0x00,0x00,
0x0E,0x00,0x00,0x0E,0x00,0x00,0x0E,0x00,0x00,0x0C,0x00,0x00,0x1C,0x7F,0x00,0x1D,
0xFF,0xC0,0x1F,0xFF,0xE0,0x1F,0x83,0xF0,0x1E,0x01,0xF0,0x1E,0x00,0xF8,0x04,0x00,
0xF8,0x00,0x00,0x78,0x00,0x00,0x78,0x00,0x00,0x7C,0x00,0x00,0x7C,0x0C,0x00,0x7C,
0x1E,0x00,0x78,0x3F,0x00,0x78,0x3F,0x00,0x78,0x3E,0x00,0xF8,0x3E,0x00,0xF0,0x1E,
0x01,0xF0,0x1E,0x03,0xE0,0x0F,0xC7,0xC0,0x03,0xFF,0x80,0x00,0x7C,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  6  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x00,0xFF,0xC0,0x01,0xF1,
0xE0,0x03,0xC1,0xF0,0x07,0x81,0xF0,0x0F,0x01,0xF0,0x0F,0x01,0xF0,0x1E,0x00,0x00,
0x1E,0x00,0x00,0x3E,0x00,0x00,0x3E,0x00,0x00,0x3E,0x00,0x00,0x3E,0x1F,0x00,0x3C,
0xFF,0xC0,0x7D,0xFF,0xE0,0x7F,0xC1,0xF0,0x7F,0x80,0xF8,0x7F,0x00,0xF8,0x7E,0x00,
0x7C,0x7E,0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x3C,0x00,0x7C,0x3E,0x00,0x7C,
0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x1F,0x00,0x78,0x1F,0x00,0x78,0x0F,
0x80,0xF0,0x07,0xC1,0xF0,0x03,0xE3,0xE0,0x01,0xFF,0x80,0x00,0x3E,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  7  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0xFF,0xFC,0x1F,0xFF,
0xFC,0x1F,0xFF,0xF8,0x1E,0x00,0x70,0x1C,0x00,0x70,0x3C,0x00,0xE0,0x38,0x00,0xE0,
0x38,0x01,0xC0,0x00,0x01,0xC0,0x00,0x03,0x80,0x00,0x07,0x80,0x00,0x07,0x00,0x00,
0x07,0x00,0x00,0x0F,0x00,0x00,0x0E,0x00,0x00,0x1E,0x00,0x00,0x1C,0x00,0x00,0x3C,
0x00,0x00,0x3C,0x00,0x00,0x7C,0x00,0x00,0x78,0x00,0x00,0x78,0x00,0x00,0x78,0x00,
0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,
0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0x70,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  8  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x03,0xFF,0x80,0x07,0xC3,
0xE0,0x0F,0x01,0xF0,0x1E,0x00,0xF0,0x3C,0x00,0x78,0x3C,0x00,0x78,0x3C,0x00,0x78,
0x3C,0x00,0x78,0x3C,0x00,0x78,0x3E,0x00,0x78,0x1F,0x00,0x78,0x1F,0x80,0xF0,0x0F,
0xE1,0xE0,0x07,0xFB,0xC0,0x03,0xFF,0x80,0x03,0xFF,0x80,0x07,0xBF,0xC0,0x0F,0x0F,
0xE0,0x1E,0x07,0xF0,0x3C,0x01,0xF8,0x3C,0x00,0xF8,0x7C,0x00,0x78,0x78,0x00,0x7C,
0x78,0x00,0x7C,0x78,0x00,0x7C,0x78,0x00,0x7C,0x7C,0x00,0x78,0x3C,0x00,0x78,0x1E,
0x00,0xF0,0x1F,0x01,0xF0,0x0F,0xC7,0xE0,0x03,0xFF,0x80,0x00,0x7C,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{/*--  文字:  9  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x03,0xFF,0x80,0x0F,0xC7,
0xC0,0x0F,0x01,0xE0,0x1E,0x01,0xE0,0x3E,0x00,0xF0,0x3C,0x00,0xF8,0x3C,0x00,0xF8,
0x7C,0x00,0x78,0x7C,0x00,0x78,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x7C,0x00,0x7C,0x7C,
0x00,0x7C,0x7C,0x00,0xFC,0x3E,0x00,0xFC,0x3E,0x01,0xFC,0x1F,0x03,0xFC,0x1F,0x8F,
0xFC,0x0F,0xFF,0x7C,0x03,0xFC,0x7C,0x00,0x00,0xF8,0x00,0x00,0xF8,0x00,0x00,0xF8,
0x00,0x00,0xF8,0x00,0x00,0xF0,0x00,0x01,0xF0,0x1F,0x01,0xE0,0x1F,0x03,0xE0,0x1F,
0x03,0xC0,0x1F,0x07,0x80,0x0F,0x9F,0x00,0x07,0xFE,0x00,0x01,0xF0,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
char score[6][144]={{//score:
/*--  文字:  s  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x01,0xFF,0xF0,0x07,
0xC3,0xF0,0x07,0x00,0xF0,0x0F,0x00,0x70,0x0F,0x00,0x70,0x0F,0x00,0x30,0x0F,0x00,
0x00,0x0F,0xC0,0x00,0x07,0xF8,0x00,0x03,0xFE,0x00,0x00,0xFF,0xC0,0x00,0x1F,0xE0,
0x00,0x07,0xF0,0x00,0x00,0xF8,0x1C,0x00,0x78,0x1C,0x00,0x78,0x1C,0x00,0x78,0x1E,
0x00,0x78,0x1F,0x00,0xF0,0x1F,0xE3,0xE0,0x0F,0xFF,0xC0,0x00,0x1C,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  c  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x01,0xFF,0xC0,0x03,
0xE3,0xE0,0x07,0x81,0xF0,0x0F,0x00,0xF0,0x1F,0x01,0xF0,0x1E,0x01,0xF0,0x3E,0x00,
0xF0,0x3E,0x00,0x00,0x3E,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,
0x3C,0x00,0x00,0x3E,0x00,0x00,0x3E,0x00,0x18,0x1E,0x00,0x18,0x1F,0x00,0x38,0x0F,
0x00,0x70,0x0F,0x80,0xF0,0x07,0xFF,0xE0,0x01,0xFF,0xC0,0x00,0x3E,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  o  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x01,0xFF,0x80,0x07,
0xC3,0xC0,0x0F,0x81,0xE0,0x0F,0x00,0xF0,0x1E,0x00,0x78,0x3E,0x00,0x78,0x3C,0x00,
0x7C,0x3C,0x00,0x3C,0x7C,0x00,0x3C,0x7C,0x00,0x3C,0x7C,0x00,0x3C,0x7C,0x00,0x3C,
0x7C,0x00,0x3C,0x3C,0x00,0x3C,0x3C,0x00,0x7C,0x3C,0x00,0x78,0x1E,0x00,0x78,0x1E,
0x00,0xF0,0x0F,0x01,0xE0,0x07,0xC3,0xC0,0x01,0xFF,0x80,0x00,0x7C,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  r  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xC0,0xF0,0x7F,0xC7,0xFC,0x7F,
0xCF,0xFC,0x03,0xDE,0x7C,0x03,0xFC,0x7C,0x03,0xF8,0x3C,0x03,0xF0,0x00,0x03,0xE0,
0x00,0x03,0xE0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,
0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,0xC0,0x00,0x03,
0xC0,0x00,0x03,0xC0,0x00,0x7F,0xFE,0x00,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  e  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x01,0xFF,0x80,0x03,
0xE7,0xE0,0x07,0x81,0xF0,0x0F,0x00,0xF0,0x0F,0x00,0xF8,0x1E,0x00,0x78,0x1E,0x00,
0x78,0x3E,0x00,0x7C,0x3E,0x00,0x7C,0x3F,0xFF,0xFC,0x3F,0xFF,0xFC,0x3E,0x00,0x00,
0x3E,0x00,0x00,0x3E,0x00,0x00,0x1E,0x00,0x00,0x1E,0x00,0x18,0x1F,0x00,0x38,0x0F,
0x00,0x70,0x07,0xC0,0xE0,0x03,0xFF,0xE0,0x01,0xFF,0x80,0x00,0x3E,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

},{
/*--  文字:  ：  宽x高=24x48  --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x01,0xF8,0x00,0x03,0xFC,0x00,0x03,0xFC,
0x00,0x03,0xFC,0x00,0x01,0xF8,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x01,0xF8,0x00,0x03,
0xFC,0x00,0x03,0xFC,0x00,0x03,0xFC,0x00,0x01,0xFC,0x00,0x01,0xF8,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
char scoreHigest[4][48*6]={{//最高分：
	/*--  文字:  最  宽x高=48x48   --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,
0x60,0x00,0x00,0x07,0x00,0x03,0xF0,0x00,0x00,0x03,0x1F,0xFF,0xF8,0x00,0x00,0x03,
0xFF,0xFF,0xE0,0x00,0x00,0x03,0xFF,0x80,0xE0,0x00,0x00,0x03,0x80,0x00,0xE0,0x00,
0x00,0x03,0x80,0x3E,0xE0,0x00,0x00,0x03,0xFF,0xFE,0xE0,0x00,0x00,0x03,0xFF,0xFF,
0xE0,0x00,0x00,0x03,0xF8,0x01,0xE0,0x00,0x00,0x03,0x80,0x01,0xC0,0x00,0x00,0x03,
0x83,0xFF,0xE0,0x00,0x00,0x03,0xFF,0xFF,0xF0,0x00,0x00,0x07,0xFF,0xFE,0x00,0x00,
0x00,0x03,0xC0,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x1F,0xC0,0x00,0x00,0x00,0xFF,
0xFF,0xE0,0x00,0x1F,0xFF,0xFF,0xFF,0xC0,0x0F,0xFF,0xFF,0xF0,0x00,0x00,0x07,0xFF,
0xFE,0x00,0x00,0x00,0x03,0x1C,0x0E,0x00,0x1C,0x00,0x00,0x1C,0x0E,0x01,0xFE,0x00,
0x00,0x1C,0x7E,0xFF,0xFC,0x00,0x00,0x1F,0xFE,0xFF,0xFC,0x00,0x00,0x1F,0xFE,0x78,
0x38,0x00,0x00,0x1C,0x0E,0x00,0x78,0x00,0x00,0x1C,0x0E,0x78,0x70,0x00,0x00,0x1C,
0xFE,0x38,0xF0,0x00,0x00,0x1F,0xFE,0x1C,0xE0,0x00,0x00,0x1F,0xCE,0x6F,0xE0,0x00,
0x00,0x1C,0x0F,0xE7,0xC0,0x00,0x00,0x1C,0x0F,0xC7,0xC0,0x00,0x00,0x1C,0x3F,0x03,
0xC0,0x00,0x00,0x1D,0xFE,0x07,0xE0,0x00,0x00,0x1F,0xEE,0x0F,0xE0,0x00,0x00,0x3F,
0x8E,0x1E,0x70,0x00,0x03,0xFE,0x0E,0x1C,0x38,0x00,0x03,0xF0,0x0E,0x38,0x3C,0x00,
0x01,0xC0,0x0E,0x70,0x1E,0x00,0x00,0x00,0x0E,0xE0,0x0F,0xF0,0x00,0x00,0x0F,0xC0,
0x0F,0xFC,0x00,0x00,0x0F,0x80,0x0F,0x80,0x00,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,
0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  高  宽x高=48x48   --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x00,0x0E,0x00,
0x00,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,
0x03,0xE0,0x00,0x00,0x00,0x00,0x01,0xE0,0x00,0x00,0x00,0x00,0x01,0xC0,0x7F,0xE0,
0x00,0x00,0x03,0xFF,0xFF,0xF8,0x07,0xFF,0xFF,0xFF,0xFF,0x00,0x07,0xFF,0xFF,0xF8,
0x00,0x00,0x03,0xFF,0x80,0x00,0x00,0x00,0x00,0x01,0x80,0x03,0x80,0x00,0x00,0x01,
0xC3,0xFF,0xC0,0x00,0x00,0x01,0xFF,0xFF,0x80,0x00,0x00,0x01,0xFF,0xFB,0x80,0x00,
0x00,0x01,0xE0,0x07,0x80,0x00,0x00,0x01,0xE0,0x07,0x00,0x00,0x00,0x01,0xE0,0x07,
0x00,0x00,0x00,0x01,0xFF,0xFF,0x80,0x00,0x00,0x01,0xFF,0xFF,0x80,0x00,0x00,0x01,
0xFE,0x00,0x03,0x00,0x00,0xC0,0xE0,0x00,0x07,0x80,0x00,0xE0,0xC0,0x1F,0xFF,0xC0,
0x00,0xE1,0xFF,0xFF,0xFF,0x80,0x00,0xFF,0xFF,0xFF,0xC7,0x80,0x00,0xFF,0xFC,0x00,
0x07,0x00,0x00,0xF0,0x00,0x04,0x07,0x00,0x00,0xF0,0xC0,0x1E,0x07,0x00,0x00,0xF0,
0xFF,0xFF,0x07,0x00,0x00,0xF0,0xFF,0xFE,0x07,0x00,0x00,0xF0,0xFF,0x1E,0x07,0x00,
0x00,0xF0,0xF0,0x1E,0x0F,0x00,0x00,0xF0,0xF0,0x1E,0x0F,0x00,0x00,0xF0,0xF0,0x1C,
0x0E,0x00,0x00,0xF0,0xFF,0xFE,0x0E,0x00,0x00,0xF0,0xFF,0xFE,0x0E,0x00,0x00,0xF0,
0xFE,0x00,0x0E,0x00,0x00,0xF0,0xF0,0x00,0x1E,0x00,0x00,0xF0,0x70,0x03,0x1C,0x00,
0x00,0xF0,0x00,0x03,0xDC,0x00,0x00,0xF0,0x00,0x01,0xFC,0x00,0x00,0xF0,0x00,0x00,
0xFC,0x00,0x00,0x70,0x00,0x00,0x78,0x00,0x00,0x30,0x00,0x00,0x38,0x00,0x00,0x00,
0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  分  宽x高=48x48   --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x01,0xE0,
0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,
0x00,0x70,0x00,0x00,0x00,0x00,0x60,0x38,0x00,0x00,0x00,0x00,0x70,0x18,0x00,0x00,
0x00,0x00,0x70,0x1C,0x00,0x00,0x00,0x00,0xF8,0x0E,0x00,0x00,0x00,0x00,0xF0,0x0E,
0x00,0x00,0x00,0x01,0xE0,0x07,0x00,0x00,0x00,0x01,0xE0,0x07,0x00,0x00,0x00,0x03,
0xC0,0x03,0x80,0x00,0x00,0x03,0x80,0x01,0xC0,0x00,0x00,0x07,0x80,0x01,0xC0,0x00,
0x00,0x07,0x00,0x00,0xE0,0x00,0x00,0x0F,0x00,0x00,0xF0,0x00,0x00,0x1E,0x00,0x00,
0x70,0x00,0x00,0x1C,0x00,0x00,0x38,0x00,0x00,0x38,0x00,0x03,0xBC,0x00,0x00,0x78,
0x00,0x7F,0xDE,0x00,0x00,0xFF,0xFF,0xFF,0xDF,0xE0,0x00,0xEF,0xFF,0xFF,0x8F,0xFC,
0x01,0xC7,0xFC,0x07,0x8F,0xE0,0x03,0x80,0x1C,0x07,0x80,0x00,0x07,0x00,0x3C,0x07,
0x00,0x00,0x0E,0x00,0x38,0x07,0x00,0x00,0x1C,0x00,0x38,0x07,0x00,0x00,0x00,0x00,
0x78,0x07,0x00,0x00,0x00,0x00,0x70,0x0F,0x00,0x00,0x00,0x00,0x70,0x0F,0x00,0x00,
0x00,0x00,0xF0,0x0E,0x00,0x00,0x00,0x00,0xE0,0x0E,0x00,0x00,0x00,0x01,0xC0,0x0E,
0x00,0x00,0x00,0x03,0xC0,0x1E,0x00,0x00,0x00,0x03,0x80,0x1E,0x00,0x00,0x00,0x07,
0x00,0x1C,0x00,0x00,0x00,0x0F,0x18,0x1C,0x00,0x00,0x00,0x1E,0x1E,0x3C,0x00,0x00,
0x00,0x3C,0x0F,0x3C,0x00,0x00,0x00,0xF0,0x03,0xF8,0x00,0x00,0x01,0xE0,0x01,0xF8,
0x00,0x00,0x07,0x80,0x00,0xF0,0x00,0x00,0x0F,0x00,0x00,0x70,0x00,0x00,0x0C,0x00,
0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},{
/*--  文字:  ：  宽x高=48x48   --*/
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,
0x01,0xF8,0x00,0x00,0x00,0x00,0x03,0xFC,0x00,0x00,0x00,0x00,0x03,0xFC,0x00,0x00,
0x00,0x00,0x03,0xFC,0x00,0x00,0x00,0x00,0x01,0xF8,0x00,0x00,0x00,0x00,0x00,0xF0,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x01,0xF8,0x00,0x00,0x00,0x00,0x03,0xFC,
0x00,0x00,0x00,0x00,0x03,0xFC,0x00,0x00,0x00,0x00,0x03,0xFC,0x00,0x00,0x00,0x00,
0x01,0xFC,0x00,0x00,0x00,0x00,0x01,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

int main(int argc,char *argv[])
{
	int check,highestScore;//检查值
	isGameOver = FALSE;//初始化游戏运行判定
	Score=0;
	if(check=lcdInit()!=SUCCESS)//屏幕初始化
	{
		return ERROR;
	}
	highestScore=scoreRead();
	if(check=gameLoad()!=SUCCESS)//加载存档
	{
		initTable();//初始化空白矩阵图案
   	 	srand((int)time(0)); //随机数种子
		setInit(rand(),rand(),rand(),rand());//随机生成初始两方格
	}
    showTable();//打印数据
    while(isGameOver!=TRUE)//游戏运行判断
	{
		check=gameLogic(getTouch());//游戏获取输入并进行运算返回值为运行成功和输入错误
		if(check==SUCCESS)//运行成功则进入下一轮，输入错误则继续进行输入
		{
			printf("\nScore:::%d\nHighest Score:%d\n\n",Score,highestScore);
			drawNum(50,250,highestScore);
			drawNum(50,120,Score);
			check=newBlock();//检测是否有空格，在可能的情况下生成新砖块
			showTable();//打印数据
			gameSave();//即时存档
			if(check==FULL||check==UNMOVE)//检测矩阵已满
			{
				printf("Data FULL\n");
				isGameOver=checkGame();//若有相邻相同数据则继续游戏，否则判定游戏结束
			}
			else if(check==ERROR)//新砖块生成发生未知错误
			{
				printf("New Block Initialization in Unknown Error\n");
				return ERROR;
			}
		}
	}
	gameSaveReset();
	if(Score>highestScore)//最高分判断和更新
	{
		printf("New Highest Score!!!\n\n");
		scoreUpdate();
	}
	printf("Game Over\n\n");//游戏结束并关闭显示屏等
	lcdClose();//关闭屏幕
    return SUCCESS;//感谢游玩
}

void gameSave()//即时保存
{
	fd_save=open(GAMESAVEPATH,O_RDWR | O_CREAT | O_TRUNC);
	if(fd_save == -1)
	{
		printf("Save File Open Error\n");
		close(fd_save);
		return;
	}
	printf("New Save\n");
	write(fd_save,Data,sizeof(Data));
	close(fd_save);
}

void gameSaveReset()//清空存档
{
	remove(GAMESAVEPATH);
}

int gameLoad()//加载存档
{
	fd_save=open(GAMESAVEPATH,O_RDWR | O_CREAT);
	if(fd_save != ERROR)
	{
		printf("Loading Save File\n");
		fd_save=open(GAMESAVEPATH,O_RDONLY);
		if(read(fd_save,Data,sizeof(Data))!=sizeof(Data))
		{
			printf("Save File Error\n");
			close(fd_save);
			return ERROR;
		}
		close(fd_save);
		return SUCCESS;
	}
}

int scoreRead()//读取历史最高分
{
	int highestScore=0;
	fd_score=open("/home/Score.data",O_RDWR | O_CREAT | O_EXCL);
	if(fd_score == ERROR)
	{
		printf("Highest Score Already Exist\n");
		fd_score=open("/home/Score.data",O_RDONLY);
		read(fd_score,&highestScore,sizeof(int));
		close(fd_score);
	}
	else 
	{
		fd_score=open("/home/Score.data",O_RDWR | O_CREAT);
		printf("Score file Create");
		write(fd_score,&highestScore,sizeof(int));
		close(fd_score);
	}
	return highestScore;
}

void scoreUpdate()//更新最高分
{
	fd_score=open("/home/Score.data",O_WRONLY);
	write(fd_score,&Score,sizeof(int));
	printf("Score File Update\n");
	close(fd_score);
}

int checkGame()//判断游戏是否结束
{
	int i,j;
	for(i=0;i<SQUARE_NUM;i++)
	{
		for(j=0;j<SQUARE_NUM;j++)
		{
			if( (j<SQUARE_NUM-1&&Data[i][j]==Data[i][j+1])||(i<SQUARE_NUM-1&&Data[i][j]==Data[i+1][j]) )
			{
				printf("Still Live\n");
				return FALSE;
			}
		}
	}
	printf("Game Over\n");
	return TRUE;
}

void drawPoint(int x,int y,int color)//逐像素显示调用
{
	*(plcd+(480-y)*800+x)=color; 
}

void lcdClear(int color)//纯色清屏
{
	memset(plcd, color, 800*480*4);
}

void drawBlock(int x0,int y0,int w,int h,int color)//在（x0,y0）处绘制宽w、高h、颜色为color的实心正方形
{
	int x,y;
	for(x=x0;x<=x0+w;x++)
	{
		for(y=y0;y<=y0+h;y++)
		{
			drawPoint(x,y,color);
		}
	}
}	

void drawPic(char *file,int x0,int y0)//按比例在（x0，y0）处绘制边长为block_len、路径在*file的图片
{
	int fd=open(file,O_RDWR);
	if(fd==-1)
	{
		printf("pic %s error\n",file);
		return;
	}
	
	//获取图片的边长
	int len;
	lseek(fd, 0x12, SEEK_SET);
	read(fd, &len, 4);
	lseek(fd, 54, SEEK_SET);
	
	//遍历数组中的每一个字节，得出图片的每一个像素点
	char bgr[block_len*block_len*3];
	read(fd,bgr,block_len*block_len*3);
	close(fd);
	
	int i, j;
	int r, g, b, color;
	int pos_x, pos_y;
	double ratio_len = block_len*1.0 / len;
	for(i = 0; i < block_len; i++)
	{
		pos_y = i / ratio_len;
		for(j = block_len; j > 0; j--)
		{
			pos_x = j / ratio_len;
			b = bgr[pos_y*len*3 + pos_x*3];
			g = bgr[pos_y*len*3 + pos_x*3 + 1];
			r = bgr[pos_y*len*3 + pos_x*3 + 2];
			color = 0 << 24 | r << 16 | g << 8 | b;
			drawPoint(j + x0, i + y0, color);
		}
	}
}

void initTable()//初始化游戏界面（无数据）
{
	//LCD清屏
	lcdClear(0xfffffacd);
	drawBlock(360,40,400,400,0xffffd700);
	
	drawScore();
	drawHighest();
	//绘制初始方格
	int x,y;
	block_len=(400-GAP*2*SQUARE_NUM)/SQUARE_NUM;
	for(x=360;x<=670;)
	{
		for(y=40;y<=350;)
		{
			drawBlock(x+GAP,y+GAP,block_len,block_len,0xff90ee90);
			y=y+block_len+2*GAP;
		}
		x=x+block_len+2*GAP;
	}
	printf("Empty Frame Initialization Success\n");
}

int getTouch()//触摸屏判断滑动方向
{
	int i,k,x1,x2,y1,y2;
	struct input_event ev;
	x1=y1=0;
	while(1)
	{
		if(read(fd_touch,&ev,sizeof(ev))&&(ev.type == EV_ABS))
		{
			if(ev.code == ABS_X)
			{
				if(x1==0)
					x1 = ev.value;
				x2 = ev.value;
			}else if(ev.code == ABS_Y)
			{
				if(y1==0)
					y1 = ev.value;
				y2 = ev.value;
			}else if((ev.code==ABS_PRESSURE)&&(ev.value==0))
			{
				printf("Get Touch Success\n");
				//printf("x1=%d,y1=%d,x2=%d,y2=%d\n",x1,y1,x2,y2);
				k=(y1-y2)*1.0/(x2-x1);
				if((k>=1||k<=-1)&&y2<y1)
				{
					printf("down\n");
					return D;
				}
				else if((k>=1||k<=-1)&&y2>y1)
				{
					printf("up\n");
					return U;
				}
				else if((k>-1&&k<1)&&x2>x1)
				{
					printf("right\n");
					return R;
				}
				else if((k>-1&&k<1)&&x2<x1)
				{
					printf("left\n");
					return L;
				} 
				break;
			}
		}
	}
}

int gameLogic(int value)//游戏逻辑部分
{
	int check=ERROR;
	switch(value)
	{
		case U:
			putUp(TAKEOUT);
			check=doMix();
			putUp(PUTBACK);
			break;
		case L:
			putLeft(TAKEOUT);
			check=doMix();
			putLeft(PUTBACK);
			break;
		case D:
			putDown(TAKEOUT);
			check=doMix();
			putDown(PUTBACK);
			break;
		case R:
			putRight(TAKEOUT);
			check=doMix();
			putRight(PUTBACK);
			break;
		default:
			break;
	}
	if(check==SUCCESS)
	{
		
		return SUCCESS;//合并成功
	}
	else
	{
		return ERROR;//合并失败
	}
}

void showTable()//打印数据
{
    int i,j;
	int x,y;
	for(i=0;i<SQUARE_NUM;i++)
	{
		for(j=0;j<SQUARE_NUM;j++)
		{
			printf("%d  ",Data[i][j]);
			x=360+(block_len+2*GAP)*j+GAP;
			y=40+(block_len+2*GAP)*(SQUARE_NUM-1-i)+GAP;
			switch(Data[i][j])
			{
				case 0:
					drawBlock(x , y , block_len , block_len , 0xff90ee90);
					break;
				case 2:
					drawPic("/home/num_bmp/color_x80_2.bmp" , x , y);
					break;
				case 4:
					drawPic("/home/num_bmp/color_x80_4.bmp" , x , y);
					break;
				case 8:
					drawPic("/home/num_bmp/color_x80_8.bmp" , x , y);
					break;
				case 16:
					drawPic("/home/num_bmp/color_x80_16.bmp" , x , y);
					break;
				case 32:
					drawPic("/home/num_bmp/color_x80_32.bmp" , x , y);
					break;
				case 64:
					drawPic("/home/num_bmp/color_x80_64.bmp" , x , y);
					break;
				case 128:
					drawPic("/home/num_bmp/color_x80_128.bmp" , x , y);
					break;
				case 256:
					drawPic("/home/num_bmp/color_x80_256.bmp" , x , y);
					break;
				case 512:
					drawPic("/home/num_bmp/color_x80_512.bmp" , x , y);
					break;
				case 1024:
					drawPic("/home/num_bmp/color_x80_1024.bmp" , x , y);
					break;
				case 2048:
					drawPic("/home/num_bmp/game_over_2048.bmp" ,x , y);
					break;
				default:break;
			}
		}
		printf("\n");
	}
}

int newBlock()// 新方块
{
    int i,j,count,random;
    count=0;
    for(i=0;i<SQUARE_NUM;i++)
    {
        for(j=0;j<SQUARE_NUM;j++)
        {
            if(Data[i][j]==0)
            {
                count++;
            }
        }
    }
    if(count==0)
    {
        return FULL;
    }
    else if(count==1)
    {
        for(i=0;i<SQUARE_NUM;i++)
        {
            for(j=0;j<SQUARE_NUM;j++)
            {
                if(Data[i][j]==0)
                {
                    random=rand();
                    if(random%4>2)
                    {
                        Data[i][j]=4;
                    }
                    else 
                    {
                        Data[i][j]=2;
                    }
					printf("New Block Success\n");
                    return SUCCESS;
                }
            }
        }
    }
    random=rand();
    random=random%count;
    count-=random;
    random=(random%4>2)?4:2;
    for(i=0;i<SQUARE_NUM;i++)
    {
        for(j=0;j<SQUARE_NUM;j++)
        {
            if(Data[i][j]==0)
            {
                if(count==0)
                {
                    Data[i][j]=random;
                    return SUCCESS;
                }
                else
                {
                    
                    count--;
                }
            }
        }
    }
	return UNMOVE;
}

int doMix()// 进行合并
{
    int i,j,m,nextNum,isChange;
	isChange=0;
	for(i=0;i<SQUARE_NUM;i++)
	{
		nextNum=-1;
		for(j=0;j<SQUARE_NUM;j++)
		{
			nextNum=-1;
			for(m=j+1;m<SQUARE_NUM;m++)
			{
				if(temp[i][m]!=0)
				{
					nextNum=temp[i][m];
					break;
				}
			}
			if(nextNum!=-1)
			{
				isChange++;
				if(temp[i][j]==0)
				{
					temp[i][j]=temp[i][m];
					temp[i][m]=0;
					j--;
				}
				else if(temp[i][j]==temp[i][m])
				{
					temp[i][j]+=temp[i][j];
					Score+=temp[i][j];//加分
					printf("Score Add Success\n");
					temp[i][m]=0;
					j--;
				}
			}
		}
	}
	if(isChange==0)
	{
		printf("No Change\n");
		return ERROR;
	}
	else
	{
		printf("Mix Success\n");
		return SUCCESS;
	}
}

void putUp(int value)//上方向合并转动
{
	int i,j,m,n;
	//up
	if(value>0)
	{
		for(i=0,n=SQUARE_NUM-1;i<SQUARE_NUM;i++,n--)
		{
			for(j=m=0;j<SQUARE_NUM;j++,m++)
			{
				temp[i][j]=Data[m][n];
			}
		}
	}
	else
	{
		for(i=0,n=0;i<SQUARE_NUM;i++,n++)
		{
			for(j=0,m=SQUARE_NUM-1;j<SQUARE_NUM;j++,m--)
			{
				Data[i][j]=temp[m][n];
			}
		}
	}
}

void putDown(int value)//下方向合并转动
{
	int i,j,m,n;
	//up
	if(value>0)
	{
		for(i=0,n=0;i<SQUARE_NUM;i++,n++)
		{
			for(j=0,m=SQUARE_NUM-1;j<SQUARE_NUM;j++,m--)
			{
				temp[i][j]=Data[m][n];
			}
		}
	}
	else
	{
		for(i=0,n=SQUARE_NUM-1;i<SQUARE_NUM;i++,n--)
		{
			for(j=m=0;j<SQUARE_NUM;j++,m++)
			{
				Data[i][j]=temp[m][n];
			}
		}
	}
}

void putLeft(int value)//左方向合并转动
{
	int i,j;
	if(value>0)
	{
		for(i=0;i<SQUARE_NUM;i++)
		{
			for(j=0;j<SQUARE_NUM;j++)
			{
				temp[i][j]=Data[i][j];
			}
		}
	}
	else
	{
		for(i=0;i<SQUARE_NUM;i++)
		{
			for(j=0;j<SQUARE_NUM;j++)
			{
				Data[i][j]=temp[i][j];
			}
		}
	}
}

void putRight(int value)//右方向合并转动
{
	int i,j,n;
	if(value>0)
	{
		for(i=0;i<SQUARE_NUM;i++)
		{
			for(j=0,n=SQUARE_NUM-1;j<SQUARE_NUM;j++,n--)
			{
				temp[i][j]=Data[i][n];
			}
		}
	}
	else
	{
		for(i=0;i<SQUARE_NUM;i++)
		{
			for(j=0,n=SQUARE_NUM-1;j<SQUARE_NUM;j++,n--)
			{
				Data[i][j]=temp[i][n];
			}
		}
	}
}

void setInit(int number1,int number2,int number3,int number4)//初始化两方格
{
	int k1=number1%SQUARE_NUM,k2=number2%SQUARE_NUM;
	if(number1%100>70)
	{
		Data[k1][k2]=4;
	}
	else
	{
		Data[k1][k2]=2;
	}
	while(number3%SQUARE_NUM==k1)
	{
		number3=rand();
	}
	k1=number3%SQUARE_NUM;
	k2=number4%SQUARE_NUM;
	Data[k1][k2]=2;
	printf("Source Block Initialization Success\n");
}

int lcdInit()//打开并初始化屏幕
{
	//打开文件
	fd_screen=open("/dev/fb0",O_RDWR);
	if(fd_screen == ERROR)
	{
		printf("Screen open error\n");
		return ERROR;
	}
	//触摸屏
	fd_touch=open("/dev/event0",O_RDWR);
	if(fd_touch == ERROR)
	{
		printf("Touchpad open error\n");
		return ERROR;
	}
	//内存映射
	plcd=mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,fd_screen,0);
	if(NULL == plcd)
	{
		printf("open lcd error\n");
		return ERROR;
	}
	printf("Lcd Screen Initialization Success\n");
	return SUCCESS;
}

void lcdClose()//关闭屏幕
{
	munmap(plcd,800*480*4);
	close(fd_screen);
	close(fd_touch);
	printf("Lcd Screen Close Success\n");
}

void drawWord(int x0,int y0,char ch[],int w,int h,int color)//在（x0,y0）显示一个宽w,高h文字
{
	int i,index;
	int n=w/8;	//一行n个字节
	
	//遍历数组
	for(index=0;index<n*h;index++)	//一个汉字的字模是n个字节
	{
		for(i=7;i>=0;i--)
		{
			if(ch[index]&(1<<i))
			{
				drawPoint(8*(index%n)+7-i+x0,480-index/n-y0,color);
			}
		}
	}
}

void drawNum(int x0,int y0,int num)//显示五位以内的数字
{
	int i;
	int n[5]={0};
	n[0]=num/10000;
	n[1]=num%10000/1000;
	n[2]=num%1000/100;
	n[3]=num%100/10;
	n[4]=num%10;
	
	drawBlock(x0,480-y0-50,300,50,0xfffffacd);
	for(i=0;i<5;i++)
	{
		if(i==0&&n[0]==0);
		else if(i==1&&n[0]==0&&n[1]==0);
		else if(i==2&&n[0]==0&&n[1]==0&&n[2]==0);
		else drawWord(x0+i*24,y0,scoreNum[n[i]],24,48,0xffa07a);
	}
}

void drawScore()//显示score:
{
	int i;
	for(i=0;i<6;i++)
	{
		drawWord(20+i*24,60,score[i],24,48,0xffcd8500);
	}
}

void drawHighest()//显示最高分：
{
	int i;
	for(i=0;i<4;i++)
	{
		drawWord(20+i*48,200,scoreHigest[i],48,48,0xffcd8500);
	}
}