# default: AGL3-game


# # Reguła jak zbudować plik wykonywalny z pliku .cpp
# %: %.cpp $(DEPS)
# 	g++ -I. $< -o $@ AGL3Window.cpp  -lepoxy -lGL -lglfw 

# # A tu dodatkowy target, którego wywołanie "make clean" czyści
# clean:
# 	rm a.out *.o *~ AGL3-game


# ==================================================
# Portable makefile for compilation on platforms: 
#     Linux   /   Windows with MSYS2 using gcc
# install required packages like:
#       e.g. gcc, make,   libglfw3-dev, libglew-dev 
# ==== TARGET ARCHITECTURE =========================
UNAMES := $(shell uname -o)
ifeq      (${UNAMES},GNU/Linux) #==== Linux ========
COPTS = 
CLIBS = -lepoxy -lGL -lglfw
EXE = 
else ifeq (${UNAMES},Msys)  #==== Windows/Msys2 ====
COPTS = -I /msys64/mingw64/include/
CLIBS = -L /msys64/mingw64/lib/ -lopengl32 -lglfw3 -lepoxy
EXE = .exe
COPTS += -DARCH_MSYS=${UNAMES}
else
TARGET = unknown-target     #==== UNKNOWN ==========
defaulterror:
	echo Unknown target architecture: uname -s 
endif                       #=======================
#===================================================


default: AGL3-terrain$(EXE)

# Komentarz: powyżej co chcemy aby powstało (można więcej)
# Sprawdzamy jeśli poniższe zmodyfikowane to także rekompilacja
DEPS=AGL3Window.cpp AGL3Window.hpp AGL3Drawable.hpp Config.hpp TileManager.hpp FrameHistory.hpp

%$(EXE): %.cpp $(DEPS)
	g++ -I. $(COPTS) $< -o $@ AGL3Window.cpp $(CLIBS) 
clean:
	rm a.out *.o *~ AGL3-terrain$(EXE)
