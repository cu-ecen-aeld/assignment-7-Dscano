/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation

 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t cumulative_size = 0;
    uint8_t index = buffer->out_offs;
    uint8_t count = 0;

    if (buffer == NULL || entry_offset_byte_rtn == NULL) {
        return NULL;
    }

    while (count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
        struct aesd_buffer_entry *entry = &buffer->entry[index];

        if (entry->size == 0) {
            return NULL; // No more valid entries
        }

        if (cumulative_size + entry->size > char_offset) {
            *entry_offset_byte_rtn = char_offset - cumulative_size;
            return entry;
        }

        cumulative_size += entry->size;
        index = (index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        count++;

        if (index == buffer->in_offs && !buffer->full) {
            break; // Reached the end of valid entries
        }
    }

    return NULL; // char_offset not found
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if (buffer == NULL || add_entry == NULL) {
        return;
    }

    // Add the new entry at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;

    // Advance the in_offs index
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // If the buffer was already full, advance the out_offs index
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Check if the buffer is now full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
    } else {
        buffer->full = false;
    }
}

size_t aesd_circular_buffer_size(struct aesd_circular_buffer *buffer)
{
    size_t size = 0;

    for (size_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        struct aesd_buffer_entry entry = buffer->entry[i];
        if (!entry.buffptr)
        {
            break; 
        }
        else
        {
            size += entry.size;
        }
    }

    return size;
}

size_t aesd_circular_write_cmd_size(struct aesd_circular_buffer *buffer)
{
    size_t size = 0;

    for (size_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        struct aesd_buffer_entry entry = buffer->entry[i];
        if (!entry.buffptr)
        {
            break; 
        }
        else
        {
            size++;
        }
    }

    return size;
}

extern size_t aesd_circular_calculate_cmd_offset(
    struct aesd_circular_buffer *buffer, uint32_t write_cmd)
{
    size_t size = 0;

    if (write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        return size;
    }

    for (size_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        struct aesd_buffer_entry entry = buffer->entry[i];
        if (!entry.buffptr || i == write_cmd)
        {
            break; 
        }
        else
        {
            size += entry.size;
        }
    }

    return size;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}