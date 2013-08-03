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
 * $Id: tm_hash.h 2008-6-30, 2008-7-6 22:20 heiyeluren $
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include "tm_global.h"
#include "tm_hash.h"

/**
 * Hash core function
 *
 * @desc times 33 algorithm 
 */
unsigned tm_hash( const char *str, unsigned table_size ){
	unsigned long hash = 5381; 
	int c;
	while (c = *str++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	hash = table_size > 0 ? hash % table_size : hash;
	return hash;
}


/**
 * Create hash table
 *
 */
struct tm_hash_t *tm_hcreate( unsigned table_size ){
	struct tm_hash_t *hashtable;
	int i;

	if ( table_size < 1 ){
		return NULL;
	}
	if ( (hashtable = malloc( sizeof( struct tm_hash_t ) )) == NULL ){
		return NULL;
	}
	if ( (hashtable->table = malloc( table_size * sizeof( struct tm_hash_entry_t * ) )) == NULL ){
		return NULL;
	}
	/* initialize hash table */
	for ( i = 0; i < table_size; i++ ){
		hashtable->table[i] = NULL;
	}
	hashtable->size = table_size;
	hashtable->total = 0;

	return hashtable;
}

/**
 * Insert a recored to hash talbe
 *
 * @desc mode is MODE_SET, MODE_ADD, MODE_REPLACE
 */
status tm_hinsert( struct tm_hash_t *hashtable, char *key, char *data, unsigned length, unsigned lifetime, short mode ){
	unsigned hash;
	struct tm_hash_entry_t *entry, *tmpentry;
	time_t currtime;

	hash = tm_hash( key, hashtable->size - 1 );

	if (hash < 0 || hash >= hashtable->size ){
		return ERROR;
	}

	/* Check key is exist */
	if ( tm_hfind(hashtable, key) != NULL){
		if ( mode == MODE_ADD ){
			return SUCCESS;			
		} 
        else {
			tm_hremove( hashtable, key );
		}
	}

	/* Alloc hash entry memory */
	if ( (entry = malloc( sizeof( struct tm_hash_entry_t ) ) ) == NULL ){
		return ERROR;
	}
	currtime = time( (time_t *)NULL );

	/* Pad struct data */
	entry->key = key;
	entry->data = data;
	entry->length = length;
	entry->created = currtime;
	entry->expired = currtime + lifetime; /* ( lifetime > MAX_LIFE_TIME ? MAX_LIFE_TIME : lifetime ); */
    entry->next = hashtable->table[hash] != NULL ? hashtable->table[hash] : NULL;

	hashtable->table[hash] = entry;

	return OK;
}

/**
 * Find special key data item 
 *
 */
struct tm_hash_entry_t *tm_hfind( struct tm_hash_t *hashtable, char *key ){
	unsigned hash;
	struct tm_hash_entry_t *entry;

	hash = tm_hash( key, hashtable->size - 1 );

	if (hash < 0 || hash >= hashtable->size ){
		return NULL;
	}
	/* Find data */
	for ( entry = hashtable->table[hash]; entry != NULL; entry = entry->next ){
		if (strcmp(entry->key, key) == 0){
			return entry;
        }
	}
	return NULL;
}

/**
 * Delete special key and data 
 *
 */
status tm_hremove( struct tm_hash_t *hashtable, char *key ){
	unsigned hash;
	struct tm_hash_entry_t *entry, *tmpentry;

	hash = tm_hash( key, hashtable->size - 1 );
	entry = hashtable->table[hash];

	/* Process key is first node */
	if ( entry && strcmp(entry->key, key) == 0 ){
		tmpentry = entry->next;
		//free( entry->key );
		//free( entry->data) );
		free(entry);
		hashtable->table[hash] = tmpentry;

		return OK;
	} 

	/* Process key not first node */
    else {
		while ( entry ){
			if ( entry->next && strcmp(entry->next->key, key) == 0 ){
				tmpentry = entry->next;
				entry->next = entry->next->next;
				//free(tmpentry->key);
				//free(tmpentry->data);
				free(tmpentry);

				return OK;
			}
			entry = entry->next;
		}
	}

	return ERROR;
}

/**
 * Destroy hash table 
 *
 */
status tm_hdestroy( struct tm_hash_t *hashtable ){
	int i;
	struct tm_hash_entry_t *entry, *prev;

	/* Remove every hash item */
	for ( i = 0; i < hashtable->size; i++ ){

		/* Remove item self and every overflow bucket */
		entry = hashtable->table[i];
		while ( entry ){
			prev = entry; 
			entry = entry->next;
			//free(prev->key);
			//free(prev->data);
			free(prev);
		}
	}
	free( hashtable->table );
	free( hashtable );

	return OK;
}

/**
 * Debug: access every item 
 *
 */
void tm_hvisit( struct tm_hash_t *hashtable ){
	int i;

	assert( hashtable != NULL );
	printf("\n[Debug]\nVisit hashtable start .. \n");
	for (i=0; i<hashtable->size; i++){
		if (hashtable->table[i] != NULL){
			printf("Item %d not null, key:%s data:%s addr:%d\n", i, hashtable->table[i]->key, hashtable->table[i]->data, hashtable->table[i]);
		}
	}
	printf("Visit hashtable done.\n\n");
}

/**
 * Debug: print a hash node
 *
 */
status tm_hprint(struct tm_hash_entry_t *node){
    if (node == NULL){
        printf("[Debug]Hash Node Print: the node is null");
        return ERROR;
    }
	printf("\n[Debug]Hash Node Print: key:%s, data:%s, lenght:%d, created:%d, expired:%d\n", 
            node->key, node->data, node->length, node->created, node->expired);
}



