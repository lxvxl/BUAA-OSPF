# 项目名称
TARGET = myOspf

# 编译器
CXX = g++

# 编译器选项
CXXFLAGS = -Iinclude -std=c++17 -Wall -Wextra

LDFLAGS = -pthread

# 源文件目录
SRC_DIR = src

# 查找所有的源文件
SRCS = $(wildcard $(SRC_DIR)/*/*.cpp) $(SRC_DIR)/main.cpp

# 生成对应的对象文件名
OBJS = $(SRCS:.cpp=.o)

# 链接目标
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# 编译每个源文件到对象文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
