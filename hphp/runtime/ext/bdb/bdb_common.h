#ifndef incl_HPHP_BDB_COMMON_H
#define incl_HPHP_BDB_COMMON_H

#include <db.h>

#include "hphp/runtime/base/base-includes.h"
#include "hphp/util/string-vsnprintf.h"

namespace HPHP {

class BDBException {
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
    cls = Unit::lookupClass(StringData::Make("BDBException"));
  }

  static HPHP::Class* cls;
};

template<typename T>
void bdbThrow(const char* fmt, ...)
  ATTRIBUTE_PRINTF(1, 2) ATTRIBUTE_NORETURN;

template<typename T>
void bdbThrow(const char* fmt, ...) {
  va_list ap;
  std::string msg;
  va_start(ap, fmt);
  string_vsnprintf(msg, fmt, ap);
  va_end(ap);
  throw T::allocObject(msg);
}

#define BDB_THROW bdbThrow<BDBException>

class BDB : public SweepableResourceData {
 public:
  BDB();
  ~BDB();

  void open(const std::string& path, DBTYPE type, uint32_t flags, int mode);
  bool get(const DBT *key, DBT *value);
  void put(const DBT *key, const DBT *value, uint32_t flags);
  void del(const DBT *key);
  void close();

 private:
  DB *m_db;

  void assert_open();
};

}

#endif /* !defined(incl_HPHP_BDB_COMMON_H) */
