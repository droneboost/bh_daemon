TARGET = bh_daemon
OBJ = bh_daemon.o

LINKFLAGS += \
    -lpthread \
    -lrt

#INCLUDES = -I../

.PHONY : default

default:$(TARGET)

$(OBJ) : $(SRC)

$(TARGET) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(INCLUDES) $^ $(LINKFLAGS)  -o $@

#	$(CXX) $(CXXFLAGS) -o $@ ${OBJECT} ${LIBDIR} $(LINKFLAGS)

.c.o:
	$(CXX) ${CXXFLAGS} -c $< ${INCLUDES}

clean:
	-rm -f bh_daemon bh_daemon.o

bh_daemon.o: bh_daemon.c



