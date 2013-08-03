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
 * $Id: tmcached.h 2008-7-10, 2008-8-3 18:01 heiyeluren $
 */


/**
 * tmcached status struct
 */
struct tm_status {
	char version[32];		/* version */
	pid_t pid;				/* pid */
	time_t start_time;		/* start time (Unix timestamp) */
	time_t run_time;		/* run time (Unix timestamp) */
	unsigned mem_total;		/* allow use memory total size (Byte) */
	unsigned mem_used;		/* used memory size (byte) */
	unsigned item_total;	/* all item total */
	unsigned visit_add;		/* store visit total */
	unsigned visit_del;		/* remove visit total */
	unsigned visit_get;		/* fetch visit total */
};


/**
 * Status function define
 */

/* Print status*/
void print_status();

/* Init status */
void init_status( unsigned max_memsize );

/* Setting status */
void set_status( unsigned mem_used, unsigned item_total, unsigned visit_add, unsigned visit_del, unsigned visit_get );

/* Get status */
struct tm_status *get_status();

/* Get memory use size */
unsigned get_mem_used();

/* Fetch status */
struct tm_status *fetch_status();


/**
 * Data operate function define
 */

/* Remove data */
status remove_data( char *key );

/* Flush expired data probability (Garbage Collection) */
status get_gc_probability(unsigned probaility, unsigned divisor);

/* Flush all data */
status flush_data();

/* Flush all expired data */
status flush_expire_data();

/* Store data */
status store_data( char *key, char *data, unsigned length, unsigned lifetime, short mode );

/* Fetch data */
struct tm_hash_entry_t *fetch_data( char *key );


/**
 * Process function define 
 */

/* usage message */
void usage(char *exefile);

/* Output environment and configure information */
void print_config();

/* Send message to client */
void send_msg(int client_sock, char *msg, int length);

/* Send error messgae to client */
void send_error(int client_sock, short error_type, char *msg);

/* Process client request */
int proc_request( int client_sock );

/* Handle a client */
//void handle_client( int client_sock);

/* Initialize server socket listen */
static void init_server_listen( unsigned int port, unsigned int max_client );

/* Parse cmd options */
int parse_options( int argc, char *argv[] );


