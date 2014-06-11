<?hh

<<__Native>>
function geoip_database_info(int $database = GEOIP_COUNTRY_EDITION): mixed;

<<__Native>>
function geoip_country_code_by_name(string $hostname): mixed;
<<__Native>>
function geoip_country_code3_by_name(string $hostname): mixed;
<<__Native>>
function geoip_country_name_by_name(string $hostname): mixed;

<<__Native>>
function geoip_continent_code_by_name(string $hostname): mixed;

<<__Native>>
function geoip_org_by_name(string $hostname): mixed;

<<__Native>>
function geoip_isp_by_name(string $hostname): mixed;

<<__Native>>
function geoip_asnum_by_name(string $hostname): mixed;

<<__Native>>
function geoip_domain_by_name(string $hostname): mixed;

<<__Native>>
function geoip_id_by_name(string $hostname): int;

<<__Native>>
function geoip_region_by_name(string $hostname): mixed;

<<__Native>>
function geoip_record_by_name(string $hostname): mixed;
