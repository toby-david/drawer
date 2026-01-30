#!/bin/bash
# 1. Compila
g++ main.cpp -o myapp \
    -I./raylib-5.0_linux_amd64/include \
    ./raylib-5.0_linux_amd64/lib/libraylib.a \
    -lGL -lm -lpthread -ldl -lrt -lX11

# 2. Crea struttura AppDir per l'AppImage
mkdir -p deploy/usr/bin
cp myapp deploy/usr/bin/
cp font.ttf deploy/usr/bin/
cp icon.png deploy/

# 3. Crea il file descrittivo per Linux
echo -e "[Desktop Entry]\nName=Drawer\nExec=myapp\nIcon=icon\nType=Application\nCategories=Utility;" > deploy/myapp.desktop

# 4. Scarica lo strumento per impacchettare (se non c'è)
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

# 5. Genera il file unico portabile
./linuxdeploy-x86_64.AppImage --appdir deploy --output appimage