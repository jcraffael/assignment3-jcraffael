/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/slab.h>
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
    /**
    * TODO: implement per description
    */
	int index = 0;
	struct aesd_buffer_entry *entryptr = NULL;
	char_offset++;
	AESD_CIRCULAR_BUFFER_FOREACH(entryptr,buffer,index) {
		if(entryptr == NULL)
			return NULL;
       	if(char_offset > entryptr->size)
	   	{
			char_offset -= entryptr->size;
	   	}else{
			*entry_offset_byte_rtn = char_offset-1;
			return entryptr;
	   	}
	}
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
bool aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
	if(add_entry == NULL)
		return false;
	char *entry_buf = kmalloc(add_entry->size, GFP_KERNEL);
	if(/*ptr == NULL || */entry_buf == NULL)
	{
		return false;
	}	
	
	memcpy(entry_buf, add_entry->buffptr, add_entry->size);
	if(!buffer->full)
	{
		(buffer->entry)[buffer->in_offs].size = add_entry->size;
		(buffer->entry)[buffer->in_offs].buffptr = entry_buf;
		printk("In added new entry with buf %s and size %ld, and in_offs %d\n", add_entry->buffptr, add_entry->size, buffer->in_offs);
		if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)
		{
			buffer->in_offs = 0;
			buffer->full = true;
		}else{
			buffer->in_offs++;
		}
	}else{
#ifdef __KERNEL__		
		if((buffer->entry)[buffer->in_offs].buffptr != NULL)
			kfree((buffer->entry)[buffer->in_offs].buffptr);
		printk("Freed old data and to hold new one with in_offs %d and out_offs %d\n", buffer->in_offs, buffer->out_offs);
#endif
		(buffer->entry)[buffer->in_offs].size = add_entry->size;
		(buffer->entry)[buffer->in_offs].buffptr = entry_buf;
		if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)
        {
            buffer->in_offs = 0;
        }else
            buffer->in_offs++;

		if(buffer->out_offs  == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)
    		buffer->out_offs = 0;
		else
    		buffer->out_offs++;
	}
    return true;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
