/**
 * TieMa(Tiny&Mini) Memory Cached 
 * Copyright (C) 2008 heiyeluren. All rights reserved.
 *
 * tmcached is a mini open-source cache daemon, mainly used in dynamic data cache.
 *  
 * Use and distribution licensed under the BSD license.  See
 * the LICENSE file for full text.
 *
 * To learn more open-source code, visit: http://heiyeluren.googlecode.com
 * My blog: http://blog.csdn.net/heiyeshuwu
 *
 * $Id: tm_hash.h 2008-6-30, 2008-7-6 22:19 heiyeluren $
 */


/**
 * Hash data struct define
 *
 */

/* Hash data item struct */
struct tm_hash_entry_t {
	char *key;						/* data key string */
	char *data;						/* data value string */
	size_t length;					/* data length */
	unsigned created;				/* data create time (Unix Timestamp) */
	unsigned expired;				/* data expire time (Unix Timestamp) */
	struct tm_hash_entry_t *next;	/* key conflict link next data node pointer */
};

/* Hash table struct */
struct tm_hash_t {
	struct tm_hash_entry_t **table;	/* hash table list (hash data struct array pointer) */
	unsigned size;					/* table size */
	unsigned total;					/* current item total */
};




/**
 * Hash function define
 *
 */

/* Hash core function */
unsigned tm_hash( const char *str, unsigned table_size );

/* Create hash table */
struct tm_hash_t *tm_hcreate( unsigned table_size );

/* Insert a recored to hash talbe */
status tm_hinsert( struct tm_hash_t *hashtable, char *key, char *data, unsigned length, unsigned lifetime, short mode ); 

/* Find special key data item */
struct tm_hash_entry_t *tm_hfind( struct tm_hash_t *hashtable, char *key );

/* Delete special key and data */
status tm_hremove( struct tm_hash_t *hashtable, char *key );

/* Destroy hash table */
status tm_hdestroy( struct tm_hash_t *hashtable );

/* Visit hashtable every item */
void tm_hvisit( struct tm_hash_t *hashtable );

