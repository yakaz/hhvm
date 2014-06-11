#include <GeoIP.h>
#include <GeoIPCity.h>

#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/base/ini-setting.h"

namespace HPHP {

struct GEOIPGlobals final : RequestEventHandler {
  std::string custom_directory;
  bool set_runtime_custom_directory;

  GEOIPGlobals() {}

  void requestInit() override {
    custom_directory = "";
    set_runtime_custom_directory = true;
  }

  void requestShutdown() override {}
};

IMPLEMENT_STATIC_REQUEST_LOCAL(GEOIPGlobals, s_geoip_globals);
#define GEOIPG(name) s_geoip_globals->name

#ifdef HAVE_CUSTOM_DIRECTORY
static void geoip_change_custom_directory(const String& value)
{
#if LIBGEOIP_VERSION >= 1004007
  GeoIP_cleanup();
#else
  int i;
  if (GeoIPDBFileName != NULL) {
    for (i = 0; i < NUM_DB_TYPES; i++) {
      if (GeoIPDBFileName[i]) {
        free(GeoIPDBFileName[i]);
      }
    }
    free(GeoIPDBFileName);
    GeoIPDBFileName = NULL;
  }
#endif

  if (value.length() == 0)
    return;

  GeoIP_setup_custom_directory((char *)value.c_str());
  GeoIP_db_avail(GEOIP_COUNTRY_EDITION);
}

static bool ini_on_update_custom_directory(const std::string& value) {
  if (value.length() == 0)
    return true;

  GEOIPG(set_runtime_custom_directory) = true;
  geoip_change_custom_directory(value);

  return true;
}
#endif

static Variant HHVM_FUNCTION(geoip_database_info, int64_t database) {
  GeoIP *gi;
  char *db_info;
  String ret;

  if (database < 0 || database >= NUM_DB_TYPES) {
    raise_warning("Database type given is out of bound.");
    return uninit_null();
  }

  if (GeoIP_db_avail(database)) {
    gi = GeoIP_open_type(database, GEOIP_STANDARD);
  } else {
    if (NULL != GeoIPDBFileName[database])
      raise_warning("Required database not available at %s.",
          GeoIPDBFileName[database]);
    else
      raise_warning("Required database not available.");
    return uninit_null();
  }

  db_info = GeoIP_database_info(gi);
  GeoIP_delete(gi);

  ret = String(db_info);
  free(db_info);

  return ret;
}

static Variant get_const_code(const String& hostname,
    const char * (*get)(GeoIP *gi, const char *name),
    int edition) {
  GeoIP *gi;
  const char *code;
  String ret;

  if (GeoIP_db_avail(edition)) {
    gi = GeoIP_open_type(edition, GEOIP_STANDARD);
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[edition]);
    return false;
  }

  code = get(gi, hostname.c_str());
  GeoIP_delete(gi);
  if (code == NULL)
    return false;

  ret = String(code);

  return ret;
}

static Variant HHVM_FUNCTION(geoip_country_code_by_name,
                             const String& hostname) {
  return get_const_code(hostname,
      &GeoIP_country_code_by_name, GEOIP_COUNTRY_EDITION);
}

static Variant HHVM_FUNCTION(geoip_country_code3_by_name,
                             const String& hostname) {
  return get_const_code(hostname,
      &GeoIP_country_code3_by_name, GEOIP_COUNTRY_EDITION);
}

static Variant HHVM_FUNCTION(geoip_country_name_by_name,
                             const String& hostname) {
  return get_const_code(hostname,
      &GeoIP_country_name_by_name, GEOIP_COUNTRY_EDITION);
}

static Variant get_allocated_code(const String& hostname,
    char * (*get)(GeoIP *gi, const char *name),
    int edition) {
  GeoIP *gi;
  char *code;
  String ret;

  if (GeoIP_db_avail(edition)) {
    gi = GeoIP_open_type(edition, GEOIP_STANDARD);
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[edition]);
    return false;
  }

  code = get(gi, hostname.c_str());
  GeoIP_delete(gi);
  if (code == NULL)
    return false;

  ret = String(code);
  free(code);

  return ret;
}

static Variant HHVM_FUNCTION(geoip_org_by_name, const String& hostname) {
  return get_allocated_code(hostname,
      &GeoIP_org_by_name, GEOIP_ORG_EDITION);
}

static Variant HHVM_FUNCTION(geoip_isp_by_name, const String& hostname) {
  return get_allocated_code(hostname,
      &GeoIP_name_by_name, GEOIP_ISP_EDITION);
}

static Variant HHVM_FUNCTION(geoip_asnum_by_name, const String& hostname) {
  return get_allocated_code(hostname,
      &GeoIP_name_by_name, GEOIP_ASNUM_EDITION);
}

static Variant HHVM_FUNCTION(geoip_domain_by_name, const String& hostname) {
  return get_allocated_code(hostname,
      &GeoIP_name_by_name, GEOIP_DOMAIN_EDITION);
}

static Variant HHVM_FUNCTION(geoip_continent_code_by_name,
                             const String& hostname) {
  GeoIP *gi;
  int id;
  String ret;

  if (GeoIP_db_avail(GEOIP_COUNTRY_EDITION)) {
    gi = GeoIP_open_type(GEOIP_COUNTRY_EDITION, GEOIP_STANDARD);
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
    return false;
  }

  id = GeoIP_id_by_name(gi, hostname.c_str());
  GeoIP_delete(gi);
  if (id == 0)
    return false;

  ret = String(GeoIP_country_continent[id]);
  return ret;
}

static int64_t HHVM_FUNCTION(geoip_id_by_name, const String& hostname) {
  GeoIP *gi;
  int64_t ret;

  if (GeoIP_db_avail(GEOIP_NETSPEED_EDITION)) {
    gi = GeoIP_open_type(GEOIP_NETSPEED_EDITION, GEOIP_STANDARD);
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[GEOIP_NETSPEED_EDITION]);
    return GEOIP_UNKNOWN_SPEED;
  }

  ret = GeoIP_id_by_name(gi, hostname.c_str());
  GeoIP_delete(gi);

  return ret;
}

const StaticString
  s_continent_code("continent_code"),
  s_country_code("country_code"),
  s_country_code3("country_code3"),
  s_country_name("country_name"),
  s_region("region"),
  s_city("city"),
  s_postal_code("postal_code"),
  s_latitude("latitude"),
  s_longitude("longitude"),
  s_dma_code("dma_code"),
  s_area_code("area_code");

static Variant HHVM_FUNCTION(geoip_region_by_name, const String& hostname) {
  GeoIP *gi;
  GeoIPRegion *region;
  Array ret;

  if (GeoIP_db_avail(GEOIP_REGION_EDITION_REV0) ||
      GeoIP_db_avail(GEOIP_REGION_EDITION_REV1)) {
    if (GeoIP_db_avail(GEOIP_REGION_EDITION_REV1))
      gi = GeoIP_open_type(GEOIP_REGION_EDITION_REV1, GEOIP_STANDARD);
    else
      gi = GeoIP_open_type(GEOIP_REGION_EDITION_REV0, GEOIP_STANDARD);
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[GEOIP_REGION_EDITION_REV0]);
    return false;
  }

  region = GeoIP_region_by_name(gi, hostname.c_str());
  GeoIP_delete(gi);
  if (NULL == region)
    return false;

  ret.set(s_country_code, String(region->country_code));
  ret.set(s_region, String(region->region));

  GeoIPRegion_delete(region);

  return ret;
}

static Variant HHVM_FUNCTION(geoip_record_by_name, const String& hostname) {
  GeoIP *gi;
  GeoIPRecord *gir;
  Array ret;

  if (GeoIP_db_avail(GEOIP_CITY_EDITION_REV1) ||
      GeoIP_db_avail(GEOIP_CITY_EDITION_REV0)) {
    if (GeoIP_db_avail(GEOIP_CITY_EDITION_REV1)) {
      gi = GeoIP_open_type(GEOIP_CITY_EDITION_REV1, GEOIP_STANDARD);
    } else {
      gi = GeoIP_open_type(GEOIP_CITY_EDITION_REV0, GEOIP_STANDARD);
    }
  } else {
    raise_warning("Required database not available at %s.",
        GeoIPDBFileName[GEOIP_CITY_EDITION_REV0]);
    return false;
  }

  gir = GeoIP_record_by_name(gi, hostname.c_str());

  GeoIP_delete(gi);

  if (NULL == gir) {
    return false;
  }

#if LIBGEOIP_VERSION >= 1004003
  ret.set(s_continent_code,
      (gir->continent_code == NULL) ? "" : String(gir->continent_code));
#endif
  ret.set(s_country_code,
      (gir->country_code == NULL) ? "" : String(gir->country_code));
  ret.set(s_country_code3,
      (gir->country_code3 == NULL) ? "" : String(gir->country_code3));
  ret.set(s_country_name,
      (gir->country_name == NULL) ? "" : String(gir->country_name));
  ret.set(s_region,
      (gir->region == NULL) ? "" : String(gir->region));
  ret.set(s_city,
      (gir->city == NULL) ? "" : String(gir->city));
  ret.set(s_postal_code,
      (gir->postal_code == NULL) ? "" : String(gir->postal_code));

  ret.set(s_latitude, gir->latitude);
  ret.set(s_longitude, gir->longitude);

#if LIBGEOIP_VERSION >= 1004005
  ret.set(s_dma_code, gir->metro_code);
#else
  ret.set(s_dma_code, gir->dma_code);
#endif
  ret.set(s_area_code, gir->area_code);

  GeoIPRecord_delete(gir);

  return ret;
}

const int64_t k_GEOIP_COUNTRY_EDITION = GEOIP_COUNTRY_EDITION;
const int64_t k_GEOIP_REGION_EDITION_REV0 = GEOIP_REGION_EDITION_REV0;
const int64_t k_GEOIP_CITY_EDITION_REV0 = GEOIP_CITY_EDITION_REV0;
const int64_t k_GEOIP_ORG_EDITION = GEOIP_ORG_EDITION;
const int64_t k_GEOIP_ISP_EDITION = GEOIP_ISP_EDITION;
const int64_t k_GEOIP_CITY_EDITION_REV1 = GEOIP_CITY_EDITION_REV1;
const int64_t k_GEOIP_REGION_EDITION_REV1 = GEOIP_REGION_EDITION_REV1;
const int64_t k_GEOIP_PROXY_EDITION = GEOIP_PROXY_EDITION;
const int64_t k_GEOIP_ASNUM_EDITION = GEOIP_ASNUM_EDITION;
const int64_t k_GEOIP_NETSPEED_EDITION = GEOIP_NETSPEED_EDITION;
const int64_t k_GEOIP_DOMAIN_EDITION = GEOIP_DOMAIN_EDITION;
#if LIBGEOIP_VERSION >= 1004008
const int64_t k_GEOIP_NETSPEED_EDITION_REV1 = GEOIP_NETSPEED_EDITION_REV1;
#endif
const int64_t k_GEOIP_UNKNOWN_SPEED = GEOIP_UNKNOWN_SPEED;
const int64_t k_GEOIP_DIALUP_SPEED = GEOIP_DIALUP_SPEED;
const int64_t k_GEOIP_CABLEDSL_SPEED = GEOIP_CABLEDSL_SPEED;
const int64_t k_GEOIP_CORPORATE_SPEED = GEOIP_CORPORATE_SPEED;

const StaticString s_GEOIP_COUNTRY_EDITION("GEOIP_COUNTRY_EDITION");
const StaticString s_GEOIP_REGION_EDITION_REV0("GEOIP_REGION_EDITION_REV0");
const StaticString s_GEOIP_CITY_EDITION_REV0("GEOIP_CITY_EDITION_REV0");
const StaticString s_GEOIP_ORG_EDITION("GEOIP_ORG_EDITION");
const StaticString s_GEOIP_ISP_EDITION("GEOIP_ISP_EDITION");
const StaticString s_GEOIP_CITY_EDITION_REV1("GEOIP_CITY_EDITION_REV1");
const StaticString s_GEOIP_REGION_EDITION_REV1("GEOIP_REGION_EDITION_REV1");
const StaticString s_GEOIP_PROXY_EDITION("GEOIP_PROXY_EDITION");
const StaticString s_GEOIP_ASNUM_EDITION("GEOIP_ASNUM_EDITION");
const StaticString s_GEOIP_NETSPEED_EDITION("GEOIP_NETSPEED_EDITION");
const StaticString s_GEOIP_DOMAIN_EDITION("GEOIP_DOMAIN_EDITION");
#if LIBGEOIP_VERSION >= 1004008
const StaticString s_GEOIP_NETSPEED_EDITION_REV1("GEOIP_NETSPEED_EDITION_REV1");
#endif
const StaticString s_GEOIP_UNKNOWN_SPEED("GEOIP_UNKNOWN_SPEED");
const StaticString s_GEOIP_DIALUP_SPEED("GEOIP_DIALUP_SPEED");
const StaticString s_GEOIP_CABLEDSL_SPEED("GEOIP_CABLEDSL_SPEED");
const StaticString s_GEOIP_CORPORATE_SPEED("GEOIP_CORPORATE_SPEED");

class geoipExtension : public Extension {
 public:
  geoipExtension() : Extension("geoip", "0.1.0") {}

  virtual void moduleInit() {
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_COUNTRY_EDITION.get(), k_GEOIP_COUNTRY_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_REGION_EDITION_REV0.get(), k_GEOIP_REGION_EDITION_REV0
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_CITY_EDITION_REV0.get(), k_GEOIP_CITY_EDITION_REV0
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_ORG_EDITION.get(), k_GEOIP_ORG_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_ISP_EDITION.get(), k_GEOIP_ISP_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_CITY_EDITION_REV1.get(), k_GEOIP_CITY_EDITION_REV1
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_REGION_EDITION_REV1.get(), k_GEOIP_REGION_EDITION_REV1
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_PROXY_EDITION.get(), k_GEOIP_PROXY_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_ASNUM_EDITION.get(), k_GEOIP_ASNUM_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_NETSPEED_EDITION.get(), k_GEOIP_NETSPEED_EDITION
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_DOMAIN_EDITION.get(), k_GEOIP_DOMAIN_EDITION
    );
#if LIBGEOIP_VERSION >= 1004008
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_NETSPEED_EDITION_REV1.get(), k_GEOIP_NETSPEED_EDITION_REV1
    );
#endif
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_UNKNOWN_SPEED.get(), k_GEOIP_UNKNOWN_SPEED
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_DIALUP_SPEED.get(), k_GEOIP_DIALUP_SPEED
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_CABLEDSL_SPEED.get(), k_GEOIP_CABLEDSL_SPEED
    );
    Native::registerConstant<KindOfInt64>(
      s_GEOIP_CORPORATE_SPEED.get(), k_GEOIP_CORPORATE_SPEED
    );

    HHVM_FE(geoip_database_info);
    HHVM_FE(geoip_country_code_by_name);
    HHVM_FE(geoip_country_code3_by_name);
    HHVM_FE(geoip_country_name_by_name);
    HHVM_FE(geoip_continent_code_by_name);
    HHVM_FE(geoip_org_by_name);
    HHVM_FE(geoip_record_by_name);
    HHVM_FE(geoip_id_by_name);
    HHVM_FE(geoip_region_by_name);
    HHVM_FE(geoip_isp_by_name);
#if 0
    HHVM_FE(geoip_db_avail);
    HHVM_FE(geoip_db_get_all_info);
    HHVM_FE(geoip_db_filename);
#if LIBGEOIP_VERSION >= 1004001
    HHVM_FE(geoip_region_name_by_code);
    HHVM_FE(geoip_time_zone_by_country_and_region);
#endif
#ifdef HAVE_CUSTOM_DIRECTORY
    HHVM_FE(geoip_setup_custom_directory);
#endif
#endif
    HHVM_FE(geoip_asnum_by_name);
    HHVM_FE(geoip_domain_by_name);
#if 0
#if LIBGEOIP_VERSION >= 1004008
    HHVM_FE(geoip_netspeedcell_by_name);
#endif
#endif

    loadSystemlib();

#ifdef HAVE_CUSTOM_DIRECTORY
    IniSetting::Bind(this, IniSetting::PHP_INI_ALL,
        "geoip.custom_directory", "",
        IniSetting::SetAndGet<std::string>(
          ini_on_update_custom_directory,
          nullptr),
        &GEOIPG(custom_directory));
#endif

    /* This will initialize file structure */
    GeoIP_db_avail(GEOIP_COUNTRY_EDITION);
  }

  virtual void moduleShutdown() {
    /* Calling this with an empty string will only perform cleanup. */
    geoip_change_custom_directory("");
  }
} s_geoip_extension;

// Uncomment for non-bundled module
//HHVM_GET_MODULE(geoip);

//////////////////////////////////////////////////////////////////////////////
} // namespace HPHP
