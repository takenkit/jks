# コンパイラ
CC = gcc

# コンパイルオプション
CFLAGS = -Wall -Wextra -pthread

# ソースファイル
SRCS = thread_safe_expandable_memory_pool.c

# ヘッダファイル
HEADERS = common.h

# 出力ファイル
TARGET = thread_safe_expandable_memory_pool

# デフォルトのターゲット
all: $(TARGET)

# 出力ファイルの生成
$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# クリーンアップ
clean:
	rm -f $(TARGET)

.PHONY: all clean
