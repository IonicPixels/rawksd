ifeq ($(OS),Windows_NT)
	STRIPIOS := stripios.exe
else
	STRIPIOS := stripios
endif

stripios: stripios.cpp rijndael.cpp
	$(CXX) -O2 -flto -o $(STRIPIOS) stripios.cpp rijndael.cpp
clean:
	rm -f $(STRIPIOS)
