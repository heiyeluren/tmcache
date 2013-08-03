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
 * $Id: tm_common.h 2008-6-30, 2008-8-29 15:52 heiyeluren $
 */

/**
 * Common function define 
 *
 */

/* Die alert message */
void die(char *mess);

/* substr - Sub string from pos to length */
char *substr( const char *s, int start_pos, int length, char *ret );

/* explode -  separate string by separator */
void explode(char *from, char delim, char ***to, int *item_num);

/* strtolower - string to lowner */
char *strtolower( char *s );

/* strtoupper - string to upper */
char *strtoupper( char *s );

/* strpos - find char at string position */
int strpos (const char *s, char c);

/* strrpos - find char at string last position */
int strrpos (const char *s, char c);

/* trim - strip left&right space char */
char *trim( char *s );

/* ltrim - strip left space char */
char *ltrim( char *s );

/* is_numeric - Check string is number */
int is_numeric( const char *s );

/* Fetch current date tme */
void getdate(char *s);

/* Set socket nonblock */
int socket_set_nonblock( int sockfd );
