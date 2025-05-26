CXX := g++
#CXXFLAGS := -Wall -O2 `pkg-config --cflags opencv4 gstreamer-1.0`
#For the pigpio Library uncomment commented lines
CXXFLAGS := -Wall -std=c++17

LDFLAGS:= -lpigpio -lrt -pthread
#LDFLAGS := `pkg-config --libs opencv4 gstreamer-1.0`

#Modify the SRC line to change whichd cpp file to compile
#Modify the OUT line to change the name of the output file
SRC := IR.cpp
OUT := IR_test

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)