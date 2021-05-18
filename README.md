tmcache
=======

类似Memcached的缓存服务器，兼容Memcached协议 

The like memcached mini memory cache daemon (ANSI C, GCC require)

 * 最后更新: 2008/10/25  v1.0.0 beta


---


###### Implementation Mechanism
> 
> The Tmcache cache service， mainly adopts a single-process model, and the internal core data structure mainly relies on Hash tables and Queues.
> 
> The protocol level is implemented in accordance with Memcached's text version protocol and can be run as an independent daemon.



###### Usage:

<pre>

<b>tmcache - TieMa?(Tiny&Mini) Memory Cache Server (Daemon)</b>
tmcache is a very small memory cache server, It run from daemon. It is similar to memcachd, and fully compatible with the memcached communication protocol can be easily carried from the memcached to tmcache transplant. tmcache is based on the thread to run, the faster the speed.

<b>tmcache including:</b>
  * Based memory data storage
  * Compatible memcached communication protocol
  * Few operation interface, The use of simple
  * Support custom port,max_clients,memory use control
  * etc ...
  

<b>tmhttpd command help</b>

#=======================================
# TieMa(Tiny&Mini) Memory Cache Server
# Version 1.0.0_alpha
# 
# heiyeluren <blog.csdn.net/heiyeshuwu>
#=======================================

usage: ./tmcache [OPTION] ... 

Options: 
  -p <num>      port number to listen on,default 11211
  -d            run as a daemon, default No
  -m <num>      max memory to use for items in megabytes, default is 16M
  -c <num>      max simultaneous connections, default is 1024
  -v            print version information
  -h            print this help and exit

Example: 
  ./tmcache -p 11211 -m 16 -c 1024 -d


</pre>

