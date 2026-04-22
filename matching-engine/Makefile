# Makefile for the matching engine
# Works in GitHub Codespaces' default Ubuntu image out of the box
# (no extra tools needed beyond g++).
# --------------------------------------------------------------

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS  ?=

SRC      := src/OrderBook.cpp src/LobsterReplay.cpp src/Benchmark.cpp src/main.cpp
OBJ      := $(SRC:.cpp=.o)
BIN      := matching_engine

.PHONY: all clean run demo bench replay

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

demo: $(BIN)
	./$(BIN) demo

bench: $(BIN)
	./$(BIN) bench 1000000

# Replay the sample LOBSTER data. Override CSV=... if you move the file.
CSV ?= data/MSFT_2012-06-21_34200000_57600000_message_10.csv
replay: $(BIN)
	./$(BIN) replay $(CSV)

clean:
	rm -f $(OBJ) $(BIN)
