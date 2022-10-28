# project root directory
P_DIR_PROJECT=${PWD}
# freetype include path
P_DIR_FREETYPE_INC=freetype/include
# fontCvt build directory
P_DIR_BUILD=${P_DIR_PROJECT}/build
# fontCvt source directory
P_DIR_SRC=${P_DIR_PROJECT}/src

P_GCC_FLAGS= -I ${P_DIR_FREETYPE_INC} -Lfreetype -lfreetype -g

.PHONY: compile
compile:
	echo ${P_DIR_PROJECT}
	if [ ! -d ${P_DIR_BUILD} ]; \
	then \
		mkdir ${P_DIR_BUILD}; \
	fi
	
	gcc ${P_DIR_SRC}/fontCvt.c ${P_GCC_FLAGS} -c -o ${P_DIR_BUILD}/fontcvt.o
	gcc ${P_DIR_SRC}/builderForC.c ${P_GCC_FLAGS} -c -o ${P_DIR_BUILD}/builderforc.o
	gcc ${P_DIR_BUILD}/fontcvt.o ${P_DIR_BUILD}/builderforc.o ${P_GCC_FLAGS} -o ${P_DIR_BUILD}/fontcvt
	@echo ok ... build done

.PHONY: clean
clean:
	rm -r ${P_DIR_BUILD}

.PHONY: debug
debug:
	gnome-terminal -x gdbserver localhost:1111 ${P_DIR_BUILD}/fontCvt ${P_ARGS}
	gnome-terminal -x gdbgui --gdb-args="-x ${P_DIR_PROJECT}/utility/setup.gdb"


.PHONY: run
run:
	${P_DIR_BUILD}/fontCvt ${P_ARGS}
