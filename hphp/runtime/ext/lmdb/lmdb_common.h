#ifndef incl_HPHP_LMDB_COMMON_H
#define incl_HPHP_LMDB_COMMON_H

#include <lmdb.h>

#include "hphp/runtime/base/base-includes.h"
#include "hphp/util/string-vsnprintf.h"

namespace HPHP {

class LMDBException {
 public:
  static Object allocObject() {
    if (cls == nullptr) {
      initClass();
    }
    return ObjectData::newInstance(cls);
  }

  static Object allocObject(const Variant& arg) {
    Object ret = allocObject();
    TypedValue dummy;
    g_context->invokeFunc(&dummy,
                          cls->getCtor(),
                          make_packed_array(arg),
                          ret.get());
    return ret;
  }

 private:
  static void initClass() {
    cls = Unit::lookupClass(StringData::Make("LMDBException"));
  }

  static HPHP::Class* cls;
};

template<typename T>
void lmdbThrow(const char* fmt, ...)
  ATTRIBUTE_PRINTF(1, 2) ATTRIBUTE_NORETURN;

template<typename T>
void lmdbThrow(const char* fmt, ...) {
  va_list ap;
  std::string msg;
  va_start(ap, fmt);
  string_vsnprintf(msg, fmt, ap);
  va_end(ap);
  throw T::allocObject(msg);
}

#define LMDB_THROW lmdbThrow<LMDBException>

class LMDBDbi;
class LMDBTxn;

class LMDBEnv : public SweepableResourceData {
 public:
  LMDBEnv();
  ~LMDBEnv();

  void set_mapsize(size_t size);
  void set_maxreaders(unsigned int readers);
  void set_maxdbs(MDB_dbi dbs);

  void open(const std::string& path, unsigned int flags, mdb_mode_t mode);
  void stat(MDB_stat *stat);
  void close();

  LMDBDbi* open_dbi(unsigned int flags);
  LMDBDbi* open_dbi(const std::string& name, unsigned int flags);
  void close_dbi(MDB_dbi dbi);

  LMDBTxn* begin_txn(unsigned int flags);

  void ref() { m_refcount++; }
  void unref() { m_refcount--; }

 private:
  MDB_env *m_env;
  bool m_open = false;
  unsigned int m_refcount = 0;

  void assert_open(bool must_be_opened);

  LMDBDbi* open_dbi(const char *name, unsigned int flags);
};

class LMDBDbi : public SweepableResourceData {
 public:
  LMDBDbi(LMDBEnv *env_obj, MDB_dbi dbi);
  ~LMDBDbi();

  void stat(LMDBTxn *txn_obj, MDB_stat *stat);
  void close();

  void dirty() { m_wipcount++; }
  void undirty() { m_wipcount--; }

 private:
  LMDBEnv *m_env_obj;
  MDB_dbi m_dbi;
  bool m_open = true;
  unsigned int m_wipcount = 0;

  void assert_open();

  MDB_dbi get_dbi() { return m_dbi; }
  friend LMDBTxn;
};

class LMDBTxn : public SweepableResourceData {
 public:
  LMDBTxn(LMDBEnv *env_obj, MDB_txn *txn);
  ~LMDBTxn();

  LMDBTxn* begin_txn(unsigned int flags);

  bool get(LMDBDbi *dbi_obj, const MDB_val *key, MDB_val *value);
  void put(LMDBDbi *dbi_obj, const MDB_val *key, const MDB_val *value,
      unsigned int flags);
  void del(LMDBDbi *dbi_obj, const MDB_val *key, const MDB_val *value);

  void commit();
  void abort();

 private:
  LMDBEnv *m_env_obj;
  MDB_txn *m_txn;
  std::map<LMDBDbi *, bool> m_dirty_dbis;

  void assert_inprogress();
  void undirty_dbis();

  MDB_txn* get_txn() { return m_txn; }
  friend LMDBDbi;
};

}

#endif /* !defined(incl_HPHP_LMDB_COMMON_H) */
