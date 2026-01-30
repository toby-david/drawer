#!/bin/bash
# 1. Compila l'eseguibile staticamente per Raylib
g++ main.cpp -o drawer -I./raylib/include ./raylib/lib/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

# 2. Prepara la cartella AppDir
mkdir -p Drawer.AppDir/usr/bin
cp drawer Drawer.AppDir/usr/bin/
cp font.ttf Drawer.AppDir/usr/bin/  # Importante: Raylib cerca il font qui
cp icon.png Drawer.AppDir/

# 3. Crea il file .desktop (indispensabile per AppImage)
cat > Drawer.AppDir/drawer.desktop <<EOF
[Desktop Entry]
Type=Application
Name=Drawer
Exec=drawer
Icon=icon
Categories=Utility;
EOF

# 4. Scarica e usa linuxdeploy per impacchettare
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

./linuxdeploy-x86_64.AppImage --appdir Drawer.AppDir --output appimage