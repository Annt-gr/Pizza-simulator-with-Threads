#pragma once


const int T_ORDER_LOW  = 1;
const int T_ORDER_HIGH = 5;
const int T_PAY_LOW    = 1;
const int T_PAY_HIGH   = 2;
const int P_FAIL       = 5;
const int T_PREP       = 1;
const int T_BAKE       = 10;
const int T_PACK       = 2;
const int T_DELI_LOW   = 5;
const int T_DELI_HIGH  = 15;

int res_tel  = 3;
int res_cook = 2;
int res_oven = 10;
int res_pack = 1;
int res_deli = 7;
int *id;
unsigned int startSeed;

int sum_cash = 0;
int sum_fail = 0;
int sum_succcess = 0;

/**
 * To Find the average time we can sum all the times and then divide by sum_fail and/or sum_success
*/
int avg_callHoldTime = 0;
int avg_serviceTime  = 0;
int avg_coolingTime  = 0;

int max_callHoldTime = -1;
int max_serviceTime  = -1;
int max_coolingTime  = -1;

struct Order {
    int id;
    int pizzas;
    int cash;

    unsigned long int start;   // when customer called, regardless of when the call connected
    unsigned long int baked;   // when all the pizzas were packed
    unsigned long int end;     // when the order was delivered
};
void *initOrder(struct Order *o, int i);
void *call(void *x);
void *prepPizza(struct Order *order, unsigned int *seed);
void *bake(struct Order *order, unsigned int *seed);
void *pack(struct Order *order, unsigned int *seed);
void *deliver(struct Order *order, unsigned int *seed);