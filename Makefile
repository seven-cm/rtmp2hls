# 指定C++编译器
XX = g++

# 编译选项：生成目标文件、调试信息、C++11标准
CXXFLAGS = -c -g -std=c++11

# 链接库：线程库、log4cxx、APR、APR-util、expat、iconv
CLIBS = -lpthread -L./3rdparty/log4cxx-install/lib -llog4cxx -L./3rdparty/apr-install/lib -lapr-1 -laprutil-1 -lexpat -liconv -Wl,-rpath,./3rdparty/log4cxx-install/lib -Wl,-rpath,./3rdparty/apr-install/lib

# 包含头文件目录
INCLUDE_DIRS = -I./src -I./3rdparty/log4cxx-install/include -I./3rdparty/apr-install/include

# 源文件目录
SRCDIR = src

# 目标文件目录
OBJDIR = objs

# 获取所有cpp源文件（包括子目录）
SRC = $(shell find $(SRCDIR) -name '*.cpp')

# 将cpp文件名转换为对应的.o文件名
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))

# 定义程序名称
PROGRAM := rtmp2hls
TARGET = $(PROGRAM)

# 默认目标：编译程序
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS)
	$(XX) -static-libstdc++ -o $@ $^ $(CLIBS)

# 编译规则：将cpp文件编译为对象文件
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(dir $@)
	$(XX) $(CXXFLAGS) $< -o $@ $(INCLUDE_DIRS)

# 清理目标：删除生成的目标文件和可执行文件
.PHONY: clean
clean:
	rm -rf $(OBJDIR)/*.o $(TARGET)

# 安装目标（已注释）
#.PHONY: install
#install:
#	install -m 755 $(TARGET) /usr/local/bin/
