#include <stdio.h>
#include "bit_queue.h"

int main()
{
    bit_queue_t * bq;
    uint16_t buffer = 0xaaaa;
    uint16_t res;
    bq = bit_queue_init((uint8_t*)&buffer, 2, false);
    bit_queue_read_bits(bq, (uint8_t*)&res, 8);
    printf("m1 = %d\n", res);
    res = 0;
    bit_queue_read_bits(bq, (uint8_t*)&res, 5);
    printf("m2 = %d\n", res);
    res = 0;
    bit_queue_read_bits(bq, (uint8_t*)&res, 1);
    printf("m3 = %d\n", res);
    bit_queue_destroy(bq);
    return 0;
}