/**
 * @file bit_queue.c
 * @author amitfr1
 * @brief This is an adt for bit queuing
 * @version 0.1
 * @date 2021-12-11
 * 
 * @ingroup bit_queue
 * 
 */
#include <stdlib.h>
#include <errno.h>
#include "bit_queue.h"

/**
 * @brief The number of bits in a byte
 */
#define BITS_IN_BYTE 8

/**
 * @brief This is the mask of a byte
 */
#define BYTE_MASK 0x000000ff

/**
 * @brief This define calculates the mask its shifted the the end of the byte
 */ 
#define CREATE_BYTE_MASK(bit_offset) ((BYTE_MASK << bit_offset) & BYTE_MASK)
/**
 * @brief This define calculates the mask and it starts from the LSB
 */
#define CREATE_BYTE_MASK_LSB(bit_offset) (CREATE_BYTE_MASK(bit_offset) >> bit_offset)

/**
 * @brief This stuct holds all the fields used in the bit queue
 */
struct _bit_queue_t
{
    uint8_t * buffer; /// The buffer that holds all of the data
    uint8_t bit_offset; /// An index used to follow the bit progression in a byte
    size_t byte_offset; /// An index used to follow byte progression
    size_t written_bits; /// The number of bits that hold data in the buffer
    size_t buffer_size; /// The buffer size in bits
    bool free_buff;
};

/**
 * @brief This function checks if there is enough space to write all of the bits
 * 
 * Sets errno to EINVAL if bq = NULL
 * 
 * @param bq The bit queue
 * @param bit_count The number of bits we want to write
 * @return true if there is sufficient space in the queue false otherwise 
 */
static bool bit_queue_has_space(bit_queue_t *bq, size_t bit_count);

/**
 * @brief This function checks if there is enough data to read
 * 
 * Sets errno to EINVAL if bq = NULL
 * 
 * @param bq The bit queue
 * @param bit_count The number of bits we want to read
 * @return true if there is sufficient data in the queue false otherwise 
 */
static bool bit_queue_has_data(bit_queue_t *bq, size_t bit_count);

bit_queue_t * bit_queue_base_init(size_t byte_count)
{
    bit_queue_t * bq = NULL;
    if (!byte_count)
    {
        errno = EINVAL;
    }
    else if (!(bq = calloc(1, sizeof(struct _bit_queue_t))))
    {
        // errno is set by calloc and bq = NULL
    }
    else if (!(bq->buffer = calloc(byte_count, sizeof(uint8_t))))
    {
        // errno is set by calloc and bq->buffer = NULL
        free(bq);
        bq = NULL;
    }
    else
    {
        bq->buffer_size = byte_count;
        bq->written_bits = 0;
        bq->free_buff = true;
    }
    return bq;
}

bit_queue_t * bit_queue_init(uint8_t * buffer, size_t byte_count, bool free_buff)
{
    bit_queue_t * bq = NULL;
    if (!byte_count || buffer == NULL)
    {
        errno = EINVAL;
    }
    else if (!(bq = calloc(1, sizeof(struct _bit_queue_t))))
    {
        // errno is set by calloc and bq = NULL
    }
    else
    {
        bq->buffer = buffer;
        bq->buffer_size = byte_count;
        bq->written_bits = byte_count * BITS_IN_BYTE;
        bq->free_buff = free_buff;
    }
    return bq;
}

int bit_queue_read_bits(bit_queue_t *bq, uint8_t *buffer, size_t bit_count)
{
    int ret_val = -1;
    size_t offset_bit_count;
    uint8_t b_bit_offset;
    size_t b_byte_offset;
    size_t r_bits, total_bits;
    if (bq == NULL || buffer == NULL || bit_count == 0)
    {
        errno = EINVAL;
    }
    else if (bq->buffer == NULL)
    {
        errno = EINVAL;
    }
    else if (bit_count > bq->buffer_size * BITS_IN_BYTE)
    {
        errno = EMSGSIZE;
    }
    else if (!bit_queue_has_data(bq, bit_count))
    {
        // ret_val allready set
        // !!! replace with a better errno
        errno = EAGAIN;
    }
    else
    {
        // set bit counters to 0
        b_bit_offset = 0;
        b_byte_offset = 0;

        // set our target bit count
        r_bits = bit_count;
        do
        {
            if (r_bits + b_bit_offset <= BITS_IN_BYTE && r_bits <= BITS_IN_BYTE - bq->bit_offset)
            {
                // we can read all the bits without needing to worry about crossing bytes
                offset_bit_count = BITS_IN_BYTE - r_bits;
            }
            // will we cross the buffer 
            else if (b_bit_offset >= bq->bit_offset)
            {
                // we will cross the buffer byte in the next copy
                offset_bit_count = b_bit_offset;
            }
            else // b_bit_offset < bq->bit_offset
            {
                // we will cross the bit queue buffer byte in the next copy
                offset_bit_count = bq->bit_offset;
            }
            // copy all the bits we can into the buffer
            // because the buffer and the bq buffer can be on diffrent bit offsets we need to shift it back and forth
            buffer[b_byte_offset] |= ((bq->buffer[bq->byte_offset] & (CREATE_BYTE_MASK_LSB(offset_bit_count) << bq->bit_offset)) >> bq->bit_offset) << b_bit_offset;

            // update the bit counters
            bq->bit_offset += (BITS_IN_BYTE - offset_bit_count);
            b_byte_offset += (BITS_IN_BYTE - offset_bit_count);
            r_bits -= (BITS_IN_BYTE - offset_bit_count);

            // check if we are crossing bytes
            if (bq->bit_offset == BITS_IN_BYTE)
            {
                bq->bit_offset = 0;
                bq->byte_offset++;
            }
            if (b_bit_offset == BITS_IN_BYTE)
            {
                bq->bit_offset = 0;
                b_byte_offset++;
            }
        } while (r_bits != 0);
        // update the bit queue and the retval
        bq->written_bits -= bit_count;
        ret_val = bit_count;
    }
    return ret_val;
}

int bit_queue_write_bits(bit_queue_t *bq, uint8_t *buffer, size_t bit_count)
{
    int ret_val = -1;
    if (bq == NULL || buffer == NULL || bit_count == 0)
    {
        errno = EINVAL;
    }
    else if (bq->buffer == NULL)
    {
        errno = EINVAL;
    }
    else if (bit_count > bq->buffer_size * BITS_IN_BYTE)
    {
        errno = EMSGSIZE;
    }
    else if (!bit_queue_has_space(bq, bit_count))
    {
        // ret_val allready set
    }
    else
    {
        // TODO: implement the write functionality
    }
    return ret_val;
}

int bit_queue_destroy(bit_queue_t *bq)
{
    int ret_val = -1;
    if (bq == NULL)
    {
        errno = EINVAL;
    }
    else if (bq->buffer == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        if (bq->free_buff)
        {
            free(bq->buffer);
        }
        bq->buffer = NULL;
        free(bq);
        ret_val = 0;
    }
    return ret_val;
}

// static functions

static bool bit_queue_has_space(bit_queue_t *bq, size_t bit_count)
{
    bool ret_val = false;
    if (bq == NULL)
    {
        errno = EINVAL;
    }
    else if ((bq->buffer_size * BITS_IN_BYTE) - bq->written_bits >= bit_count)
    {
        ret_val = true;
    }
    return ret_val;
}

static bool bit_queue_has_data(bit_queue_t *bq, size_t bit_count)
{
    bool ret_val = false;
    if (bq == NULL)
    {
        errno = EINVAL;
    }
    else if (bq->written_bits >= bit_count)
    {
        ret_val = true;
    }
    return ret_val;
}