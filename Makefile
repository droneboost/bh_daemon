TARGET = wifi_daemon
OBJ = wifi_daemon.o

LINKFLAGS += \
    -lpthread \
    -lrt

#INCLUDES = -I../../../../../../mtk/release/BDP_Generic/build/sysroot/usr/include
#           $(BDP_GENERIC_DIR)/build/sysroot/usr/include



.PHONY : default

default:$(TARGET)

$(OBJ) : $(SRC)

$(TARGET) : $(OBJ) 
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(INCLUDES) $^ $(LINKFLAGS)  -o $@

#	$(CXX) $(CXXFLAGS) -o $@ ${OBJECT} ${LIBDIR} $(LINKFLAGS)

.c.o:
	$(CXX) ${CXXFLAGS} -c $< ${INCLUDES}

clean:
	-rm -f wifi_daemon wifi_daemon.o

wifi_daemon.o: wifi_daemon.c 



