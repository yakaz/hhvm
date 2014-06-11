#include "hphp/runtime/ext/lmdb/lmdb_common.h"

namespace HPHP {

static MDB_txn* begin_txn(MDB_env *env, MDB_txn *parent, unsigned int flags) {
  int ret;
  MDB_txn *txn;

  ret = mdb_txn_begin(env, parent, flags, &txn);
  if (ret != 0) {
    LMDB_THROW("Failed to create transaction: %s", mdb_strerror(ret));
  }

  return txn;
}

/* -------------------------------------------------------------------
 * LMDB environment.
 * ------------------------------------------------------------------- */

LMDBEnv::LMDBEnv() {
  int ret;

  ret = mdb_env_create(&m_env);
  if (ret != 0) {
    LMDB_THROW("Failed to create environment: %s", mdb_strerror(ret));
  }
}

void LMDBEnv::assert_open(bool must_be_opened) {
  if (m_env == NULL) {
    LMDB_THROW("Environment already closed");
  }
  if (m_open != must_be_opened) {
    if (must_be_opened) {
      LMDB_THROW("Environment not opened yet");
    } else {
      LMDB_THROW("Environment already opened");
    }
  }
}

void LMDBEnv::set_mapsize(size_t size) {
  int ret;

  assert_open(false);

  ret = mdb_env_set_mapsize(m_env, size);
  if (ret != 0) {
    LMDB_THROW("Failed to set mapsize to %lu: %s", size, mdb_strerror(ret));
  }
}

void LMDBEnv::set_maxreaders(unsigned int readers) {
  int ret;

  assert_open(false);

  ret = mdb_env_set_maxreaders(m_env, readers);
  if (ret != 0) {
    LMDB_THROW("Failed to set maxreaders to %u: %s",
        readers, mdb_strerror(ret));
  }
}

void LMDBEnv::set_maxdbs(MDB_dbi dbs) {
  int ret;

  assert_open(false);

  ret = mdb_env_set_maxdbs(m_env, dbs);
  if (ret != 0) {
    LMDB_THROW("Failed to set maxdbs to %u: %s", dbs, mdb_strerror(ret));
  }
}

void LMDBEnv::open(const std::string& path, unsigned int flags,
 mdb_mode_t mode) {
  int ret;

  assert_open(false);

  ret = mdb_env_open(m_env, path.c_str(), flags, mode);
  if (ret != 0) {
    mdb_env_close(m_env);
    m_env = NULL;
    LMDB_THROW("Failed to open environment \"%s\": %s",
        path.c_str(), mdb_strerror(ret));
  }

  m_open = true;
}

void LMDBEnv::stat(MDB_stat *stat) {
  int ret;

  assert_open(true);

  ret = mdb_env_stat(m_env, stat);
  if (ret != 0) {
    LMDB_THROW("Failed to get environment stats: %s",
        mdb_strerror(ret));
  }
}

LMDBDbi* LMDBEnv::open_dbi(unsigned int flags) {
  return open_dbi(NULL, flags);
}

LMDBDbi* LMDBEnv::open_dbi(const std::string& name, unsigned int flags) {
  return open_dbi(name.c_str(), flags);
}

LMDBDbi* LMDBEnv::open_dbi(const char *name, unsigned int flags) {
  int ret;
  MDB_txn *tmp_txn;
  MDB_dbi dbi;

  assert_open(true);

  tmp_txn = HPHP::begin_txn(m_env, NULL, MDB_RDONLY);

  ret = mdb_dbi_open(tmp_txn, name ? name : NULL, flags, &dbi);
  if (ret != 0) {
    mdb_txn_abort(tmp_txn);
    LMDB_THROW("Failed to open DB \"%s\": %s",
        name ? name : "<unnamed>", mdb_strerror(ret));
  }

  ret = mdb_txn_commit(tmp_txn);
  if (ret != 0) {
    mdb_txn_abort(tmp_txn);
    LMDB_THROW("Failed to commit temporary transaction, "
        "while opening DB \"%s\": %s",
        name ? name : "<unnamed>", mdb_strerror(ret));
  }

  return new LMDBDbi(this, dbi);
}

void LMDBEnv::close_dbi(MDB_dbi dbi) {
  assert_open(true);

  mdb_dbi_close(m_env, dbi);
}

LMDBTxn* LMDBEnv::begin_txn(unsigned int flags) {
  MDB_txn *txn;

  assert_open(true);

  txn = HPHP::begin_txn(m_env, NULL, flags);

  return new LMDBTxn(this, txn);
}

void LMDBEnv::close() {
  assert_open(true);

  if (m_refcount > 0) {
    LMDB_THROW("Environment still in use by %d transactions or dbi handles",
        m_refcount);
  }

  mdb_env_close(m_env);
  m_env = NULL;
  m_open = false;
}

LMDBEnv::~LMDBEnv() {
  if (m_env != NULL) {
    this->close();
  }
}

/* -------------------------------------------------------------------
 * LMDB database.
 * ------------------------------------------------------------------- */

LMDBDbi::LMDBDbi(LMDBEnv *env_obj, MDB_dbi dbi) :
  m_env_obj(env_obj), m_dbi(dbi) {
    m_env_obj->ref();
}

void LMDBDbi::assert_open() {
  if (!m_open) {
    LMDB_THROW("DB already closed");
  }
}

void LMDBDbi::stat(LMDBTxn *txn_obj, MDB_stat *stat) {
  int ret;

  assert_open();

  ret = mdb_stat(txn_obj->get_txn(), m_dbi, stat);
  if (ret != 0) {
    LMDB_THROW("Failed to get DB stats: %s",
        mdb_strerror(ret));
  }
}

void LMDBDbi::close() {
  assert_open();

  if (m_wipcount > 0) {
    LMDB_THROW("DB modified by %d unfinished transactions",
        m_wipcount);
  }

  m_env_obj->close_dbi(m_dbi);
  m_env_obj->unref();
  m_open = false;
}

LMDBDbi::~LMDBDbi() {
  if (m_open) {
    this->close();
  }
}

/* -------------------------------------------------------------------
 * LMDB transaction.
 * ------------------------------------------------------------------- */

LMDBTxn::LMDBTxn(LMDBEnv *env_obj, MDB_txn *txn) :
  m_env_obj(env_obj), m_txn(txn) {
    m_env_obj->ref();
}

void LMDBTxn::assert_inprogress() {
  if (m_txn == NULL) {
    LMDB_THROW("Transaction already finished");
  }
}

void LMDBTxn::undirty_dbis() {
  for (auto i = m_dirty_dbis.begin(); i != m_dirty_dbis.end(); ++i) {
    i->first->undirty();
  }
}

LMDBTxn* LMDBTxn::begin_txn(unsigned flags) {
  MDB_env *env;
  MDB_txn *txn;

  env = mdb_txn_env(m_txn);
  txn = HPHP::begin_txn(env, m_txn, flags);

  return new LMDBTxn(m_env_obj, txn);
}

bool LMDBTxn::get(LMDBDbi *dbi_obj, const MDB_val *key,
 MDB_val *value) {
  int ret;
  MDB_dbi dbi;

  assert_inprogress();

  dbi = dbi_obj->get_dbi();

  ret = mdb_get(m_txn, dbi, (MDB_val *)key, value);
  if (ret == MDB_NOTFOUND) {
    return false;
  } else if (ret != 0) {
    LMDB_THROW("Failed to get key \"%.*s\": %s",
        (int)key->mv_size, (char *)key->mv_data, mdb_strerror(ret));
  }

  return true;
}

void LMDBTxn::put(LMDBDbi *dbi_obj, const MDB_val *key,
 const MDB_val *value, unsigned int flags) {
  int ret;
  MDB_dbi dbi;

  assert_inprogress();

  dbi = dbi_obj->get_dbi();

  ret = mdb_put(m_txn, dbi, (MDB_val *)key, (MDB_val *)value, flags);
  if (ret != 0) {
    LMDB_THROW("Failed to put key \"%.*s\": %s",
        (int)key->mv_size, (char *)key->mv_data, mdb_strerror(ret));
  }

  if (m_dirty_dbis.count(dbi_obj) == 0) {
    dbi_obj->dirty();
    m_dirty_dbis[dbi_obj] = true;
  }
}

void LMDBTxn::del(LMDBDbi *dbi_obj, const MDB_val *key,
 const MDB_val *value) {
  int ret;
  MDB_dbi dbi;

  assert_inprogress();

  dbi = dbi_obj->get_dbi();

  ret = mdb_del(m_txn, dbi, (MDB_val *)key, (MDB_val *)value);
  if (ret != 0) {
    LMDB_THROW("Failed to del key \"%.*s\": %s",
        (int)key->mv_size, (char *)key->mv_data, mdb_strerror(ret));
  }

  if (m_dirty_dbis.count(dbi_obj) == 0) {
    dbi_obj->dirty();
    m_dirty_dbis[dbi_obj] = true;
  }
}

void LMDBTxn::commit() {
  int ret;

  assert_inprogress();

  ret = mdb_txn_commit(m_txn);
  if (ret != 0) {
    LMDB_THROW("Failed to commit transaction: %s",
        mdb_strerror(ret));
  }

  undirty_dbis();

  m_env_obj->unref();
  m_txn = NULL;
}

void LMDBTxn::abort() {
  assert_inprogress();

  mdb_txn_abort(m_txn);

  undirty_dbis();

  m_env_obj->unref();
  m_txn = NULL;
}

LMDBTxn::~LMDBTxn() {
  if (m_txn != NULL) {
    this->abort();
  }
}

}
