#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct
{
	// 要添加的字符串
        ngx_str_t dqd_string;
	// 模块开关
        ngx_int_t dqd_filter;
}ngx_http_dqd_filter_loc_conf_t;


typedef struct
{
	// 0: header 未处理，1：body 未处理，2：body已处理
	ngx_int_t flag;
}ngx_http_dqd_filter_ctx_t;

static ngx_int_t ngx_http_dqd_filter_init(ngx_conf_t *cf);
static void *ngx_http_dqd_filter_create_loc_conf(ngx_conf_t *cf);

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_command_t ngx_http_dqd_filter_commands[] = {
 	{
                ngx_string("dqd_filter_string"),
                NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,              
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_dqd_filter_loc_conf_t, dqd_string),
                NULL },

        {
                ngx_string("dqd_filter"),
                NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_dqd_filter_loc_conf_t, dqd_filter),
                NULL },

        ngx_null_command
};

 
static ngx_http_module_t ngx_http_dqd_filter_module_ctx = {
        NULL,                          /* preconfiguration */
        ngx_http_dqd_filter_init,           /* postconfiguration */

        NULL,                          /* create main configuration */
        NULL,                          /* init main configuration */

        NULL,                          /* create server configuration */
        NULL,                          /* merge server configuration */

        ngx_http_dqd_filter_create_loc_conf, /* create location configuration */
        NULL                            /* merge location configuration */
};


ngx_module_t ngx_http_dqd_filter_module = {
        NGX_MODULE_V1,
        &ngx_http_dqd_filter_module_ctx,    /* module context */
        ngx_http_dqd_filter_commands,       /* module directives */
        NGX_HTTP_MODULE,               /* module type */
        NULL,                          /* init master */
        NULL,                          /* init module */
        NULL,                          /* init process */
        NULL,                          /* init thread */
        NULL,                          /* exit thread */
        NULL,                          /* exit process */
        NULL,                          /* exit master */
        NGX_MODULE_V1_PADDING
};



static void *ngx_http_dqd_filter_create_loc_conf(ngx_conf_t *cf)
{
        ngx_http_dqd_filter_loc_conf_t* local_conf = NULL;
        local_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_dqd_filter_loc_conf_t));
        if (local_conf == NULL)
        {
                return NULL;
        }

        ngx_str_null(&local_conf->dqd_string);
        local_conf->dqd_filter = NGX_CONF_UNSET;

        return local_conf;
}

static ngx_int_t
ngx_http_dqd_header_filter(ngx_http_request_t *r){
	if (r->headers_out.status != NGX_HTTP_OK){
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "headers_out.status = %d", r->headers_out.status);
		return ngx_http_next_header_filter(r);
	}
	
	ngx_str_t type = ngx_string("text/html");
	if (r->headers_out.content_type.len != type.len || ngx_strncasecmp(r->headers_out.content_type.data, type.data, type.len) != 0){
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "content_type is not text/html");
		return ngx_http_next_header_filter(r);
	}

	ngx_http_dqd_filter_loc_conf_t *mlcf;
	mlcf = ngx_http_get_module_loc_conf(r, ngx_http_dqd_filter_module);
	if (!mlcf->dqd_filter){
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "dqd filter is off");
		return ngx_http_next_header_filter(r);
	}

	ngx_http_dqd_filter_ctx_t *dctx;
	dctx = ngx_http_get_module_ctx(r, ngx_http_dqd_filter_module);

	if (dctx != NULL){
		return ngx_http_next_header_filter(r);
	}

	dctx = ngx_pcalloc(r->pool, sizeof(ngx_http_dqd_filter_ctx_t));
	if (dctx == NULL){
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "dqd filter ctx pcalloc faild");
		return ngx_http_next_header_filter(r);
	
	}
	ngx_http_set_ctx(r, dctx, ngx_http_dqd_filter_module);

	if (mlcf->dqd_string.len >=0 ) {
		dctx->flag = 1;	
		r->headers_out.content_length_n += mlcf->dqd_string.len;
	}
	return ngx_http_next_header_filter(r);
}

static ngx_int_t
ngx_http_dqd_body_filter(ngx_http_request_t *r, ngx_chain_t *in){
	ngx_http_dqd_filter_ctx_t *dctx;
	dctx = ngx_http_get_module_ctx(r, ngx_http_dqd_filter_module);

	if (dctx == NULL || dctx->flag != 1){
		return ngx_http_next_body_filter(r, in);
	}

	ngx_http_dqd_filter_loc_conf_t *mlcf;
	mlcf = ngx_http_get_module_loc_conf(r, ngx_http_dqd_filter_module);
	if (mlcf == NULL || mlcf->dqd_string.len == 0){
		return ngx_http_next_body_filter(r, in);
	}

	
	ngx_buf_t *b =  ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	b->pos = mlcf->dqd_string.data;
	b->last = mlcf->dqd_string.data + mlcf->dqd_string.len;
	b->memory = 1;
	b->last_buf = 1;
	
	ngx_chain_t  out;
	out.buf =  b;
	out.next = in;
	return ngx_http_next_body_filter(r, &out);
}

static ngx_int_t
ngx_http_dqd_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_dqd_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_dqd_body_filter;

    return NGX_OK;
}


