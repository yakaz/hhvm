#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/base/ini-setting.h"

#include "hphp/runtime/ext/lmdb/lmdb_common.h"

namespace HPHP {

static const StaticString
  s_LMDBEnv("LMDBEnv"),
  s_env("__env"),
  s_LMDBTxn("LMDBTxn"),
  s_parent_obj("__parent"),
  s_txn("__txn"),
  s_LMDBDbi("LMDBDbi"),
  s_dbi("__dbi"),
  s_flags("flags"),
  s_mode("mode"),
  s_mapsize("mapsize"),
  s_maxreaders("maxreaders"),
  s_maxdbs("maxdbs");

static const StaticString
  s_ms_psize("psize"),
  s_ms_depth("depth"),
  s_ms_branch_pages("branch_pages"),
  s_ms_leaf_pages("leaf_pages"),
  s_ms_overflow_pages("overflow_pages"),
  s_ms_entries("entries");

HPHP::Class* LMDBException::cls = nullptr;

static LMDBEnv* get_LMDBEnv(const Object& obj);
static LMDBDbi* get_LMDBDbi(const Object& obj);
static LMDBTxn* get_LMDBTxn(const Object& obj);

/* -------------------------------------------------------------------
 * LMDB environment.
 * ------------------------------------------------------------------- */

static Resource get_LMDBEnv_resource(const Object& obj) {
  auto res = obj->o_realProp(s_env, ObjectData::RealPropUnchecked,
      s_LMDBEnv.get());

  if (!res || !res->isResource()) {
    return null_resource;
  }

  return res->toResource();
}

static LMDBEnv* get_LMDBEnv(const Object& obj) {
  auto res = get_LMDBEnv_resource(obj);
  return res.getTyped<LMDBEnv>(false, false);
}

static void HHVM_METHOD(LMDBEnv, __construct, const String& path,
 const Variant& options_container) {
  LMDBEnv* env;
  unsigned int flags = 0;
  mdb_mode_t mode = 0666;

  env = new LMDBEnv();

  try {
    if (options_container.isArray()) {
      const Array options = options_container.toCArrRef();

      if (options.exists(s_flags)) {
        flags = options[s_flags].toInt64();
        if (flags < 0) {
          LMDB_THROW("Invalid environment flags: %u", flags);
        }
      }

      if (options.exists(s_mode)) {
        mode = options[s_mode].toInt64();
        if (mode < 0) {
          LMDB_THROW("Invalid environment mode: %04o", mode);
        }
      }

      if (options.exists(s_mapsize)) {
        size_t mapsize = options[s_mapsize].toInt64();
        if (mapsize < 0) {
          LMDB_THROW("Invalid environment mapsize: %lu", mapsize);
        }

        env->set_mapsize(mapsize);
      }

      if (options.exists(s_maxreaders)) {
        unsigned int maxreaders = options[s_maxreaders].toInt32();
        if (maxreaders < 0) {
          LMDB_THROW("Invalid environment maxreaders: %d", maxreaders);
        }

        env->set_maxreaders(maxreaders);
      }

      if (options.exists(s_maxdbs)) {
        int maxdbs = options[s_maxdbs].toInt32();
        if (maxdbs < 0) {
          LMDB_THROW("Invalid environment maxdbs: %d", maxdbs);
        }

        env->set_maxdbs(maxdbs);
      }
    }

    env->open(path.toCppString(), flags, mode);
  } catch (Exception &e) {
    delete env;
    throw;
  }

  this_->o_set(s_env, env, s_LMDBEnv.get());
}

static Array HHVM_METHOD(LMDBEnv, stat) {
  auto env = get_LMDBEnv(this_);
  MDB_stat stat;
  Array ret;

  env->stat(&stat);

  ret.set(s_ms_psize, Variant((uint64_t)stat.ms_psize));
  ret.set(s_ms_depth, Variant((uint64_t)stat.ms_depth));
  ret.set(s_ms_branch_pages, Variant(stat.ms_branch_pages));
  ret.set(s_ms_leaf_pages, Variant(stat.ms_leaf_pages));
  ret.set(s_ms_overflow_pages, Variant(stat.ms_overflow_pages));
  ret.set(s_ms_entries, Variant(stat.ms_entries));

  return ret;
}

static void HHVM_METHOD(LMDBEnv, close) {
  auto env = get_LMDBEnv(this_);

  env->close();
}

/* -------------------------------------------------------------------
 * LMDB database.
 * ------------------------------------------------------------------- */

static Resource get_LMDBDbi_resource(const Object& obj) {
  auto res = obj->o_realProp(s_dbi, ObjectData::RealPropUnchecked,
      s_LMDBDbi.get());

  if (!res || !res->isResource()) {
    return null_resource;
  }

  return res->toResource();
}

static LMDBDbi* get_LMDBDbi(const Object& obj) {
  auto res = get_LMDBDbi_resource(obj);
  return res.getTyped<LMDBDbi>(false, false);
}

static void HHVM_METHOD(LMDBDbi, __construct,
 const Object& php_env, const Variant& name, int64_t flags) {
  Resource data;

  if (flags < 0) {
    LMDB_THROW("Invalid DB flags: %ld", flags);
  }

  auto env = get_LMDBEnv(php_env);
  if (name.isNull()) {
    data = env->open_dbi(flags);
  } else if (name.isString()) {
    data = env->open_dbi(name.toCStrRef().toCppString(), flags);
  } else {
    LMDB_THROW("Invalid DB name");
  }

  this_->o_set(s_env, php_env, s_LMDBDbi.get());
  this_->o_set(s_dbi, data, s_LMDBDbi.get());
}

static Array HHVM_METHOD(LMDBDbi, stat,
 const Object& php_txn) {
  auto dbi = get_LMDBDbi(this_);
  auto txn = get_LMDBTxn(php_txn);
  MDB_stat stat;
  Array ret;

  dbi->stat(txn, &stat);

  ret.set(s_ms_psize, Variant((uint64_t)stat.ms_psize));
  ret.set(s_ms_depth, Variant((uint64_t)stat.ms_depth));
  ret.set(s_ms_branch_pages, Variant(stat.ms_branch_pages));
  ret.set(s_ms_leaf_pages, Variant(stat.ms_leaf_pages));
  ret.set(s_ms_overflow_pages, Variant(stat.ms_overflow_pages));
  ret.set(s_ms_entries, Variant(stat.ms_entries));

  return ret;
}

static void HHVM_METHOD(LMDBDbi, close) {
  auto res = get_LMDBDbi(this_);

  res->close();
}

/* -------------------------------------------------------------------
 * LMDB transaction.
 * ------------------------------------------------------------------- */

static Resource get_LMDBTxn_resource(const Object& obj) {
  auto res = obj->o_realProp(s_txn, ObjectData::RealPropUnchecked,
      s_LMDBTxn.get());

  if (!res || !res->isResource()) {
    return null_resource;
  }

  return res->toResource();
}

static LMDBTxn* get_LMDBTxn(const Object& obj) {
  auto res = get_LMDBTxn_resource(obj);
  return res.getTyped<LMDBTxn>(false, false);
}

static void HHVM_METHOD(LMDBTxn, __construct,
 const Object& parent, int64_t flags) {
  Resource data;

  if (flags & (~MDB_RDONLY)) {
    LMDB_THROW("Invalid transaction flags: %ld", flags);
  }

  if (parent->o_instanceof(s_LMDBEnv.get())) {
    /* The caller wants a top-level transaction. */
    auto env = get_LMDBEnv(parent);
    data = env->begin_txn(flags);
  } else if (parent->o_instanceof(s_LMDBTxn.get())) {
    /* The caller wants a nested transaction. */
    auto txn = get_LMDBTxn(parent);
    data = txn->begin_txn(flags);
  } else {
    LMDB_THROW("Invalid parent object");
  }

  this_->o_set(s_parent_obj, parent, s_LMDBTxn.get());
  this_->o_set(s_txn, data, s_LMDBTxn.get());
}

static bool HHVM_METHOD(LMDBTxn, exists, const Object& php_dbi,
 const String& php_key) {
  auto txn = get_LMDBTxn(this_);
  auto dbi = get_LMDBDbi(php_dbi);
  MDB_val key, value;
  bool exists;

  key.mv_data = (void *)php_key.c_str();
  key.mv_size = php_key.size();

  exists = txn->get(dbi, &key, &value);
  return exists;
}

static Variant HHVM_METHOD(LMDBTxn, get, const Object& php_dbi,
 const String& php_key) {
  auto txn = get_LMDBTxn(this_);
  auto dbi = get_LMDBDbi(php_dbi);
  MDB_val key, value;
  bool exists;

  key.mv_data = (void *)php_key.c_str();
  key.mv_size = php_key.size();

  exists = txn->get(dbi, &key, &value);
  if (!exists) {
    return null_resource;
  }

  return String((const char *)value.mv_data, value.mv_size, CopyString);
}

static void HHVM_METHOD(LMDBTxn, put, const Object& php_dbi,
 const String& php_key, const String& php_value, int64_t flags) {
  auto txn = get_LMDBTxn(this_);
  auto dbi = get_LMDBDbi(php_dbi);
  MDB_val key, value;

  key.mv_data = (void *)php_key.c_str();
  key.mv_size = php_key.size();
  value.mv_data = (void *)php_value.c_str();
  value.mv_size = php_value.size();

  txn->put(dbi, &key, &value, flags);
}

static void HHVM_METHOD(LMDBTxn, del, const Object& php_dbi,
 const String& php_key, const Variant& php_value) {
  auto txn = get_LMDBTxn(this_);
  auto dbi = get_LMDBDbi(php_dbi);
  MDB_val key, value;

  key.mv_data = (void *)php_key.c_str();
  key.mv_size = php_key.size();

  if (php_value.isNull()) {
    txn->del(dbi, &key, NULL);
  } else {
    value.mv_data = (void *)php_value.toString().c_str();
    value.mv_size = php_value.toString().size();

    txn->del(dbi, &key, &value);
  }
}

static void HHVM_METHOD(LMDBTxn, commit) {
  auto res = get_LMDBTxn(this_);

  res->commit();
}

static void HHVM_METHOD(LMDBTxn, abort) {
  auto res = get_LMDBTxn(this_);

  res->abort();
}

/* -------------------------------------------------------------------
 * PHP extension.
 * ------------------------------------------------------------------- */

static const StaticString
  s_MDB_FIXEDMAP("MDB_FIXEDMAP"),
  s_MDB_NOSUBDIR("MDB_NOSUBDIR"),
  s_MDB_NOSYNC("MDB_NOSYNC"),
  s_MDB_RDONLY("MDB_RDONLY"),
  s_MDB_NOMETASYNC("MDB_NOMETASYNC"),
  s_MDB_WRITEMAP("MDB_WRITEMAP"),
  s_MDB_MAPASYNC("MDB_MAPASYNC"),
  s_MDB_NOTLS("MDB_NOTLS"),
  s_MDB_NOLOCK("MDB_NOLOCK"),
  s_MDB_NORDAHEAD("MDB_NORDAHEAD"),
  s_MDB_NOMEMINIT("MDB_NOMEMINIT");

static const StaticString
  s_MDB_REVERSEKEY("MDB_REVERSEKEY"),
  s_MDB_DUPSORT("MDB_DUPSORT"),
  s_MDB_INTEGERKEY("MDB_INTEGERKEY"),
  s_MDB_DUPFIXED("MDB_DUPFIXED"),
  s_MDB_INTEGERDUP("MDB_INTEGERDUP"),
  s_MDB_REVERSEDUP("MDB_REVERSEDUP"),
  s_MDB_CREATE("MDB_CREATE");

static const StaticString
  s_MDB_NOOVERWRITE("MDB_NOOVERWRITE"),
  s_MDB_NODUPDATA("MDB_NODUPDATA"),
  s_MDB_CURRENT("MDB_CURRENT"),
  s_MDB_RESERVE("MDB_RESERVE"),
  s_MDB_APPEND("MDB_APPEND"),
  s_MDB_APPENDDUP("MDB_APPENDDUP"),
  s_MDB_MULTIPLE("MDB_MULTIPLE");

class lmdbExtension : public Extension {
 public:
  lmdbExtension() : Extension("lmdb", "0.1.0") {}

  virtual void moduleInit() {
    HHVM_ME(LMDBEnv, __construct);
    HHVM_ME(LMDBEnv, stat);
    HHVM_ME(LMDBEnv, close);

    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_FIXEDMAP.get(), (int64_t)MDB_FIXEDMAP);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOSUBDIR.get(), (int64_t)MDB_NOSUBDIR);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOSYNC.get(), (int64_t)MDB_NOSYNC);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_RDONLY.get(), (int64_t)MDB_RDONLY);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOMETASYNC.get(), (int64_t)MDB_NOMETASYNC);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_WRITEMAP.get(), (int64_t)MDB_WRITEMAP);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_MAPASYNC.get(), (int64_t)MDB_MAPASYNC);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOTLS.get(), (int64_t)MDB_NOTLS);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOLOCK.get(), (int64_t)MDB_NOLOCK);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NORDAHEAD.get(), (int64_t)MDB_NORDAHEAD);
    Native::registerClassConstant<KindOfInt64>(s_LMDBEnv.get(),
        s_MDB_NOMEMINIT.get(), (int64_t)MDB_NOMEMINIT);

    HHVM_ME(LMDBDbi, __construct);
    HHVM_ME(LMDBDbi, stat);
    HHVM_ME(LMDBDbi, close);

    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_REVERSEKEY.get(), (int64_t)MDB_REVERSEKEY);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_DUPSORT.get(), (int64_t)MDB_DUPSORT);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_INTEGERKEY.get(), (int64_t)MDB_INTEGERKEY);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_DUPFIXED.get(), (int64_t)MDB_DUPFIXED);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_INTEGERDUP.get(), (int64_t)MDB_INTEGERDUP);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_REVERSEDUP.get(), (int64_t)MDB_REVERSEDUP);
    Native::registerClassConstant<KindOfInt64>(s_LMDBDbi.get(),
        s_MDB_CREATE.get(), (int64_t)MDB_CREATE);

    HHVM_ME(LMDBTxn, __construct);
    HHVM_ME(LMDBTxn, exists);
    HHVM_ME(LMDBTxn, get);
    HHVM_ME(LMDBTxn, put);
    HHVM_ME(LMDBTxn, del);
    HHVM_ME(LMDBTxn, commit);
    HHVM_ME(LMDBTxn, abort);

    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_NOOVERWRITE.get(), (int64_t)MDB_NOOVERWRITE);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_NODUPDATA.get(), (int64_t)MDB_NODUPDATA);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_CURRENT.get(), (int64_t)MDB_CURRENT);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_RESERVE.get(), (int64_t)MDB_RESERVE);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_APPEND.get(), (int64_t)MDB_APPEND);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_APPENDDUP.get(), (int64_t)MDB_APPENDDUP);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_MULTIPLE.get(), (int64_t)MDB_MULTIPLE);
    Native::registerClassConstant<KindOfInt64>(s_LMDBTxn.get(),
        s_MDB_RDONLY.get(), (int64_t)MDB_RDONLY);

    loadSystemlib();
  }
} s_lmdb_extension;

// Uncomment for non-bundled module
//HHVM_GET_MODULE(lmdb);

//////////////////////////////////////////////////////////////////////////////
} // namespace HPHP
