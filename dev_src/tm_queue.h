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
 * $Id: tm_queue.h 2008-7-6, 2008-7-8 23:01 heiyeluren $
 */


/**
 * Queue data struct define
 *
 */

 /* Queue node */
struct tm_queue_node_t {
	char *key;						/* key name */
	unsigned data_len;				/* key data length */
	struct tm_queue_node_t *next;	/* next node pointer */
};

/* Queue list */
struct tm_queue_t {
	unsigned total;					/* queue all node total */
	unsigned memsize;				/* queue all node use memory size */
	struct tm_queue_node_t *front;	/* queue front pointer */
	struct tm_queue_node_t *rear;	/* queue rear pointer */
};




/**
 * Queue function define
 *
 */

/* Initialize (create) queue list */
struct tm_queue_t *tm_qcreate();

/* Check queue is empty */
status tm_qempty( struct tm_queue_t *queuelist );

/* Get queue list length */
unsigned tm_qlength( struct tm_queue_t *queuelist );

/* Get queue list memory size bytes */
unsigned tm_qmemsize( struct tm_queue_t *queuelist );

/* Push a node into the bottom of queue */
status tm_qentry( struct tm_queue_t *queuelist, char *key, unsigned data_len );

/* Pop a node the top of queue */
struct tm_queue_node_t *tm_qremove( struct tm_queue_t *queuelist );

/* Destroy queue list */
status tm_qdestroy( struct tm_queue_t *queuelist );

/* Visit queue list */
void tm_qvisit( struct tm_queue_t *queuelist );




