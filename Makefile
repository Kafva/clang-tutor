CLANG_DIR=/usr
BUILD_DIR=$(shell pwd)/build
NPROC=$(shell echo $$((`nproc` - 1)))

OUT_LIB=$(BUILD_DIR)/lib/libArgStates.so
OUTPUT= $(OUT_LIB) $(OUT_EXEC)
SRCS=lib/ArgStates.cpp lib/SecondPass.cpp lib/FirstPass.cpp lib/WriteJson.cpp \
		 include/ArgStates.hpp include/Util.hpp include/Base.hpp
.PHONY: clean run

$(OUTPUT): $(BUILD_DIR)/Makefile $(SRCS)
	make -C $(BUILD_DIR) -j$(NPROC) ArgStates

#	-DCMAKE_CXX_FLAGS=-std=c++17
$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	cmake -DCT_Clang_INSTALL_DIR=$(CLANG_DIR) -S. -B $(BUILD_DIR)

run: $(OUTPUT)
	rm -f states/*
	./run.py
	bat states/*.json

clean:
	rm -rf $(BUILD_DIR)
