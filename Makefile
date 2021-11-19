all:
	gcc webserver.c
	./a.out
net:
	netstat -tulpn | grep :8080
ps:
	ps aux | grep a.out
