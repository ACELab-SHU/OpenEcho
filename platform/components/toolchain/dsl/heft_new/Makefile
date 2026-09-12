CXX = g++
CXXFLAGS = -std=c++17
CXXFLAGS += -I./json/include
CXXFLAGS += -I./include
SRCS = $(wildcard *.cpp) $(wildcard include/*.cpp)
OBJDIR = obj
OBJS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))
EXEC = main
HEADERS = $(wildcard include/*.hpp)

INPUT_DIR = ../IJ
OUTPUT_DIR = ./DAG
SUBFOLDERS = $(wildcard $(INPUT_DIR)/*)
INPUT_FILES = $(foreach dir,$(SUBFOLDERS),$(wildcard $(dir)/slice_updated_tasks.json))
OUTPUT_FILES = $(foreach dir,$(SUBFOLDERS),$(OUTPUT_DIR)/$(notdir $(dir)).json)

.PHONY: all clean

all: $(OUTPUT_FILES)

$(OUTPUT_DIR)/%.json: $(INPUT_DIR)/%/slice_updated_tasks.json $(EXEC)
	@mkdir -p $(OUTPUT_DIR)
	./$(EXEC) $< $@ >debug.txt

$(EXEC): $(OBJS)
	$(CXX) -g -o $(EXEC) $(OBJS) $(CXXFLAGS)

$(OBJDIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-find $(OUTPUT_DIR) -name '*.json' -delete