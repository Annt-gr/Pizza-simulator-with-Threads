#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "p3160264-p3190054-p3140312.h"


pthread_mutex_t lock_tel;
pthread_mutex_t lock_cook;
pthread_mutex_t lock_oven;
pthread_mutex_t lock_pack;
pthread_mutex_t lock_deli;
pthread_cond_t  cond_tel;
pthread_cond_t  cond_cook;
pthread_cond_t  cond_oven;
pthread_cond_t  cond_pack;
pthread_cond_t  cond_deli;

/**
 * Initiallize the order's variables;
*/
void *initOrder(struct Order *o, int i) {
    o->id     = i;
    o->pizzas = 0;
    o->cash   = 0;
    o->start  = 0;
    o->baked  = 0;
    o->end    = 0;
}

void *call(void *x){

    int id = *(int *)x;
    unsigned int seed = startSeed + id;
    int rc;
    struct Order* order = malloc(sizeof(struct Order));
    (*initOrder)(order, id);
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    order->start = time.tv_sec;

//    printf("Customer No.%d is calling.\n",id);

    rc = pthread_mutex_lock(&lock_tel);
    while (res_tel == 0) {
//	    printf("Customer No.%d on hold...\n", id);
	    rc = pthread_cond_wait(&cond_tel, &lock_tel);
    }
    clock_gettime(CLOCK_REALTIME, &time);
	printf("Now receiving call of customer No.%d\n", id);
    res_tel--;
    avg_callHoldTime += (int)(time.tv_sec - order->start);
    if (max_callHoldTime < (int)(time.tv_sec - order->start)) {
        max_callHoldTime = (int)(time.tv_sec - order->start);
    }
//    printf("HoldTime: %d\n", avg_callHoldTime);
    rc = pthread_mutex_unlock(&lock_tel);

    sleep((rand_r(&seed) % T_PAY_HIGH) + T_PAY_LOW); //time delay for each payment

    if((rand_r(&seed) % 100) < P_FAIL) {
        rc = pthread_mutex_lock(&lock_tel);
        printf("Order No.%d failed successfully!\n", id);
        res_tel++;
        sum_fail++;
        rc = pthread_mutex_unlock(&lock_tel);
        rc = pthread_cond_signal(&cond_tel);
        pthread_exit(NULL);
    }
    order->pizzas = (rand_r(&seed) % T_ORDER_HIGH) + T_ORDER_LOW; //Amount of pizzas ordered
    order->cash = order->pizzas * 10;

    rc = pthread_mutex_lock(&lock_tel);
    printf("Order No.%d received successfully!\n", order->id);
    res_tel++;
    sum_succcess++;
    sum_cash += order->cash;
    rc = pthread_mutex_unlock(&lock_tel);
    rc = pthread_cond_signal(&cond_tel);
    
    (*prepPizza)(order, &seed);

    avg_serviceTime += (int)(order->end - order->start);
    if (max_serviceTime < (int)(order->end - order->start)) {
        max_serviceTime = (int)(order->end - order->start);
    }
    avg_coolingTime += (int)(order->end - order->baked);
    if (max_coolingTime < (int)(order->end - order->baked)) {
        max_coolingTime = (int)(order->end - order->baked);
    }

    free(order);
    pthread_exit(NULL);
}

void *prepPizza(struct Order *order, unsigned int *seed){

    int rc;
//    printf("Order No.%d needs to be prepared.\n",order->id);

    rc = pthread_mutex_lock(&lock_cook);
    while (res_cook == 0) {
//	    printf("Order No.%d on hold... no cooks available.\n", order->id);
	    rc = pthread_cond_wait(&cond_cook, &lock_cook);
    }
//	printf("Now preparing order No.%d\n", order->id);
    res_cook--;
    rc = pthread_mutex_unlock(&lock_cook);

    sleep(order->pizzas * T_PREP); //Time to prepare each pizza

//    printf("Order No.%d prepared successfully \n", order->id);  

    (*bake)(order, seed);
}

void *bake(struct Order *order,unsigned int *seed){

    int rc;
//    printf("Order No.%d: %d pizzas need to be baked.\n",order->id, order->pizzas);

    struct timespec time;
    rc = pthread_mutex_lock(&lock_oven);
    
    while (res_oven < order->pizzas) {
//	    printf("Order No.%d on hold... not enough ovens available.\n", order->id);
	    rc = pthread_cond_wait(&cond_oven, &lock_oven);
    }
//	printf("Now baking order No.%d\n", order->id);
    res_oven -= order->pizzas;
    rc = pthread_mutex_unlock(&lock_oven);
    rc = pthread_mutex_lock(&lock_cook);
    res_cook++;
    rc = pthread_mutex_unlock(&lock_cook);
    rc = pthread_cond_signal(&cond_cook);

    sleep(T_BAKE); //Time to bake the pizzas

    rc = pthread_mutex_lock(&lock_oven);
    printf("Order No.%d pizzas now getting cold! \n", order->id);
    clock_gettime(CLOCK_REALTIME, &time);
    order->baked = time.tv_sec;
    res_oven += (order->pizzas);
    rc = pthread_mutex_unlock(&lock_oven);
    rc = pthread_cond_broadcast(&cond_oven); //We use broadcast instead of signal here because the ovens that we free up are variable.

    (*pack)(order, seed);
}

void *pack(struct Order *order, unsigned int *seed){

    int rc;
    struct timespec time;

//    printf("Order No.%d needs to be packaged.\n", order->id);

    rc = pthread_mutex_lock(&lock_pack);
    while (res_pack == 0) {
//	    printf("Order No.%d on hold... packing speciallist busy.\n", order->id);
	    rc = pthread_cond_wait(&cond_pack, &lock_pack);
    }
//	printf("Now packing order No.%d\n", order->id);
    res_pack--;
    rc = pthread_mutex_unlock(&lock_pack);

    sleep((order->pizzas) * T_PACK); //not 6-pack... pizza-pack

    rc = pthread_mutex_lock(&lock_pack);
    clock_gettime(CLOCK_REALTIME, &time);
    printf("Order No.%d packed successfully in %4.2f Minutes\n", order->id, (float)(time.tv_sec - order->start) / 60.0f);

    res_pack++;
    rc = pthread_mutex_unlock(&lock_pack);
    rc = pthread_cond_signal(&cond_pack);

    (*deliver)(order, seed);
}

void *deliver(struct Order *order, unsigned int *seed){

    int rc;
    struct timespec time;
//    printf("Order No.%d needs to be delivered.\n", order->id);

    rc = pthread_mutex_lock(&lock_deli);
    while (res_deli == 0) {
//	    printf("Order No.%d on hold... delivery boy is messing around.\n", order->id);
	    rc = pthread_cond_wait(&cond_deli, &lock_deli);
    }
//	printf("Now delivering order No.%d\n", order->id);
    res_deli--;
    rc = pthread_mutex_unlock(&lock_deli);

    sleep((rand_r(seed) % T_DELI_HIGH - 4) + T_DELI_LOW);

    rc = pthread_mutex_lock(&lock_deli);
    clock_gettime(CLOCK_REALTIME, &time);
    order->end = time.tv_sec;
    printf("Order No.%d delivered successfully in %4.2f Minutes\n", order->id, (float)(order->end - order->start) / 60.0f);
    res_deli++;
    rc = pthread_mutex_unlock(&lock_deli);
    rc = pthread_cond_signal (&cond_deli);
}

int main(int argc, char **argv)  {

    
    if(atoi(argv[1]) == 0 || atoi(argv[2]) == 0 ||
       atoi(argv[1]) < 0  || atoi(argv[2]) < 0) {
        printf("ERROR: invalid arguments for the threads and/or the starting seed!\n(Hint: ./file amountOfThreads startingSeed)\n");
        return -1;
    }
    
//    printf("print1 -- before assigning total threads and seed\n");
    int totalThreads = atoi(argv[1]);
    startSeed = atoi(argv[2]);
    id = (int *)malloc(totalThreads * sizeof(int*));
    int seed = startSeed;
//    printf("print2 -- total threads and seed assigned\n");
    int rc;
    pthread_t threads[totalThreads];

    pthread_mutex_init(&lock_tel, NULL);
    pthread_mutex_init(&lock_cook, NULL);
    pthread_mutex_init(&lock_oven, NULL);
    pthread_mutex_init(&lock_pack, NULL);
    pthread_mutex_init(&lock_deli, NULL);
    pthread_cond_init(&cond_tel, NULL);
    pthread_cond_init(&cond_cook, NULL);
    pthread_cond_init(&cond_oven, NULL);
    pthread_cond_init(&cond_pack, NULL);
    pthread_cond_init(&cond_deli, NULL);

    for (int i = 0; i < totalThreads; i++) {
	    id[i] = i+1;
	    printf("Main: Customer No.%d is dialling...\n", id[i]);
        rc = pthread_create(&threads[i], NULL, call, &id[i]);
        sleep((rand_r(&seed) % T_ORDER_HIGH) + T_ORDER_LOW); //call delay between different customers
    }
//    printf("print3 -- finished creating all the threads\n");
    for (int i = 0; i < totalThreads; i++) {
//        printf("----trying to join thread No.%d----\n", i+1);
	    rc = pthread_join(threads[i], NULL);
        if(rc != 0) {
            printf("----failed the join thread No.%d----\n", i+1);
        }
    }
//    printf("print4 -- JOINED all the threads\n");
    printf("\nTotal income: %d\n", sum_cash);
    printf("Total Orders: \n Successful: %d -- Failed: %d\n", sum_succcess, sum_fail);
    printf("Hold Time:\n Average: %4.2f -- Longest: %4.2f\n", (float)((avg_callHoldTime / 60.0f) / (sum_fail + sum_succcess)), (float)(max_callHoldTime / 60.0f));
    printf("Service Time:\n Average: %4.2f -- Longest: %4.2f\n", (float)((avg_serviceTime  / 60.0f)  / sum_succcess), (float)(max_serviceTime  / 60.0f));
    printf("Cooling Time:\n Average: %4.2f -- Longest: %4.2f\n", (float)((avg_coolingTime  / 60.0f)  / sum_succcess), (float)(max_coolingTime  / 60.0f));

    pthread_mutex_destroy(&lock_tel);
    pthread_mutex_destroy(&lock_cook);
    pthread_mutex_destroy(&lock_oven);
    pthread_mutex_destroy(&lock_pack);
    pthread_mutex_destroy(&lock_deli);
    pthread_cond_destroy(&cond_tel);
    pthread_cond_destroy(&cond_cook);
    pthread_cond_destroy(&cond_oven);
    pthread_cond_destroy(&cond_pack);
    pthread_cond_destroy(&cond_deli);
    //printf("print5\n");
    free(id);
    return 0;
}
