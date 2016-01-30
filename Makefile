.PHONY: clean distclean all

obj := main.o
obj += json.o
obj += action.o
obj += json.o
obj += online.o 
obj += client.o
obj += push.o

TARGET  = wechat-server
CFLAGS  = -I /usr/include/mysql/
LDFLAGS = -L /lib/ -lmysqlclient

all: $(TARGET)

$(TARGET): $(obj)
	gcc -o $@ $^ $(LDFLAGS)

%.o:%.c
	gcc -c $< $(CFLAGS)

clean:
	rm -rf *.o wechat-server

distclean:
	rm -rf *.o wechat-server *~

