#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILENAME "dict.dic"
#define ORDERNUMMAX 54001
#define COMBOS 100
#define FOODNAMESIZE 50
#define FOODTYPE 100
// 套餐结构体
typedef struct
{
    int combo_food_num; // 套餐的食物数量
    char combo_name[FOODNAMESIZE];
    int combo_foods_id[5]; // 存储食物信息
} Combo;

// 菜单结构体
typedef struct
{
    int food_num;                       // 总菜单的食物数量
    int combo_num;                      // 套餐的数量
    char foods[FOODTYPE][FOODNAMESIZE]; // 存储食物信息
    int food_time[FOODTYPE];            // 食物的所需要的制作时间
    int make_time[FOODTYPE];            // 食物当前的制作时间
    int max_capacity[FOODTYPE];         // 食物的最大存储容量
    int capacity[FOODTYPE];             // 食物当前的容量
    int w_1, w_2;
    Combo combos[100]; // 存储套餐信息
} Dict;

typedef struct
{
    int hour;
    int sec;
    int min;
    int isCombo;                  // 是否是套餐
    int food_num;                 // 订单中的食物数量
    int food_finish_flag[5];      // 表示对应食物是否以及被制作
    char food_name[FOODNAMESIZE]; // 食物/套餐名字
    int food_id[5];               // 表示订单内的食物，
} Order;

typedef struct
{
    int order_num; // 表示订单总数
    Order orders[ORDERNUMMAX];
    int is_finish[ORDERNUMMAX];   // 标志是否处理完成;
    int finish_time[ORDERNUMMAX]; // 订单完成的时间
    int finish_num;               // 表示已完成的订单数量
    int fail_num;                 // 表示失败的订单数量
    int handling_order;           // 正在进行的订单数
    int is_open;                  // 0表示关闭系统 1是开启
    int first_not_finish_order;   // 第一个未完成的订单
} System;

void read_dict(Dict *menu);
void read_order(System *system, Dict *menu);
// 制作食物
void produce_food(Dict *menu);
// 处理订单
void handle_order(Dict *menu, System *system, int time);
// 根据食品名字寻找在菜单中的下标
int find_food_id(Dict *menu, char *food_name);
int find_combo_id(Dict *menu, char *combo_name);
// 检查某个订单是否完成
int check_finish(Order *order);
// 将整数时间给解析成 h:m:s
void parse_time(int time, int *h, int *m, int *s);
// 时间增加 1s
void increase_time(int *time);
void print_result(System *system);

int main()
{
    Dict menu;
    read_dict(&menu);
    System system;
    read_order(&system, &menu);

    // 每次减少当前食物的制作时间
    for (int time = 70000; time < 235959; increase_time(&time))
    {
        produce_food(&menu);
        handle_order(&menu, &system, time);

        // 所有订单已经处理完毕
        if (system.fail_num + system.finish_num == system.order_num)
        {
            print_result(&system);
            break;
        }
        for (int i = 0; i < menu.food_num; i++)
        {
            // 判断是否有食物库存达到上限
            if (menu.max_capacity[i] > menu.capacity[i])
            {
                // 表示食物经过一秒的制作
                menu.make_time[i]--;
            }
        }
    }
}

void read_dict(Dict *menu)
{
    if (menu == NULL)
        return;
    FILE *fp = fopen(FILENAME, "r");
    if (fp == NULL)
    {
        puts("FILE OPEN ERROR");
        return;
    }
    // 读取第一行数据
    fscanf(fp, "%d%d", &menu->food_num, &menu->combo_num);
    // 分配空间
    for (int i = 0; i < menu->food_num; i++)
    {
        fscanf(fp, "%s", menu->foods[i]);
    }
    // 读取制作时间数据
    for (int i = 0; i < menu->food_num; i++)
    {
        fscanf(fp, "%d", &menu->food_time[i]);
        menu->make_time[i] = menu->food_time[i];
    }
    // 读取最大容量信息
    for (int i = 0; i < menu->food_num; i++)
    {
        fscanf(fp, "%d", &menu->max_capacity[i]);
    }
    fscanf(fp, "%d%d", &menu->w_1, &menu->w_2);
    // 读取combo数据
    for (int i = 0; i < menu->combo_num; i++)
    {
        menu->combos[i].combo_food_num = 0;
        // 读取套餐名
        fscanf(fp, "%s", menu->combos[i].combo_name);
        // 读取套餐内的食物名
        while (1)
        {
            int ch = fgetc(fp);
            if (ch == '\n' || ch == EOF || ch == 13)
            {
                break;
            }
            char food_name[FOODNAMESIZE];
            fscanf(fp, "%s", food_name);
            menu->combos[i].combo_foods_id[menu->combos[i].combo_food_num++] = find_food_id(menu, food_name);
        }
    }
    // 初始化当前所有食物的容量为0
    for (int i = 0; i < menu->food_num; i++)
    {
        menu->capacity[i] = 0;
    }
    fclose(fp);
}
void read_order(System *system, Dict *menu)
{
    if (system == NULL)
        return;
    system->finish_num = 0;
    system->fail_num = 0;
    system->handling_order = 0;
    system->is_open = 1;
    system->first_not_finish_order = -1;
    scanf("%d", &system->order_num);
    memset(system->finish_time, 0, sizeof(int) * system->order_num);
    // 全部初始化为0， 表示当前订单都没有完成
    memset(system->is_finish, 0, system->order_num * sizeof(int));
    for (int i = 0; i < system->order_num; i++)
    {
        scanf("%d:%d:%d %s", &system->orders[i].hour, &system->orders[i].min, &system->orders[i].sec, system->orders[i].food_name);
        char *food_name = system->orders[i].food_name;
        // 判断是否是套餐
        int combo_id = find_combo_id(menu, food_name);
        if (combo_id != -1)
        {
            // 将套餐信息存入 order的对应信息中
            system->orders[i].food_num = menu->combos[combo_id].combo_food_num;
            memcpy(system->orders[i].food_id, menu->combos[combo_id].combo_foods_id, sizeof(int) * 5);
            memset(system->orders[i].food_finish_flag, 0, sizeof(int) * 5);
            system->orders[i].isCombo = 1;
        }
        else
        {
            system->orders[i].food_num = 1;
            system->orders[i].food_id[0] = find_food_id(menu, food_name);
            system->orders[i].isCombo = 0;
        }
    }
}

void produce_food(Dict *menu)
{
    // 先存储食物
    for (int i = 0; i < menu->food_num; i++)
    {
        if (menu->make_time[i] == 0)
        {
            // 食物的库存+1
            menu->capacity[i]++;
            menu->make_time[i] = menu->food_time[i];
        }
    }
}
void handle_order(Dict *menu, System *system, int time)
{
    int open_pre = system->is_open;
    int first_not_finish_index = system->first_not_finish_order;
    int flag_continue = 1;
    for (int i = first_not_finish_index + 1; i < system->order_num; i++)
    {
        int order_time = system->orders[i].hour * 10000 + system->orders[i].min * 100 + system->orders[i].sec;
        // 如果当前时间还未到下单的时间，那么直接退出
        if (order_time > time)
            break;
        // 判断是否已经完成,或者下单失败
        if (system->is_finish[i] == 1)
        {
            if (flag_continue == 1)
            {
                system->first_not_finish_order = i;
                // printf("time : %d %d\n", time, system->first_not_finish_order);
            }
            continue;
        }
        // 如果当前系统处于关闭状态,并且当前时刻有新的订单，那么直接下单失败
        if (order_time == time && open_pre == 0)
        {
            system->finish_time[i] = -1;
            system->is_finish[i] = 1;
            system->fail_num++;
            break;
        }
        // 处理订单，该订单当前时间之前的订单
        for (int t = 0; t < system->orders[i].food_num; t++)
        {
            // 遍历订单中的每一个食物
            int food_id = system->orders[i].food_id[t];
            // 如果食物没有制作完成,那么检查当前是否有该食物的容量
            if (system->orders[i].food_finish_flag[t] == 0)
            {
                // 如果当前有该个食物
                if (menu->capacity[food_id] > 0)
                {
                    menu->capacity[food_id]--;
                    system->orders[i].food_finish_flag[t] = 1;
                }
            }
        }

        if (check_finish(&system->orders[i]) == 1)
        {
            system->is_finish[i] = 1;
            system->finish_num++;
            system->finish_time[i] = time;

            if (time != order_time)
            {
                system->handling_order--;
                if (system->handling_order < menu->w_2)
                    system->is_open = 1; // 重新打开系统
            }
        }
        else
        {
            flag_continue = 0;
            if (time == order_time)
            {
                system->handling_order++;
                // 系统立即自动关闭(不再接受订单
                if (system->handling_order > menu->w_1)
                    system->is_open = 0;
            }
        }
    }
}

int find_food_id(Dict *menu, char *food_Name)
{
    for (int i = 0; i < menu->food_num; i++)
    {
        if (strcmp(food_Name, menu->foods[i]) == 0)
            return i;
    }
    return -1;
}

int find_combo_id(Dict *menu, char *combo_name)
{
    for (int i = 0; i < menu->combo_num; i++)
    {
        if (strcmp(menu->combos[i].combo_name, combo_name) == 0)
        {
            return i;
        }
    }
    return -1;
}
int check_finish(Order *order)
{
    if (order == NULL)
        return 0;
    for (int i = 0; i < order->food_num; i++)
    {
        if (order->food_finish_flag[i] == 0)
        {
            return 0;
        }
    }
    return 1;
}
void parse_time(int time, int *h, int *m, int *s)
{
    *h = time / 10000;
    *m = (time % 10000) / 100;
    *s = time % 100;
}
// 时间增加 1s
void increase_time(int *time)
{
    int hour, min, sec;
    parse_time(*time, &hour, &min, &sec);
    sec++;
    if (sec >= 60)
    {
        min++;
        sec = 0;
    }
    if (min >= 60)
    {
        hour++;
        min = 0;
    }
    *time = hour * 10000 + min * 100 + sec;
}
void print_result(System *system)
{
    for (int i = 0; i < system->order_num; i++)
    {
        if (system->finish_time[i] == -1)
        {
            puts("Fail");
        }
        else
        {
            int h, m, s;
            parse_time(system->finish_time[i], &h, &m, &s);
            printf("%02d:%02d:%02d\n", h, m, s);
        }
    }
}
