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
 * $Id: tm_common.h 2008-6-30, 2008-8-3 15:34 heiyeluren $
 */


/* type define */
typedef short status;

/* Return value define */
#define SUCCESS			1
#define OK				0
#define ERROR			-1

#define FALSE			0
#define TRUE			1
#define EXIT			2

/* Hash key operate method */
#define MODE_SET		0
#define	MODE_ADD		1
#define MODE_REPLACE	2

/* Data error type */
#define E_GENERAL		0		/* General error */
#define E_CLIENT		1		/* Client error */
#define E_SERVER		2		/* Server error */

/* Default options */
#define SERVER_NAME				"tmcached"
#define VERSION					"1.0.0_alpha"

#define BUFFER_SIZE				8192        /* Default data buffer size */
#define MAX_BUF_SIZE			1048576		/* Key data max length bytes */
#define MAX_TABLE_SIZE			10000;		/* Hash table max item total */
#define MAX_LIFE_TIME			2592000		/* Max expire time is a month */
#define GC_PROBAILITY           1           /* Garbage Collection probaility, value big probaility increase */
#define GC_DIVISOR              100         /* Garbage Collection probaility divisor, value big probaility decrease */

#define IS_DEBUG				0			/* Is open debug mode */
#define IS_DAEMON				0			/* Is daemon running */
#define PORT					11211		/* Server listen port */
#define MAX_CLIENTS				1024		/* Max connection requests */
#define MAX_MEM_SIZE			16777216	/* Max key data use memory size */
