#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"
#include "util_md5.h"
#include "apr_tables.h"
#include "apr_strings.h"

#include "config.h"
#include "metatile.h"

/**
 * config structure
 */
typedef struct {
    char tilepath[1024];
    int etag;
} metatile_config;

/**
 * Prototypes
 */
static int metatile_handler(request_rec *r);
const char *metatile_set_tilepath(cmd_parms *cmd, void *cfg, const char *arg);
const char *metatile_set_etag(cmd_parms *cmd, void *cfg, const char *arg);
static void *create_dir_conf(apr_pool_t *pool, char *metatilePath, char *etagValue);
static void register_hooks(apr_pool_t *pool);

/**
 * configuration
 */
static const command_rec metatile_directives[] = {
    AP_INIT_TAKE1("MetatilePath", metatile_set_tilepath, NULL, ACCESS_CONF, "Set root path of metatiles"),
    AP_INIT_TAKE1("MetatileEtag", metatile_set_etag, NULL, ACCESS_CONF, "Wheter Etags should be generated or not."),
    {NULL}
};

/**
 * module name tag
 */
module AP_MODULE_DECLARE_DATA  metatile_module = {
    STANDARD20_MODULE_STUFF,
    create_dir_conf,        // Per-directory configuration handler
    NULL,                   // Merge handler for per-directory configurations
    NULL,                   // Per-server configuration handler
    NULL,                   // Merge handler for per-server configurations
    metatile_directives,    // Any directives we may have for httpd
    register_hooks          // Our hook registering function
};

/**
 *  Register hooks
 */
static void register_hooks(apr_pool_t *pool) {
    ap_hook_handler(metatile_handler, NULL, NULL, APR_HOOK_LAST);
}

/**
 * request handler
 */
static int metatile_handler(request_rec *r) {
    if (!r->handler || !strcmp(r->handler, "metatile-handler")) {
        return (DECLINED);
    }
    char extension[256];
    int x, y, z, n, len;
    n = sscanf(r->path_info, "/%d/%d/%d.%255[a-z]", &z, &x, &y, extension);
    if (n < 4) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "Invalid path for tilelayer %s:", r->path_info);
        return DECLINED;
    }
    metatile_config *config = (metatile_config *) ap_get_module_config(r->per_dir_config, &metatile_module);
    if (!strcmp(config->tilepath, PATH_NOT_SET)) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "Tilepath not set. Aborting. %s", r->path_info);
        return DECLINED;
    }
    char *tileBuffer;
    char err_msg[META_PATH_MAX];

    tileBuffer = apr_pcalloc(r->pool, (apr_size_t)MAX_TILE_SIZE);
    len = file_tile_read(config->tilepath, x, y, z, tileBuffer, MAX_TILE_SIZE, err_msg);
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Read tile of length %i for x/y/z: %i / %i / %i", len, x, y, z);
    if (len > 0) {
        if (config->etag > 0) {
            char *md5;
            md5 = ap_md5_binary(r->pool, (unsigned char *)tileBuffer, len);
            apr_table_setn(r->headers_out, "ETag", apr_psprintf(r->pool, "\"%s\"", md5));
        }
        ap_set_content_type(r, "image/png");
        ap_set_content_length(r, len);
        ap_rwrite(tileBuffer, len, r);
        free(tileBuffer);
        return OK;
    }
    free(tileBuffer);
    ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "Failed to read tile from disk: %s", err_msg);
    return DECLINED;
}

/**
 *  configuration directive handlers
 */
const char *metatile_set_tilepath(cmd_parms *cmd, void *cfg, const char *arg) {
    metatile_config *config = (metatile_config *) cfg;
    if (config) {
        strcpy(config->tilepath, arg);
    }
    return NULL;
};

const char *metatile_set_etag(cmd_parms *cmd, void *cfg, const char *arg) {
    metatile_config *config = (metatile_config *) cfg;
    if (config) {
        config->etag = (strcmp(arg, "On") || strcmp(arg, "ON")) ? 1 : 0;
    }
    return NULL;
};

/**
 * create new config per-directory
 */
static void *create_dir_conf(apr_pool_t *pool, char *metatilePath, char *etagValue) {
    metatile_config *config = apr_pcalloc(pool, sizeof(metatile_config));

    if(config) {
        config->etag = (strcmp(etagValue, "On") || strcmp(etagValue, "ON")) ? 1 : 0;
        strcpy(config->tilepath, metatilePath ? metatilePath : PATH_NOT_SET);
    }
    return config;
}