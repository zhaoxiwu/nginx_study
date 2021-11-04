just for fun~
      
	location /dqd {

                dqd_string "Dongqiudi";
                dqd_counter on;
        }



curl -i http://127.0.0.1:8080/dqd

HTTP/1.1 200 OK

Server: nginx/1.8.1

Date: Thu, 04 Nov 2021 02:39:35 GMT

Content-Type: text/html

Content-Length: 25

Connection: keep-alive

Dongqiudi Visited Times:1
