#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct
{
        ngx_str_t dqd_string;
        ngx_int_t dqd_counter;
}ngx_http_dqd_loc_conf_t;

static ngx_int_t ngx_http_dqd_init(ngx_conf_t *cf);

static void *ngx_http_dqd_create_loc_conf(ngx_conf_t *cf);

//static char *ngx_http_dqd_string(ngx_conf_t *cf, ngx_command_t *cmd,
//        void *conf);
//static char *ngx_http_dqd_counter(ngx_conf_t *cf, ngx_command_t *cmd,
//        void *conf);

static ngx_command_t ngx_http_dqd_commands[] = {
 	{
                ngx_string("dqd_string"),
                NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
//                ngx_http_dqd_string,
		ngx_conf_set_str_slot,              
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_dqd_loc_conf_t, dqd_string),
                NULL },

        {
                ngx_string("dqd_counter"),
                NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
//                ngx_http_dqd_counter,
		ngx_conf_set_flag_slot,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_dqd_loc_conf_t, dqd_counter),
                NULL },

        ngx_null_command
};

static int 
ngx_dqd_visited_times = 0;

static ngx_http_module_t ngx_http_dqd_module_ctx = {
        NULL,                          /* preconfiguration */
        ngx_http_dqd_init,           /* postconfiguration */

        NULL,                          /* create main configuration */
        NULL,                          /* init main configuration */

        NULL,                          /* create server configuration */
        NULL,                          /* merge server configuration */

        ngx_http_dqd_create_loc_conf, /* create location configuration */
        NULL                            /* merge location configuration */
};


ngx_module_t ngx_http_dqd_module = {
        NGX_MODULE_V1,
        &ngx_http_dqd_module_ctx,    /* module context */
        ngx_http_dqd_commands,       /* module directives */
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


static ngx_int_t
ngx_http_dqd_handler(ngx_http_request_t *r)
{
        ngx_int_t    rc;
        ngx_buf_t   *b;
        ngx_chain_t  out;
        ngx_http_dqd_loc_conf_t* my_conf;
        u_char ngx_dqd_string[1024] = {0};
        ngx_uint_t content_length = 0;

        ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "ngx_http_dqd_handler is called!");

        my_conf = ngx_http_get_module_loc_conf(r, ngx_http_dqd_module);
        if (my_conf->dqd_string.len == 0 )
        {
                ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "dqd_string is empty!");
                return NGX_DECLINED;
        }


        if (my_conf->dqd_counter == NGX_CONF_UNSET
                || my_conf->dqd_counter == 0)
        {
                ngx_sprintf(ngx_dqd_string, "%s", my_conf->dqd_string.data);
        }
        else
        {
                ngx_sprintf(ngx_dqd_string, "%s Visited Times:%d", my_conf->dqd_string.data,
                        ++ngx_dqd_visited_times);
        }
        ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "dqd_string:%s", ngx_dqd_string);
        content_length = ngx_strlen(ngx_dqd_string);

        /* we response to 'GET' and 'HEAD' requests only */
        if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
                return NGX_HTTP_NOT_ALLOWED;
        }

        /* discard request body, since we don't need it here */
        rc = ngx_http_discard_request_body(r);

        if (rc != NGX_OK) {
                return rc;
        }

        /* set the 'Content-type' header */
        /*
         *r->headers_out.content_type.len = sizeof("text/html") - 1;
         *r->headers_out.content_type.data = (u_char *)"text/html";
         */
        ngx_str_set(&r->headers_out.content_type, "text/html");


        /* send the header only, if the request type is http 'HEAD' */
        if (r->method == NGX_HTTP_HEAD) {
                r->headers_out.status = NGX_HTTP_OK;
                r->headers_out.content_length_n = content_length;

                return ngx_http_send_header(r);
        }

        /* allocate a buffer for your response body */
        b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
        if (b == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        /* attach this buffer to the buffer chain */
        out.buf = b;
        out.next = NULL;

        /* adjust the pointers of the buffer */
        b->pos = ngx_dqd_string;
        b->last = ngx_dqd_string + content_length;
        b->memory = 1;    /* this buffer is in memory */
        b->last_buf = 1;  /* this is the last buffer in the buffer chain */

        /* set the status line */
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = content_length;

        /* send the headers of your response */
        rc = ngx_http_send_header(r);

        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
                return rc;
        }

        /* send the buffer chain of your response */
        return ngx_http_output_filter(r, &out);
}

static void *ngx_http_dqd_create_loc_conf(ngx_conf_t *cf)
{
        ngx_http_dqd_loc_conf_t* local_conf = NULL;
        local_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_dqd_loc_conf_t));
        if (local_conf == NULL)
        {
                return NULL;
        }

        ngx_str_null(&local_conf->dqd_string);
        local_conf->dqd_counter = NGX_CONF_UNSET;

        return local_conf;
}


//static char *
//ngx_http_dqd_string(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
//{
//
//        ngx_http_dqd_loc_conf_t* local_conf;
//
//
//        local_conf = conf;
//        char* rv = ngx_conf_set_str_slot(cf, cmd, conf);
//
//        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "dqd_string:%s", local_conf->dqd_string.data);
//
//        return rv;
//}
//
//
//static char *ngx_http_dqd_counter(ngx_conf_t *cf, ngx_command_t *cmd,
//        void *conf)
//{
//        ngx_http_dqd_loc_conf_t* local_conf;
//
//        local_conf = conf;
//
//        char* rv = NULL;
//
//        rv = ngx_conf_set_flag_slot(cf, cmd, conf);
//
//
//        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "dqd_counter:%d", local_conf->dqd_counter);
//        return rv;
//}
//
static ngx_int_t
ngx_http_dqd_init(ngx_conf_t *cf)
{
        ngx_http_handler_pt        *h;
        ngx_http_core_main_conf_t  *cmcf;

        cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

        h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
        if (h == NULL) {
                return NGX_ERROR;
        }

        *h = ngx_http_dqd_handler;

        return NGX_OK;
}
