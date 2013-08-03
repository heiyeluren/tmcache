/**
 * TieMa(Tiny&Mini) Memory Cached 
 * Copyright (C) 2008 heiyeluren. All rights reserved.
 *
 * tmcache is a mini open-source cache daemon, mainly used in dynamic data cache.
 *  
 * Use and distribution licensed under the BSD license.  See
 * the LICENSE file for full text.
 *
 * To learn more open-source code, visit: http://heiyeluren.googlecode.com
 * My blog: http://blog.csdn.net/heiyeshuwu
 *
 * $Id: tmcache.c 2008-7-10, 2008-8-29 15:56 heiyeluren $
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#define _GNU_SOURCE

#include <sys/select.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <event.h>

#include "tm_global.h"
#include "tm_common.h"
#include "tm_hash.h"
#include "tm_queue.h"
#include "tmcache.h"


/* default global configure */
unsigned short g_is_debug	= IS_DEBUG;
unsigned short g_is_daemon	= IS_DAEMON;
unsigned int g_port			= PORT;
unsigned int g_max_client	= MAX_CLIENTS;
unsigned int g_max_mem_size	= MAX_MEM_SIZE;

unsigned int g_max_tablesize = MAX_TABLE_SIZE;

/* global varibales */
static struct tm_hash_t *g_htable;
static struct tm_queue_t *g_qlist;
static struct tm_status *g_status;


/********************************
 *
 *   Status function
 *
 ********************************/

/**
 * Print status
 *
 */
void print_status(){
	char buf[8192];
	struct tm_status *status;
	int visit_total;

	status = g_status;
	visit_total = status->visit_add + status->visit_del + status->visit_get;

sprintf(buf, "=============Status===============\n\
STAT version %s\r\n\
STAT pid %d\r\n\
STAT start_time %d\r\n\
STAT run_time %d\r\n\
STAT mem_total %d\r\n\
STAT mem_used %d\r\n\
STAT item_total %d\r\n\
STAT visit_total %d\r\n\
STAT visit_add %d\r\n\
STAT visit_del %d\r\n\
STAT visit_get %d\r\n\
===============END================\n", 
status->version, status->pid, status->start_time, status->run_time, 
status->mem_total, status->mem_used, status->item_total, 
visit_total, status->visit_add, status->visit_del, status->visit_get);

		printf("%s", buf);
		//exit(0);
}


/** 
 * Init status
 *
 */
void init_status( unsigned max_memsize ){
	g_status = (struct tm_status *)malloc(sizeof(struct tm_status));
	memset(g_status, 0, sizeof(struct tm_status));
	//memset(g_status->version, 0, sizeof(g_status->version));
	sprintf(g_status->version, "%s", VERSION);
	g_status->pid			= getpid();	
	g_status->start_time	= time( (time_t *)NULL );
	g_status->run_time		= 0;
	g_status->mem_total		= max_memsize;
	g_status->mem_used		= 0;
	g_status->item_total	= 0;
	g_status->visit_add		= 0;
	g_status->visit_del		= 0;
	g_status->visit_get		= 0;
}

/**
 * Setting status
 *
 */
void set_status( unsigned mem_used, unsigned item_total, unsigned visit_add, unsigned visit_del, unsigned visit_get ){
	time_t currtime;

	//print_status();

	currtime = time((time_t *)NULL);
	g_status->run_time		= currtime - g_status->start_time;
	g_status->mem_used		+= mem_used;
	g_status->item_total	+= item_total;
	g_status->visit_add		+= visit_add;
	g_status->visit_del		+= visit_del;
	g_status->visit_get		+= visit_get;

	//print_status();
}

/**
 * Get status
 *
 */
struct tm_status *get_status(){
	time_t currtime;

	currtime = time((time_t *)NULL);
	g_status->run_time = currtime - g_status->start_time;
	return g_status;
	//status = g_status;
}

/** 
 * Get memory use size
 *
 */
unsigned get_mem_used(){
	unsigned ret;
	ret = g_status->mem_used;
	return ret;
}

/**
 * Fetch status
 *
 */
struct tm_status *fetch_status(){
	struct tm_status *status;
	status = g_status;

	return status;
}



/********************************
 *
 *   Data operate Function
 *
 ********************************/

/**
 * Store data
 *
 */
status store_data( char *key, char *data, unsigned length, unsigned lifetime, short mode ){
	status ret;
	
	printf("key:%s, data:%s, lenght:%d, lifetime:%d, mode:%d\n", key, data, length, lifetime, mode);
	//return 1;

	/* current memory use size exceed MAX_MEM_SIZE, remove last node from queue, remove key from hash table */
	if ( (get_mem_used() + length) > g_max_mem_size ){
		printf("Max?\n");
		struct tm_queue_node_t *qnode;
		while ( (get_mem_used() + length) > g_max_mem_size ){
			qnode = tm_qremove( g_qlist );
			remove_data( qnode->key );
		}
	}

	/* insert data to hashtable */
	ret = tm_hinsert( g_htable, key, data, length, lifetime, mode );
	printf("Store Result: %d\n", ret);
	
	/* use add method, if key exist return SUCCESS */
	if ( ret == SUCCESS || ret == ERROR ){
		return FALSE;
	}
    tm_hvisit(g_htable);

	/* set status */
	set_status( length, 1, 1, 0, 0 );

    /* gc expird data */
    if (get_gc_probability(GC_PROBAILITY, GC_DIVISOR) == TRUE){
        flush_expire_data(); 
    }
	
	return TRUE;
}


/**
 * Fetch data
 *
 */
struct tm_hash_entry_t *fetch_data( char *key ){
	struct tm_hash_entry_t *hnode;

	hnode = tm_hfind( g_htable, key );
    if (hnode == NULL){
        printf("[Find] key:%s Not Found.\n", key);
    }
    tm_hvisit(g_htable);
	set_status(0, 0, 0, 0, 1);

	return hnode;
}

/**
 * Remove data
 *
 */
status remove_data( char *key ){
	status ret;
	int length;
	struct tm_hash_entry_t *hnode;

	/* remove data from hash table, set status */
	if ( (hnode = tm_hfind( g_htable, key )) == NULL){
		return FALSE;
	}
	length = 0 - hnode->length;
	set_status( length, -1, 0, -1, 0 );	
	ret = tm_hremove( g_htable, key );
    tm_hvisit(g_htable);

	//free(hnode->key);
	//free(hnode->data);
	//free(hnode);

	return TRUE;
}



/**
 * Flush all data
 *
 */
status flush_data(){
	/* Destroy queue & hash table */
	tm_hdestroy( g_htable );
	tm_qdestroy( g_qlist );

	/* Init hash table & queue */
	g_htable = tm_hcreate( g_max_tablesize );
	g_qlist  = tm_qcreate();
	init_status( g_max_mem_size );	

	return TRUE;
}

/**
 * Flush all expired data
 *
 */
status flush_expire_data(){
    int i;
    time_t currtime;
    char *key;

    /* Remove expired (after date) data */
    currtime = time( (time_t *)NULL );
	for (i=0; i<g_htable->size; i++){
		if (g_htable->table[i] != NULL && g_htable->table[i]->expired < currtime){
            remove_data( g_htable->table[i]->key );
        }
	}
    return TRUE;
}


/**
 * Flush expired data probability (Garbage Collection)
 *
 * @desc probability big probaility increase, divisor big probaility decrease
 */
status get_gc_probability(unsigned probaility, unsigned divisor){
    int n;
    struct timeval tv; 

    gettimeofday(&tv , (struct timezone *)NULL);
    srand((int)(tv.tv_usec + tv.tv_sec));
    n = 1 + (int)( (float)divisor * rand() / (RAND_MAX+1.0) );
    return (n <= probaility ? TRUE : FALSE); 
}


/********************************
 *
 *   Process Function
 *
 ********************************/
 
/**
 * usage message
 *
 */
void usage(char *exefile){
	/* Print copyright information */
	fprintf(stderr, "#=======================================\n");
	fprintf(stderr, "# TieMa(Tiny&Mini) Memory Cached\n");
	fprintf(stderr, "# Version %s\n", VERSION);
	fprintf(stderr, "# \n");
	fprintf(stderr, "# heiyeluren <blog.csdn.net/heiyeshuwu>\n");
	fprintf(stderr, "#=======================================\n\n");
    fprintf(stderr, "usage: %s [OPTION] ... \n", exefile);

	/* Print options information */
	fprintf(stderr, "\nOptions: \n\
  -p <num>	port number to listen on,default 11211\n\
  -d		run as a daemon, default No\n\
  -m <num>	max memory to use for items in megabytes, default is 16M\n\
  -c <num>	max simultaneous connections, default is 1024\n\
  -v		print version information\n\
  -h		print this help and exit\n");

	/* Print example information */
	fprintf(stderr, "\nExample: \n  %s -p 11211 -m 16 -c 1024 -d\n", exefile);

}

/**
 * Output environment and configure information
 *
 */
void print_config(){
	fprintf(stderr, "===================================\n");
	fprintf(stderr, " tmcache Configure information\n");
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "Is-Debug\t = %s\n", g_is_debug ? "Yes" : "No");
	fprintf(stderr, "Is-Daemon\t = %s\n", g_is_daemon ? "Yes" : "No");
	fprintf(stderr, "Port\t\t = %d\n", g_port);
	fprintf(stderr, "Max-Client\t = %d\n", g_max_client);
	fprintf(stderr, "Max-MemSize\t = %d\n", g_max_mem_size);
	fprintf(stderr, "===================================\n\n");
}

/** 
 * Parse cmd options
 *
 */
int parse_options( int argc, char *argv[] ){
	int opt;
	struct option longopts[] = {
		{ "is-daemon",	0, NULL, 'd' },
		{ "port",		1, NULL, 'p' },
		{ "max-memsize",1, NULL, 'm' },
		{ "max-client",	1, NULL, 'c' },
		{ "help",		0, NULL, 'h' },
		{ "version",	0, NULL, 'v' },
		{ 0,			0, 0,	 0   }
	};

	/* Parse every options */
	while ( (opt = getopt_long(argc, argv, ":dp:m:c:hv", longopts, NULL)) != -1){
		switch (opt){
			case 'h': 
				usage(argv[0]); 
				return(-1);
				break;
			case 'v':
				fprintf(stderr, "%s %s\n", SERVER_NAME, VERSION);
				return(-1);
				break;
			case 'd': g_is_daemon = 1; break;
			case 'p':
				g_port = atoi(optarg);
				if ( g_port < 1 || g_port > 65535 ){
					fprintf(stderr, "Options -p error: input port number %s invalid, must between 1 - 65535.\n\n", optarg);
					return(-1);
				}
				break;
			case 'c':
				g_max_client = atoi(optarg);
				if ( !isdigit(g_max_client) ){
					fprintf(stderr, "Options -c error: input clients %s invalid, must number, proposal between 32 - 2048.\n\n", optarg);
					return(-1);
				}
				break;
			case 'm':
				if ( !is_numeric(optarg) || atoi(optarg) < 1 ){
					fprintf(stderr, "Options -m error: input memory size %s invalid, recommend 16 - 2048 (unit is MB, megabytes)\n\n", optarg);
					return(-1);
				}
				g_max_mem_size = atoi(optarg) * 1024 * 1024;
				break;
		}
	}

	return(0);
}


/**
 * Send message to client
 *
 */
void send_msg(int client_sock, char *msg, int length){
	int len;
	len = length>0 ? length : strlen(msg);
	write(client_sock, msg, len);
}

/**
 * Send error messgae to client
 *
 */
void send_error(int client_sock, short error_type, char *msg){
	char buf[BUFFER_SIZE];

	memset(buf, 0, BUFFER_SIZE);
	switch(error_type){
		case E_GENERAL:	
			sprintf(buf, "ERROR\r\n");break;
		case E_CLIENT:	
			sprintf(buf, "CLIENT_ERROR %s\r\n", msg);break;
		case E_SERVER:	
			sprintf(buf, "SERVER_ERROR %s\r\n", msg);break;
		default: 
			sprintf(buf, "ERROR\r\n");
	}
	send_msg(client_sock, buf, 0);
}


/**
 * Process client request
 *
 */
int proc_request( int client_sock ){//, struct sockaddr_in client_addr ){
    char head[BUFFER_SIZE], **head_arr, *method;
	int head_num;
	FILE *fp;

	/* Read & explode header */
	fp = fdopen(client_sock, "r");
	memset(head, 0, sizeof(head));
	if ( fgets(head, BUFFER_SIZE, fp) == NULL ){
		send_error(client_sock, E_SERVER, "not recv client message");
		return FALSE;
	}
	explode(head, ' ', &head_arr, &head_num);
	method  = trim( strtolower( head_arr[0] ) );

	/* debug message */
	if ( g_is_debug ){
		printf("Method: %s | Header: %s", method, head);
	}

	/**
	 * method: store 
	 * format: <cmd> <key> [<flag>] <expire> <length>\r\n
	 *		   cmd: set/add/replace
	 */
	if ( strcmp(method, "set")==0 || strcmp(method, "add")==0 || strcmp(method, "replace")==0 || strcmp(method, "append")==0 ){
		if ( head_num != 5 && head_num != 4 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		/* store variables */
		short mode;
		char *key, *data;
		unsigned length, lifetime;		

		/* pick every store item */
		if ( head_num == 5 ){
			lifetime = atoi(trim(head_arr[3]));
			length   = atoi(trim(head_arr[4]));
		} 
        else {
			lifetime = atoi(trim(head_arr[2]));
			length	 = atoi(trim(head_arr[3]));
		}
		
		/* data too large */
		if ( length > MAX_BUF_SIZE ){
			send_error(client_sock, E_CLIENT, "data too large, to exceed MAX_BUF_SIZE" );
			return FALSE;
		}

		/* item assign */
		key		 = head_arr[1];
		data	 = (char *)malloc(length + 1);
		lifetime = lifetime > MAX_LIFE_TIME ? MAX_LIFE_TIME : lifetime;
		mode	 = ( strcmp(method, "add")==0 || strcmp(method, "append")==0 ? MODE_ADD : 
				   ( strcmp(method, "replace")==0 ? MODE_REPLACE : MODE_SET ) );

		/* debug message */
		if ( g_is_debug ){
			printf("Store method, Key: %s | Length: %d | Lifetime: %d\n", key, length, lifetime);
		}

		/* read data */
		if ( fread(data, 1, length, fp) <= 0 ){
			send_error(client_sock, E_CLIENT, "data too short, not match LENGTH");
			return FALSE;
		}
		//printf("key:%s, data:%s, length:%d, lifetime:%d, mode:%d\n", key, data, length, lifetime, mode);
		//return 1;

		
		/*printf("fread: %s\n", data);
		return 0;

		
		if ( read(client_sock, data, length) != length){
			send_error(client_sock, E_CLIENT, "data too short, not match LENGTH");
			return FALSE;
		}
		*/

		/* store data */
		if ( store_data( key, data, length, lifetime, mode ) == TRUE ){
			send_msg(client_sock, "STORED\r\n", 0);
		} 
        else {
			send_msg(client_sock, "NOT_STORED\r\n", 0);
		}

		struct tm_hash_entry_t *node;
		node = tm_hfind(g_htable, key);
		printf("[Node info] key:%s, data:%s, length:%d, created:%d, expred:%d, next:%d\n", node->key, node->data, node->length, node->created, node->expired, node->next);


		return OK;
	}

	/**
	 * method: fetch 
	 * format: <cmd> <key>\r\n
	 *		   cmd: get/fetch
	 */
	else if ( strcmp(method, "get")==0 || strcmp(method, "fetch")==0 || strcmp(method, "gets")==0 ){
		if ( head_num != 2 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		char *key, buf[BUFFER_SIZE];
		struct tm_hash_entry_t *hnode;
		time_t currtime;

        printf("\nGET OPT FUNC-1\n");

		key = trim(head_arr[1]);
        printf("FIND KEY: %s\n", key);
		hnode = fetch_data( key );
        if ( hnode == NULL ){
			send_msg(client_sock, "END\r\n", 0);
            return FALSE;
        }
        printf("FIND KEY: %s  DATA: %s LENGTH: %d CREATED: %d EXPIRED: %d\n", hnode->key, hnode->data, hnode->length, hnode->created, hnode->expired);
        printf("\nGET OPT FUNC-2\n");

		/* item out expire time, remove it */
		currtime = time((time_t *)NULL);
		if ( hnode->expired < currtime ){
			remove_data( key );
			send_msg(client_sock, "END\r\n", 0);
			return FALSE;
		}
        printf("\nGET OPT FUNC-3\n");

		/* send data to client */
		memset(buf, 0, BUFFER_SIZE);
		sprintf(buf, "VALUE %s 0 %d\r\n", hnode->key, hnode->length);
		send_msg(client_sock, buf, 0);
		send_msg(client_sock, hnode->data, hnode->length);
		send_msg(client_sock, "\r\n", 0);
        printf("\nGET OPT FUNC-4\n");
		return TRUE;
	}

	/**
	 * method: delete 
	 * format: <cmd> <key> [<longtime>]\r\n
	 *		   cmd: delete/del/remove
	 */
	else if ( strcmp(method, "delete")==0 || strcmp(method, "del")==0 || strcmp(method, "remove")==0 || strcmp(method, "rm")==0){
		if ( head_num != 2 && head_num != 3 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		char *key;

		key = trim(head_arr[1]);
        tm_hvisit(g_htable);
        printf("Remove data.\n");
		if ( remove_data( key ) == FALSE ){
			send_msg(client_sock, "NOT_FOUND\r\n", 0);
			return FALSE;
		} 
        else {
			send_msg(client_sock, "DELETED\r\n", 0);
			return TRUE;
		}
	}
		
	/**
	 * method: flush_all 
	 * format: <cmd>\r\n
	 *		   cmd: flush_all/flush
	 */	
	else if ( strcmp(method, "flush_all")==0 || strcmp(method, "flush")==0 ){
		if ( head_num != 1 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		flush_data();
		send_msg(client_sock, "OK\r\n", 0);
		return TRUE;
	}
	
	/**
	 * method: stats 
	 * format: <cmd> [<option>]\r\n
	 *		   cmd: stats/state/stat/status
	 */	
	else if ( strcmp(method, "stats")==0 || strcmp(method, "state")==0 || strcmp(method, "stat")==0 || strcmp(method, "status")==0 ){
		if ( head_num != 1 && head_num != 2 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		char buf[BUFFER_SIZE], line[1024];
		struct tm_status *status;
		int visit_total;

		status = get_status();
		visit_total = status->visit_add + status->visit_del + status->visit_get;
		memset(buf, 0, BUFFER_SIZE);

		sprintf(buf, "\
STAT pid %d\r\n\
STAT start_time %d\r\n\
STAT run_time %d\r\n\
STAT version %s\r\n\
STAT mem_total %d\r\n\
STAT mem_used %d\r\n\
STAT item_total %d\r\n\
STAT visit_total %d\r\n\
STAT visit_add %d\r\n\
STAT visit_del %d\r\n\
STAT visit_get %d\r\n\
END\r\n", 
		status->pid, status->start_time, status->run_time, status->version, 
		status->mem_total, status->mem_used, status->item_total, 
		visit_total, status->visit_add, status->visit_del, status->visit_get);

		send_msg(client_sock, buf, 0);
		return TRUE;
	}

	/**
	 * method: version 
	 * format: <cmd>\r\n
	 *		   cmd: version/ver
	 */	
	else if ( strcmp(method, "version")==0 || strcmp(method, "ver")==0 ){
		if ( head_num != 1 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
		send_msg(client_sock, VERSION, 0);
		send_msg(client_sock, "\r\n", 0);
		return TRUE;
	}

	/**
	 * method: quit 
	 * format: <cmd>\r\n
	 *		   cmd: quit/exit
	 */	
	else if ( strcmp(method, "quit")==0 || strcmp(method, "exit")==0 ){
		if ( head_num != 1 ){
			send_error(client_sock, E_GENERAL, "");
			return FALSE;
		}
        return EXIT;
		//return TRUE;
	 }

	/**
	 * exception
	 */
	else {
		send_error(client_sock, E_CLIENT, "That method is not implemented");
		return FALSE;
	}
	/*
	return;

	while ( fgets(buf, MAX_BUF_SIZE, fp) != NULL){
		printf("%s ", buf);
	}
	printf("%s ", buf);
	return;
	*/

	//char **buf, **method_buf, **query_buf, currtime[32], cwd[1024], tmp_path[1536], pathinfo[512], path[256], file[256], log[1024];
	//int line_total, method_total, query_total, i;

	///* Split client request */
	//getdate(currtime);
	//explode(req, '\n', &buf, &line_total);

	///* Print log message  */
	//memset(log, 0, sizeof(log));
	//sprintf(log, "[%s] %s %s\n", currtime, inet_ntoa(client_addr.sin_addr), buf[0]);
	//WriteLog(log);

	///* Check request is empty */
	//if (strcmp(buf[0], "\n") == 0 || strcmp(buf[0], "\r\n") == 0){
	//	SendError( client_sock, 400, "Bad Request", "", "Can't parse request." );
	//}

	///* Check method is implement */
	//explode(buf[0], ' ', &method_buf, &method_total);
	//if ( strcmp( strtolower(method_buf[0]), "get") != 0 &&  strcmp( strtolower(method_buf[0]), "head") != 0 ){
	//	SendError( client_sock, 501, "Not Implemented", "", "That method is not implemented." );
	//}
	//explode(method_buf[1], '?', &query_buf, &query_total);

	///* Make request data */	
	//getcwd(cwd, sizeof(cwd));
	//strcpy(pathinfo, query_buf[0]);
	//substr(query_buf[0], 0, strrpos(pathinfo, '/')+1, path);
	//substr(query_buf[0], strrpos(pathinfo, '/')+1, 0, file);
	//
	///* Pad request struct */
	//memset(&request_info, 0, sizeof(request_info));
	//strcat(cwd, pathinfo);

	///* Is a directory pad default index page */
	//memset(tmp_path, 0, sizeof(tmp_path));
	//strcpy(tmp_path, cwd);
	//if ( is_dir(tmp_path) ){
	//	strcat(tmp_path, g_dir_index);
	//	if ( file_exists(tmp_path) ){
	//		request_info.physical_path = tmp_path;
	//	}
	//}
	//
	///* Debug message */
	//if ( g_is_debug ){
	//	fprintf(stderr, "[ Request ]\n");
	//	for(i=0; i<line_total; i++){
	//		fprintf(stderr, "%s\n", buf[i]);
	//	}
	//}

	///* Process client request */
	//proc_request( client_sock, client_addr, request_info );

	//return 0;
}



/**
 * Handle a client
 *
 */
void handle_client_event( int client_sock, short event, void *arg ){//, struct sockaddr_in client_addr ){
    int nread;
    int ret;
	//ret = proc_request( client_sock);//, client_addr );
    //printf("proc_requrest result %d\n", ret);

    while(1){
        /* Not read data */
        ioctl(client_sock, FIONREAD, &nread);
        if ( nread > 0 ){
            ret = proc_request( client_sock);//, client_addr );
            if (ret == EXIT){
                break;
            }
            printf("proc_requrest result %d\n", ret);
        }
        //sleep(1);
    }
}

/**
 * Handle a client
 *
 */
void handle_client( int client_sock){//, struct sockaddr_in client_addr ){
    int nread;
    int ret;
	//ret = proc_request( client_sock);//, client_addr );
    //printf("proc_requrest result %d\n", ret);

    while(1){
        /* Not read data */
        ioctl(client_sock, FIONREAD, &nread);
        if ( nread > 0 ){
            ret = proc_request( client_sock);//, client_addr );
            if (ret == EXIT){
                break;
            }
            printf("proc_requrest result %d\n", ret);
        }
        //sleep(1);
    }
}



/**
 * Epoll I/O Multiplexing
 *
 */
void tm_epoll( int serversock, unsigned int max_client ){
    struct sockaddr_in client_addr;
    unsigned int clientlen;
    struct epoll_event ev,events[20];
	int clientsock, epollfd, i, eventnum;
    char currtime[32];


    epollfd = epoll_create(max_client);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = serversock;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, serversock, &ev);
   
    /* Run until cancelled */
    while (1){
      eventnum = epoll_wait(epollfd, events, 20, -1);
    
	  //fprintf(stdout, "[%d] events Waiting client connection ...\n", eventnum);
      if(eventnum>0) {
            memset(currtime, 0, sizeof(currtime));
            getdate(currtime);
      }
      for(i=0; i<eventnum; i++) {
		//fprintf(stdout,"enter listening\n");
        if(events[i].data.fd == serversock){
           clientlen = sizeof(client_addr);
           /* Wait for client connection */
           if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0){
               //die("Failed to accept client connection");
               continue;
           }
           socket_set_nonblock(clientsock);
           ev.events = EPOLLIN | EPOLLET;
           ev.data.fd = clientsock;
           epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);
        } else {
		  //fprintf(stdout,"enter client\n");
          clientlen = sizeof(client_addr);
          clientsock = events[i].data.fd;
          getpeername(clientsock,(struct sockaddr *) &client_addr, &clientlen);
          //handle_client(clientsock);//, client_addr);
          epoll_ctl(epollfd, EPOLL_CTL_DEL, clientsock, &ev);
       } 
      }
    }
}


/**
 * Select I/O Multiplexing
 *
 */
void tm_select( int serversock, unsigned int max_client ){
    int clientsock;
    struct sockaddr_in client_addr;
	char currtime[32];
    fd_set readfds, optfds;
    unsigned clientlen, fd_maxsize;

    fd_maxsize = serversock;
    FD_ZERO(&readfds);
    FD_SET(serversock, &readfds);

    /* Run until cancelled */
    while(1) {
        int fd;
        int nread;
        int result;
        optfds = readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000000;

        FD_ZERO(&readfds);
        /* Select availably file descriptor */
        if ( (result = select(fd_maxsize+1, &optfds, (fd_set *)0, (fd_set *)0, &timeout) ) < 0 ) {
            die("Failed select socket");
        }
        printf("select result: %d\n", result);

        /* Traversal file descriptor list find availably fd */
        for(fd = 0; fd < fd_maxsize+1; fd++) {
            printf("for FD: %d\n", fd);
            if(FD_ISSET(fd, &optfds)) {

                //Fd is server socekt so accept a new connection
                if(fd == serversock) {
                    clientlen = sizeof(client_addr);
                    memset(currtime, 0, sizeof(currtime));
                    getdate(currtime);

                    if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0) {
                        die("Failed to accept client connection");
                    }
                    FD_SET(clientsock, &readfds);
                    fd_maxsize = clientsock;
                    printf("accept new fd & fd_maxsize: %d\n", clientsock);
                }

                //Availably fd read client request 
                else {
                    ioctl(fd, FIONREAD, &nread);
                    if(nread == 0) {
                        close(fd);
                        FD_CLR(fd, &readfds);
                        printf("FD_CLR fd %d\n", fd);
                    }
                    else {
                        //handle_client(clientsock, client_addr);
                        //fd_maxsize--;
                        printf("handle client %d\n", fd);
                    }
                }
            }
        }
    }
	
}

/**
 * Select I/O Multiplexing
 *
 */
void tm_select3( int serversock, unsigned int max_client ){
    int clientsock;
    struct sockaddr_in client_addr;
    char currtime[32];
    fd_set readfds, optfds;
    unsigned clientlen, fd_maxsize;

    fd_maxsize = serversock;
    FD_ZERO(&readfds);
    FD_SET(serversock, &readfds);
    printf("server sock:%d\n", serversock);

    /* Run until cancelled */
    while(1) {
        int fd;
        int nread;
        int result;
        optfds = readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(serversock, &readfds);
        /* Select availably file descriptor */
        if ( (result = select(fd_maxsize+1, &readfds, (fd_set *)0, (fd_set *)0, &timeout) ) <= 0 ) {
            continue;
        }
        printf("select result: %d\n", result);

        /* Traversal file descriptor list find availably fd */
        for(fd = 0; fd < fd_maxsize+1; fd++) {
            printf("for FD: %d\n", fd);
            if(FD_ISSET(fd, &readfds)) {
                if (fd < 3){
                    continue;
                }
                if (fd == serversock){
                    clientlen = sizeof(client_addr);
                    memset(currtime, 0, sizeof(currtime));
                    getdate(currtime);

                    if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0) {
                        die("Failed to accept client connection");
                    }
                    FD_SET(clientsock, &readfds);
                    fd_maxsize = clientsock;
                    printf("accept new fd %d & fd_maxsize: %d\n", clientsock, fd_maxsize);
                } 
                else {
                    ioctl(fd, FIONREAD, &nread);
                    if(nread != 0) {
                        close(fd);
                        FD_CLR(fd, &readfds);
                        printf("FD_CLR fd %d\n", fd);
                    }
                    else {
                        //handle_client(clientsock, client_addr);
                        //fd_maxsize--;
                        printf("handle client %d\n", fd);
                    }
                    
                }
            }
        }


        /*

        if ( serversock != -1 && FD_ISSET( serversock, &readfds ) ){
            if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0) {
                perror("accept");
                continue;
            }
        }
        fd_maxsize = clientsock;
        handle_client(clientsock, client_addr);

        FD_CLR(clientsock, &readfds);
        FD_CLR(serversock, &readfds);
        close(clientsock);
        */
    }
}


/**
 * Select I/O Multiplexing
 *
 */
void tm_select2( int serversock, unsigned int max_client ){
    int clientsock;
    struct sockaddr_in client_addr;
    char currtime[32];
    fd_set readfds, optfds;
    unsigned clientlen, fd_maxsize;

    fd_maxsize = serversock;
    FD_ZERO(&readfds);
    FD_SET(serversock, &readfds);
    printf("server sock:%d\n", serversock);

    /* Run until cancelled */
    while(1) {
        int fd;
        int nread;
        int result;
        optfds = readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        clientlen = sizeof(client_addr);

        FD_ZERO(&readfds);
        FD_SET(serversock, &readfds);
        /* Select availably file descriptor */
        if ( (result = select(fd_maxsize+1, &readfds, (fd_set *)0, (fd_set *)0, &timeout) ) <= 0 ) {
            continue;
        }
        printf("select result: %d\n", result);

        if ( serversock != -1 && FD_ISSET( serversock, &readfds ) ){
            if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0) {
                perror("accept");
                continue;
            }
        }
        fd_maxsize = clientsock;
        //handle_client(clientsock, client_addr);

        FD_CLR(clientsock, &readfds);
        FD_CLR(serversock, &readfds);
        close(clientsock);
    }
}


/**
 * Thread callback function
 *
 */
void *thread_function(void *arg) {
    int *clientsock;
    clientsock = (int *)arg;
    //handle_client(*clientsock);//, (struct sockaddr_in *)NULL);
    //close(*clientsock);
    printf("thread close socket id: %d\n", *clientsock);
    printf("thread exit\n");
    pthread_exit(NULL);
    return NULL;
}


/**
 * Posix thread process new connection
 *
 */
void tm_thread( int serversock, unsigned int max_client ){
    int clientsock, *arg;
    struct sockaddr_in client_addr;
    char currtime[32];
    unsigned clientlen;
    pthread_attr_t thread_attr;
    void *thread_result;
    
    /* Setting pthread attribute */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    /* Run until cancelled */
    while (1){
        pthread_t thread;
        unsigned int clientlen = sizeof(client_addr);
        memset(currtime, 0, sizeof(currtime));
        getdate(currtime);

        /* Wait for client connection */
        if ((clientsock = accept(serversock, (struct sockaddr *) &client_addr, &clientlen)) < 0){
            die("Failed to accept client connection");
        }

        /* Use thread process new connection */
        arg = &clientsock;
        printf("socket id: %d\n", *arg);
        if (pthread_create(&thread, &thread_attr, thread_function, (void *)arg) != 0){
            die("Create new thread failed");
        }
        /* if (pthread_join(thread, &thread_result) != 0){
            die("Thread join failed");
        }
    pthread_t thread;
        */
        printf("Main exit.\n");


        /* Use child process new connection */
        //if ( fork() == 0 ){
        //    HandleClient(clientsock, client_addr);
        //} else {
        //    wait(NULL); 
        //}

        /* Not use close socket connection */
        //close(clientsock);
    }
    /* Destory pthread attribute */
    (void)pthread_attr_destroy(&thread_attr);
}


void tm_event_accept(int serversock, short event, void *arg){
    int client_sock;
    struct sockaddr_in cin;
    socklen_t len;
    struct event *ev = NULL;

    len = sizeof(struct sockaddr_in);
    if ((client_sock = accept(serversock, (struct sockaddr *)&cin, &len)) <= 0){
        fprintf(stderr, "accept faild\n");
        return;
    }
    ev = malloc(sizeof(struct event));
    event_set(ev, client_sock, EV_READ, (void *)handle_client_event, ev);
    event_add(ev, NULL);
}


void tm_event_init(int serversock){
    struct event ev; 

    event_init();
    event_set(&ev, serversock, EV_READ|EV_PERSIST, tm_event_accept, &ev);
    event_add(&ev, NULL);
    event_dispatch();
}



/**
 * Initialize server socket listen
 *
 */
void init_server_listen( unsigned int port, unsigned int max_client ){
    int serversock;
    struct sockaddr_in server_addr;
    char currtime[32];
  
    /* Create the TCP socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        die("Failed to create socket");
    }

    /* Construct the server sockaddr_in structure */
    memset(&server_addr, 0, sizeof(server_addr));		/* Clear struct */
    server_addr.sin_family = AF_INET;					/* Internet/IP */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/* Incoming addr */
    server_addr.sin_port = htons(port);					/* server port */

    /* Bind the server socket */
    if (bind(serversock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        die("Failed to bind the server socket");
    }

    /* Listen on the server socket */
    if (listen(serversock, max_client) < 0){
        die("Failed to listen on server socket");
    }

    /* Print listening message */
    getdate(currtime);
    fprintf(stdout, "[%s] Start server listening at port %d ...\n", currtime, port);
    fprintf(stdout, "[%s] Waiting client connection ...\n", currtime);

	/* Multiplexing IO */
	//tm_epoll(serversock, max_client);
	//tm_select(serversock, max_client);
	//tm_select2(serversock, max_client);
	//tm_select3(serversock, max_client);
	//tm_select3(serversock, max_client);
	tm_thread(serversock, max_client);
    //tm_event_init(serversock);

}




/********************************
 *
 *     Server running
 *
 ********************************/


/**
 * Main function 
 *
 */
int main( int argc, char *argv[] ){

	/* Parse cli input options */
	if ( argc > 1 ){
		if ( parse_options( argc, argv ) != 0 ){
			exit(-1);
		}
	}
	
	/* Set is daemon mode run */
	if ( g_is_daemon ){
		pid_t pid;
		if ( (pid = fork()) < 0 ){
			die("daemon fork error");
		} else if ( pid != 0){
			exit(1);
		}
	}

	/* Debug mode out configure information */
	if ( g_is_debug ){
		print_config();
	}

	/* Init hash table & queue */
	g_htable = tm_hcreate( g_max_tablesize );
	g_qlist  = tm_qcreate();
	init_status( g_max_mem_size );

	/* Start server listen */
	init_server_listen( g_port, g_max_client );
	
	return(0);
}




