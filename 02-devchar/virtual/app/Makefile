#如果没有值则赋值 “?=” 如果没有值则赋值
ARCH ?= arm64

PROJECT := app.out

pwd = $(shell pwd)

C_SRCS += \
	app.c 

all:$(PROJECT)
# 链接可执行文件
$(PROJECT):$(OBJ) 
	$(CC) -o  $@ $^ $(C_SRCS)

# 编译源文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

