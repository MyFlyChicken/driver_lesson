ARCH = arm64

CFLAGS = -Wall -g           # 编译选项
TARGET = app         		# 可执行文件名
SRCS = app.c 				# 源文件
OBJS = $(SRCS:.c=.o)        # 目标文件

pwd = $(shell pwd) 

all:
# 链接可执行文件
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
