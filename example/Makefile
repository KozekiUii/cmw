# 编译器和选项
CC      := g++
CFLAGS  := -std=c++14 
DEBUG   := -g 
ROOT := $(abspath ../..)

# Third-party roots (override from env if needed)
FASTDDS_HOME ?= $(ROOT)/Fast-DDS/install
GTEST_HOME ?=

INCLUDE := -I$(FASTDDS_HOME)/include \
           -I$(ROOT) \
           -I../thirdparty/

ThirdPartyLibPath := -L$(FASTDDS_HOME)/lib

ThirdPartyLib := -lfastrtps -lfastcdr -lfoonathan_memory-0.7.3 \
                 -lpthread -lrt -latomic -ldl

RPATH := -Wl,-rpath,$(FASTDDS_HOME)/lib

# Detect whether gtest headers+libs are available; if not, skip gtest-based targets.
GTEST_HEADER := $(firstword $(wildcard /usr/include/gtest/gtest.h) $(wildcard $(GTEST_HOME)/include/gtest/gtest.h))
GTEST_LIB := $(firstword $(wildcard /usr/lib/*/libgtest*.a) $(wildcard $(GTEST_HOME)/lib/libgtest*.a) $(wildcard $(GTEST_HOME)/lib64/libgtest*.a))
GTEST_AVAILABLE := $(if $(and $(GTEST_HEADER),$(GTEST_LIB)),1,)

ifeq ($(GTEST_AVAILABLE),1)
INCLUDE += $(if $(GTEST_HOME),-I$(GTEST_HOME)/include,)
ThirdPartyLibPath += $(if $(GTEST_HOME),-L$(GTEST_HOME)/lib -L$(GTEST_HOME)/lib64,)
ThirdPartyLib += -lgtest
endif

# 可执行文件列表
BASE_EXECUTABLES := test_writer test_reader test_serialize test_node_manager \
                   test_channel_manager test_topology_manager test_publisher \
                   test_log test_getenv

GTEST_EXECUTABLES := test_croutine test_subscriber test_publisher_subscriber \
                    test_task test_blocker test_scheduler test_class_loader test_node

EXECUTABLES := $(BASE_EXECUTABLES) $(if $(GTEST_AVAILABLE),$(GTEST_EXECUTABLES),)

EXECUTABLES_OUT := $(addsuffix .out,$(EXECUTABLES))

.PHONY: all
all: $(EXECUTABLES_OUT)


# 找出源文件（仅框架源码；排除 build/example/thirdparty/test 以避免多个 main）
SRCS := $(shell find .. -type f -name '*.cpp' \
    ! -path '../build/*' \
    ! -path '../example/*' \
    ! -path '../thirdparty/*' \
    ! -path '*/test/*')
ASMS := $(shell find .. -type f -name '*.S' ! -path '../build/*')
OBJS := $(patsubst %.cpp, %.o, $(SRCS)) $(patsubst %.S, %.o, $(ASMS))

# 编译规则
%.o: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<

%.o: %.S
	$(AS) -c $< -o $@

# 生成可执行文件的规则
%.out: $(OBJS) %.cpp
	$(CC) $(INCLUDE) $(DEBUG) -o $@ $^ $(ThirdPartyLibPath) $(ThirdPartyLib) $(RPATH)

# 清理规则
.PHONY: clean
clean:
	rm -rf *.out *.log $(OBJS)
