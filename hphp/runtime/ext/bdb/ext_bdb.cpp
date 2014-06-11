#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/base/ini-setting.h"

#include "hphp/runtime/ext/bdb/bdb_common.h"

namespace HPHP {

static const StaticString
  s_BDB("BDB"),
  s_db("__db");

HPHP::Class* BDBException::cls = nullptr;

static Resource get_BDB_resource(const Object& obj) {
  auto res = obj->o_realProp(s_db, ObjectData::RealPropUnchecked,
      s_BDB.get());

  if (!res || !res->isResource()) {
    return null_resource;
  }

  return res->toResource();
}

static BDB* get_BDB(const Object& obj) {
  auto res = get_BDB_resource(obj);
  return res.getTyped<BDB>(false, false);
}

static void HHVM_METHOD(BDB, __construct, const String& path,
 int64_t type, int64_t flags, int64_t mode) {
  BDB* db;

  if (type & ~(DB_UNKNOWN|DB_BTREE)) {
    BDB_THROW("Invalid DB type: %ld", type);
  }

  if (flags & ~(DB_CREATE|DB_RDONLY|DB_TRUNCATE)) {
    BDB_THROW("Invalid open flags: %ld", flags);
  }

  db = new BDB();

  try {
    db->open(path.toCppString(), (DBTYPE)type, flags, mode);
  } catch (Exception &e) {
    delete db;
    throw;
  }

  this_->o_set(s_db, db, s_BDB.get());
}

static bool HHVM_METHOD(BDB, exists, const String& php_key) {
  auto db = get_BDB(this_);
  DBT key, value;
  bool exists;

  memset(&key, 0, sizeof(key));
  key.data = (void *)php_key.c_str();
  key.size = php_key.size();

  exists = db->get(&key, &value);
  return exists;
}

static Variant HHVM_METHOD(BDB, get, const String& php_key) {
  auto db = get_BDB(this_);
  DBT key, value;
  bool exists;

  memset(&key, 0, sizeof(key));
  key.data = (void *)php_key.c_str();
  key.size = php_key.size();

  exists = db->get(&key, &value);
  if (!exists) {
    return null_resource;
  }

  return String((const char *)value.data, value.size, CopyString);
}

static void HHVM_METHOD(BDB, put, const String& php_key,
 const String& php_value, int64_t flags) {
  auto db = get_BDB(this_);
  DBT key, value;

  if (flags & ~(DB_NOOVERWRITE)) {
    BDB_THROW("Invalid put flags: %ld", flags);
  }

  memset(&key, 0, sizeof(key));
  key.data = (void *)php_key.c_str();
  key.size = php_key.size();
  memset(&value, 0, sizeof(value));
  value.data = (void *)php_value.c_str();
  value.size = php_value.size();

  db->put(&key, &value, flags);
}

static void HHVM_METHOD(BDB, del, const String& php_key) {
  auto db = get_BDB(this_);
  DBT key;

  memset(&key, 0, sizeof(key));
  key.data = (void *)php_key.c_str();
  key.size = php_key.size();

  db->del(&key);
}

static void HHVM_METHOD(BDB, close) {
  auto db = get_BDB(this_);

  db->close();
}

/* -------------------------------------------------------------------
 * PHP extension.
 * ------------------------------------------------------------------- */

static const StaticString
  s_DB_UNKNOWN("DB_UNKNOWN"),
  s_DB_BTREE("DB_BTREE"),
  s_DB_CREATE("DB_CREATE"),
  s_DB_RDONLY("DB_RDONLY"),
  s_DB_TRUNCATE("DB_TRUNCATE"),
  s_DB_NOOVERWRITE("DB_NOOVERWRITE");

class bdbExtension : public Extension {
 public:
  bdbExtension() : Extension("bdb", "0.1.0") {}

  virtual void moduleInit() {
    HHVM_ME(BDB, __construct);
    HHVM_ME(BDB, exists);
    HHVM_ME(BDB, get);
    HHVM_ME(BDB, put);
    HHVM_ME(BDB, del);
    HHVM_ME(BDB, close);

    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_UNKNOWN.get(), (int64_t)DB_UNKNOWN);
    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_BTREE.get(), (int64_t)DB_BTREE);
    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_CREATE.get(), (int64_t)DB_CREATE);
    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_RDONLY.get(), (int64_t)DB_RDONLY);
    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_TRUNCATE.get(), (int64_t)DB_TRUNCATE);
    Native::registerClassConstant<KindOfInt64>(s_BDB.get(),
        s_DB_NOOVERWRITE.get(), (int64_t)DB_NOOVERWRITE);

    loadSystemlib();
  }
} s_bdb_extension;

// Uncomment for non-bundled module
//HHVM_GET_MODULE(bdb);

//////////////////////////////////////////////////////////////////////////////
} // namespace HPHP
