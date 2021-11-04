just for fun~

        location /dqd {

                dqd_string "Dongqiudi";
                dqd_counter on;
                dqd_filter on;
                dqd_filter_string "T.T:";
        }


curl -i http://127.0.0.1:8080/dqd
HTTP/1.1 200 OK
Server: nginx/1.8.1
Date: Thu, 04 Nov 2021 02:41:29 GMT
Content-Type: text/html
Content-Length: 29
Connection: keep-alive

T.T:Dongqiudi Visited Times:1
