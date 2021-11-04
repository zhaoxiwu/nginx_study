## 上节回顾：
1、nginx处理请求流程：ngx_http_process_request_line-->ngx_http_process_request_headers->ngx_http_request_handler-->ngx_http_core_run_phase->ngx_http_xxx_filter

2、nginx扩展四要素，module，context，command，conf。以及他们之间的包含关系。 

## filter模块的作用：
过滤（filter）模块是过滤响应头和内容的模块，可以对回复的头和内容进行处理。它的处理时间在获取回复内容之后，向用户发送响应之前。它的处理过程分为两个阶段，过滤HTTP回复的头部和主体，在这两个阶段可以分别对头部和主体进行修改。 

## filter执行顺序：
之前提到过nginx编译时会加载哪些模块写在了objs/ngx_modules.c里面。 filter模块的顺序其实也是在这个文件里定义好的。我们看下nginx里面都有哪些filter模块。
  
```
ngx_module_t *ngx_modules[] = {
    &ngx_core_module,
    &ngx_errlog_module,
    &ngx_conf_module,
    .
    .
    .
    &ngx_http_upstream_keepalive_module,
    &ngx_http_dqd_module,
    &ngx_http_write_filter_module,
    &ngx_http_header_filter_module,
    &ngx_http_chunked_filter_module,
    &ngx_http_range_header_filter_module,
    &ngx_http_gzip_filter_module,
    &ngx_http_postpone_filter_module,
    &ngx_http_ssi_filter_module,
    &ngx_http_charset_filter_module,
    &ngx_http_userid_filter_module,
    &ngx_http_headers_filter_module,
    &ngx_http_dqd_filter_module,
    &ngx_http_copy_filter_module,
    &ngx_http_range_body_filter_module,
    &ngx_http_not_modified_filter_module,
    NULL
};
```

从上面文件中可以看到nginx其实预加载了很多filter模块，他们其实是通过一个单行链表串联起来的，每加载一个模块就往链表头挂一个。所以在后面执行的时候其实是先执行最后加载的那个模块。另外需要特别注意的ngx_http_write_filter_module 和ngx_http_header_filter_module必须最后执行。整个执行过程如下图所示：先处理完所有header相关filter，在进行body的filter的处理。



细心的同学应该还记得我们第一个扩展ngx_http_dqd_module的handler中拼装好给客户端的返回数据后，调用了ngx_http_send_header(r) 和 ngx_http_output_filter(r, &out)。 这2个函数其实就是filter的入口，一个是header filter 入口，一个是body filter入口。通过这2个filter最终把数据发送给客户端。
  ```
  /* send the headers of your response */
        rc = ngx_http_send_header(r);

        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
                return rc;
        }

        /* send the buffer chain of your response */
        return ngx_http_output_filter(r, &out);

```
一起看下ngx_http_send_header() 的实现逻辑, 代码位置src/http/ngx_http_core_module.c。 整个很简单做一些基本判断后直接调用ngx_http_top_header_filter，它是一个函数指针也是整个单链表的头指针。 body也是同样的逻辑（ngx_http_top_body_filter），就不累述了。
```
ngx_int_t
ngx_http_send_header(ngx_http_request_t *r)
{
    if (r->post_action) {
        return NGX_OK;
    }

    if (r->header_sent) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "header already sent");
        return NGX_ERROR;
    }

    if (r->err_status) {
        r->headers_out.status = r->err_status;
        r->headers_out.status_line.len = 0;
    }

    return ngx_http_top_header_filter(r);
}

ngx_int_t
ngx_http_output_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = r->connection;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http output filter \"%V?%V\"", &r->uri, &r->args);

    rc = ngx_http_top_body_filter(r, in);

    if (rc == NGX_ERROR) {
        /* NGX_ERROR may be returned by any filter */
        c->error = 1;
    }

    return rc;
}
```

## filter扩展开发：
### 语法：

       location /dqd {

                dqd_string "Dongqiudi";
                dqd_counter on;
                dqd_filter on;
                dqd_filter_string "T.T:";
        }

这次主要是和ngx_http_dqd_module模块一起配合使用，当然如果有其他的location符合该模块要求也可以使用。配置好nginx后，尝试发起请求，会看到response body前面中多了 "T.T:"。 
```
curl -i http://127.0.0.1:8080/dqd
        
 HTTP/1.1 200 OK
 Server: nginx/1.8.1
 Date: Thu, 04 Nov 2021 02:41:29 GMT
 Content-Type: text/html
 Content-Length: 29
 Connection: keep-alive

 T.T:Dongqiudi Visited Times:1
```
### 编译：
config文件：
```
ngx_addon_name=ngx_http_dqd_filter_module
HTTP_AUX_FILTER_MODULES="$HTTP_AUX_FILTER_MODULES ngx_http_dqd_filter_module"
NGX_ADDON_SRCS="$NGX_ADDON_SRCS $ngx_addon_dir/ngx_http_dqdfilter_module.c"
```
编写config文件是需要注意：filter module 需要加到HTTP_AUX_FILTER_MODULES里面，这个跟ngx_http_xxx_module 不一样。还有一种filter模块：HTTP_FILTER_MODULES， 其实ngx内置的filter模块都在这个里面，官方不建议三方开发的filter module模块挂载到这个上面。主要是这个里面的module都支持subrequests（Re: Wha difference between HTTP_FILTER_MODULES and HTTP_AUX_FILTER_MODULES is it? https://forum.nginx.org/read.php?2,152699,152717）。 其实加到HTTP_FILTER_MODULES里面也是可以的，感兴趣的可以试试。

看一下nginx的编译脚本这块的实现逻辑：
```
if [ $HTTP = YES ]; then
    modules="$modules $HTTP_MODULES $HTTP_FILTER_MODULES \
             $HTTP_HEADERS_FILTER_MODULE \
             $HTTP_AUX_FILTER_MODULES \
             $HTTP_COPY_FILTER_MODULE \
             $HTTP_RANGE_BODY_FILTER_MODULE \
             $HTTP_NOT_MODIFIED_FILTER_MODULE"

    NGX_ADDON_DEPS="$NGX_ADDON_DEPS \$(HTTP_DEPS)"
fi
```
从上面的代码可以看到，扩展的HTTP_AUX_FILTER_MODULES都会加到copy_filter 和headers_filter中间。 编译后的ngx_modules.c 可以看到模块已经加进去了。 
```
ngx_module_t *ngx_modules[] = {
    .
    .
    .
    &ngx_http_postpone_filter_module,
    &ngx_http_ssi_filter_module,
    &ngx_http_charset_filter_module,
    &ngx_http_userid_filter_module,
    &ngx_http_headers_filter_module,
    &ngx_http_dqd_filter_module,
    &ngx_http_copy_filter_module,
    &ngx_http_range_body_filter_module,
    &ngx_http_not_modified_filter_module,
    NULL
};
```

### 源码分析：
我们知道了nginx在处理filter时是一个单项链表，那么模块是如何把自己加到链表里的呢，我们接着看代码：
```
static ngx_int_t
ngx_http_dqd_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_dqd_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_dqd_body_filter;

    return NGX_OK;
}
```
从上面的代码块，其实逻辑也很简单，模块本身会定义一个ngx_http_next_header_filter的函数指针指向当前链表最前端的模块，然后该列表的头指针指向自己的filter实现。 header和body处理逻辑一样，所以也比较好的解释了为什么最先加载的模块最后执行。

我们在看下第一个加载的filter模块(src/http/ngx_http_header_filter_module.c)的链表挂载实现逻辑，因为它处于整个filter单向链表的末端所以么有next指针。初始化时只修改了ngx_http_top_header_filter指针。
```
static ngx_int_t
ngx_http_header_filter_init(ngx_conf_t *cf)
{
    ngx_http_top_header_filter = ngx_http_header_filter;

    return NGX_OK;
}
```

好，因为这个模块的功能是在消息返回体的最前端增加一部分数据。所以会涉及到header中content_length的修改， 和返回消息体的修改。因此这个扩展需要实现header_filter 和body_filter2部分。

我们先看header部分的逻辑，一系列各种检查后content_length修改为原值+新增字符串的长度。然后把request交给下一个header filter模块进行处理。
```
static ngx_int_t
ngx_http_dqd_header_filter(ngx_http_request_t *r){
       .
       .
       .
        if (mlcf->dqd_string.len >=0 ) {
                dctx->flag = 1;
                r->headers_out.content_length_n += mlcf->dqd_string.len;
        }
        return ngx_http_next_header_filter(r);
}
```

同理，body filter也比较简单，从配置中获取需要配件的字符串拼接到body最前面，然后交给下个body filter继续处理后续逻辑。
```
static ngx_int_t
ngx_http_dqd_body_filter(ngx_http_request_t *r, ngx_chain_t *in){
        .
        .
        .
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
```

这样所有content_type类型是"text/html"类型的请求，返回值都会多了dqd_filter_string配置的内容啦， 大家也可以尝试把content_type做成配置项试一试。
