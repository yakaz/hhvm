/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef __EXT_MEMCACHE_H__
#define __EXT_MEMCACHE_H__

// >>>>>> Generated by idl.php. Do NOT modify. <<<<<<

#include <runtime/base/base_includes.h>
#include <libmemcached/memcached.h>
namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

Object f_memcache_connect(CStrRef host, int port = 0, int timeout = 0, int timeoutms = 0);
Object f_memcache_pconnect(CStrRef host, int port = 0, int timeout = 0, int timeoutms = 0);
bool f_memcache_add(CObjRef memcache, CStrRef key, CVarRef var, int flag = 0, int expire = 0);
bool f_memcache_set(CObjRef memcache, CStrRef key, CVarRef var, int flag = 0, int expire = 0);
bool f_memcache_replace(CObjRef memcache, CStrRef key, CVarRef var, int flag = 0, int expire = 0);
Variant f_memcache_get(CObjRef memcache, CVarRef key, VRefParam flags = uninit_null());
bool f_memcache_delete(CObjRef memcache, CStrRef key, int expire = 0);
int64_t f_memcache_increment(CObjRef memcache, CStrRef key, int offset = 1);
int64_t f_memcache_decrement(CObjRef memcache, CStrRef key, int offset = 1);
bool f_memcache_close(CObjRef memcache);
bool f_memcache_debug(bool onoff);
Variant f_memcache_get_version(CObjRef memcache);
bool f_memcache_flush(CObjRef memcache, int timestamp = 0);
bool f_memcache_setoptimeout(CObjRef memcache, int timeoutms);
int64_t f_memcache_get_server_status(CObjRef memcache, CStrRef host, int port = 0);
bool f_memcache_set_compress_threshold(CObjRef memcache, int threshold, double min_savings = 0.2);
Array f_memcache_get_stats(CObjRef memcache, CStrRef type = null_string, int slabid = 0, int limit = 100);
Array f_memcache_get_extended_stats(CObjRef memcache, CStrRef type = null_string, int slabid = 0, int limit = 100);
bool f_memcache_set_server_params(CObjRef memcache, CStrRef host, int port = 11211, int timeout = 0, int retry_interval = 0, bool status = true, CVarRef failure_callback = null_variant);
bool f_memcache_add_server(CObjRef memcache, CStrRef host, int port = 11211, bool persistent = false, int weight = 0, int timeout = 0, int retry_interval = 0, bool status = true, CVarRef failure_callback = null_variant, int timeoutms = 0);

///////////////////////////////////////////////////////////////////////////////
// class Memcache

FORWARD_DECLARE_CLASS_BUILTIN(Memcache);
class c_Memcache : public ExtObjectData, public Sweepable {
 public:
  DECLARE_CLASS(Memcache, Memcache, ObjectData)

  // need to implement
  public: c_Memcache(VM::Class* cls = c_Memcache::s_cls);
  public: ~c_Memcache();
  public: void t___construct();
  public: bool t_connect(CStrRef host, int port = 0, int timeout = 0, int timeoutms = 0);
  public: bool t_pconnect(CStrRef host, int port = 0, int timeout = 0, int timeoutms = 0);
  public: bool t_add(CStrRef key, CVarRef var, int flag = 0, int expire = 0);
  public: bool t_set(CStrRef key, CVarRef var, int flag = 0, int expire = 0);
  public: bool t_replace(CStrRef key, CVarRef var, int flag = 0, int expire = 0);
  public: Variant t_get(CVarRef key, VRefParam flags = uninit_null());
  public: bool t_delete(CStrRef key, int expire = 0);
  public: int64_t t_increment(CStrRef key, int offset = 1);
  public: int64_t t_decrement(CStrRef key, int offset = 1);
  public: Variant t_getversion();
  public: bool t_flush(int expire = 0);
  public: bool t_setoptimeout(int64_t timeoutms);
  public: bool t_close();
  public: int64_t t_getserverstatus(CStrRef host, int port = 0);
  public: bool t_setcompressthreshold(int threshold, double min_savings = 0.2);
  public: Array t_getstats(CStrRef type = null_string, int slabid = 0, int limit = 100);
  public: Array t_getextendedstats(CStrRef type = null_string, int slabid = 0, int limit = 100);
  public: bool t_setserverparams(CStrRef host, int port = 11211, int timeout = 0, int retry_interval = 0, bool status = true, CVarRef failure_callback = null_variant);
  public: bool t_addserver(CStrRef host, int port = 11211, bool persistent = false, int weight = 0, int timeout = 0, int retry_interval = 0, bool status = true, CVarRef failure_callback = null_variant, int timeoutms = 0);
  public: Variant t___destruct();



 private:
  memcached_st m_memcache;
  int m_compress_threshold;
  double m_min_compress_savings;
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // __EXT_MEMCACHE_H__
