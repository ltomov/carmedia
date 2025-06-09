
Release:
	g++-4.8 -std=c++11 `find . -name "*.cpp"` -o bin/Release/carmedia2  `sdl2-config --cflags --libs` `pkg-config --cflags --libs SDL2_image SDL2_ttf SDL2_mixer sqlite3 taglib libavformat libavcodec dbus-c++-1`


Debug:
	g++-4.8 -std=c++11 -g -O0 -W -Wall -Wextra -pedantic -Wno-unused-parameter `find . -name "*.cpp"` -o bin/Debug/carmedia2  `sdl2-config --cflags --libs` `pkg-config --cflags --libs SDL2_image SDL2_ttf SDL2_mixer sqlite3 taglib libavformat libavcodec dbus-c++-1`


#panda:
#	g++ main.cpp button.cpp image.cpp wt32i.cpp touch.cpp -o bin/Release/carmedia2 -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -L/usr/lib/arm-linux-gnueabihf -lSDL -lSDL_image -lSDL_ttf -lsqlite3
