#include "hphp/runtime/ext/bdb/bdb_common.h"

namespace HPHP {

BDB::BDB() {
  int ret;

  ret = db_create(&m_db, NULL, 0);
  if (ret != 0) {
    BDB_THROW("Failed to create DB handle: %s", db_strerror(ret));
  }
}

void BDB::assert_open() {
  if (m_db == NULL) {
    BDB_THROW("Environment already closed");
  }
}

void BDB::open(const std::string& path, DBTYPE type, uint32_t flags,
 int mode) {
  int ret;

  assert_open();

  ret = m_db->open(m_db, NULL, path.c_str(), NULL, type, flags, mode);
  if (ret != 0) {
    m_db->close(m_db, 0);
    m_db = NULL;
    BDB_THROW("Failed to open database \"%s\": %s",
        path.c_str(), db_strerror(ret));
  }
}

bool BDB::get(const DBT *key, DBT *value) {
  int ret;

  assert_open();

  memset(value, 0, sizeof(*value));

  ret = m_db->get(m_db, NULL, (DBT *)key, value, 0);
  if (ret != 0) {
    return false;
  }

  return true;
}

void BDB::put(const DBT *key, const DBT *value, uint32_t flags) {
  int ret;

  assert_open();

  ret = m_db->put(m_db, NULL, (DBT *)key, (DBT *)value, flags);
  if (ret != 0) {
    BDB_THROW("Failed to put key \"%.*s\": %s",
        (int)key->size, (char *)key->data, db_strerror(ret));
  }
}

void BDB::del(const DBT *key) {
  int ret;

  assert_open();

  ret = m_db->del(m_db, NULL, (DBT *)key, 0);
  if (ret != 0) {
    BDB_THROW("Failed to del key \"%.*s\": %s",
        (int)key->size, (char *)key->data, db_strerror(ret));
  }
}

void BDB::close() {
  assert_open();

  m_db->close(m_db, 0);
  m_db = NULL;
}

BDB::~BDB() {
  if (m_db != NULL) {
    this->close();
  }
}

}
