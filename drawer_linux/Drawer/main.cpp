#define RAYLIB_PLATFORM_DESKTOP
#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace std;

// --- STRUTTURA DATI ---
struct Credenziale {
    string dati;
    string note;
    float altezzaCorrente = 45.0f;
    bool espanso = false;
    bool visibile = false; 
};

// --- LOGICA DI BACKEND ---
string cripta(string t) { for(char &c : t) c += 1; return t; }
string decripta(string t) { for(char &c : t) c -= 1; return t; }

void salvaMaster(string p) {
    ofstream f("config.dat");
    f << cripta(p);
    f.close();
}

string caricaMaster() {
    ifstream f("config.dat");
    string p; getline(f, p);
    f.close();
    return decripta(p);
}

void salva(const vector<Credenziale>& elenco) {
    ofstream f("vault.txt");
    for (const auto& c : elenco) f << cripta(c.dati) << "|" << cripta(c.note) << endl;
    f.close();
}

string generaPass(int len = 16) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";
    string res = "";
    for (int i = 0; i < len; i++) res += charset[GetRandomValue(0, 71)];
    return res;
}

// --- UTILITY DISEGNO ---
void DrawSText(Font font, const char* text, Rectangle rec, float size, Color col, bool focus, int frms, const char* ph) {
    float tw = MeasureTextEx(font, text, size, 1).x;
    float off = (tw > rec.width - 20) ? tw - (rec.width - 20) : 0;
    
    DrawRectangleLinesEx(rec, 1, focus ? RED : DARKGRAY);
    if (focus) DrawRectangleLinesEx({rec.x-2, rec.y-2, rec.width+4, rec.height+4}, 1, Fade(RED, sinf(GetTime()*10)*0.5f + 0.5f));
    
    if (strlen(text) == 0 && !focus) {
        DrawTextEx(font, ph, { rec.x + 10, rec.y + rec.height/2 - size/2 }, size, 1, DARKGRAY);
    } else {
        DrawTextEx(font, text, { rec.x + 10 - off, rec.y + rec.height/2 - size/2 }, size, 1, col);
    }
    if (focus && ((frms/30)%2)==0) {
        DrawRectangle((int)(rec.x + 10 + tw - off + 2), (int)(rec.y + rec.height/2 - size/2), 2, (int)size, RED);
    }
}

void DrawCenteredText(Font font, const char* text, Rectangle rec, float size, Color col) {
    Vector2 ts = MeasureTextEx(font, text, size, 1);
    DrawTextEx(font, text, { rec.x + rec.width/2 - ts.x/2, rec.y + rec.height/2 - ts.y/2 }, size, 1, col);
}

int main() {
    const int sw = 1000; const int sh = 750;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(sw, sh, "Drawer - The Ultimate Password Manager");
    SetTargetFPS(60);

    Font fHD = LoadFontEx("font.ttf", 96, 0, 250);
    if (fHD.texture.id == 0) fHD = GetFontDefault();
    SetTextureFilter(fHD.texture, TEXTURE_FILTER_BILINEAR);

    bool login = false, caricato = false, menuAperto = false;
    bool primoAvvio = !FileExists("config.dat");
    
    int deleteIndex = -1, focus = 0, frames = 0, editingIndex = -1;
    static char pIn[64] = { 0 }, sIn[128] = { 0 }, passIn[128] = { 0 }, cIn[128] = { 0 }, noteIn[256] = { 0 };
    
    vector<Credenziale> elenco;
    string masterPass = primoAvvio ? "" : caricaMaster();

    float transX = 0.0f, menuY = (float)sh, copyTimer = 0.0f, scrollY = 0.0f, errorShake = 0.0f; 

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        frames++;
        if (copyTimer > 0) copyTimer -= dt;
        if (errorShake > 0) errorShake -= dt * 5.0f;
        
        int cw = GetScreenWidth(); 
        int ch = GetScreenHeight();
        Vector2 mPos = GetMousePosition();

        auto DrawBtn = [&](Rectangle r, Color c, const char* t) -> bool {
            bool h = CheckCollisionPointRec(mPos, r);
            DrawRectangleLinesEx(r, 1, h ? WHITE : c);
            if (h) DrawRectangleRec(r, Fade(c, 0.2f));
            DrawCenteredText(fHD, t, r, 14, h ? WHITE : c);
            return h && IsMouseButtonPressed(0);
        };

        // --- GESTIONE INPUT ---
        char* currentBuffer = nullptr;
        int maxChars = 127;
        if (!login) { currentBuffer = pIn; maxChars = 63; }
        else if (focus == 1) { currentBuffer = cIn; maxChars = 127; }
        else if (focus == 2) { currentBuffer = sIn; maxChars = 127; }
        else if (focus == 3) { currentBuffer = passIn; maxChars = 127; }
        else if (focus == 5) { currentBuffer = noteIn; maxChars = 255; }

        if (currentBuffer && deleteIndex == -1) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (strlen(currentBuffer) < maxChars)) {
                    int len = strlen(currentBuffer);
                    currentBuffer[len] = (char)key;
                    currentBuffer[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(currentBuffer);
                if (len > 0) currentBuffer[len - 1] = '\0';
            }
        }

        if (!login && IsKeyPressed(KEY_ENTER)) {
            if (primoAvvio) { masterPass = pIn; salvaMaster(masterPass); primoAvvio = false; memset(pIn, 0, 64); }
            else if (string(pIn) == masterPass) { login = true; errorShake = 0; memset(pIn, 0, 64); }
            else { errorShake = 1.0f; memset(pIn, 0, 64); }
        }

        float targetX = login ? -(float)cw : 0.0f;
        transX += (targetX - transX) * (1.0f - expf(-10.0f * dt));
        
        if (login && !caricato && transX < -cw/2) {
            ifstream f("vault.txt"); string r;
            while (getline(f, r)) {
                size_t pos = r.find('|');
                if (pos != string::npos) elenco.push_back({decripta(r.substr(0, pos)), decripta(r.substr(pos+1)), 45.0f, false, false});
            }
            f.close(); caricato = true;
        }

        float targetMenuY = menuAperto ? (float)ch - 240 : (float)ch - 45;
        menuY += (targetMenuY - menuY) * (1.0f - expf(-12.0f * dt));

        BeginDrawing();
        ClearBackground(GetColor(0x121212FF));

        if (transX > -cw + 1) {
            float ox = transX + sinf(GetTime()*60)*(errorShake*10);
            DrawCenteredText(fHD, primoAvvio ? "CREATE MASTER PASS" : "LOGIN", {ox, (float)ch/2 - 100, (float)cw, 50}, 32, RED);
            Rectangle rL = { ox + (float)cw/2 - 150, (float)ch/2 - 30, 300, 50 };
            string ast(strlen(pIn), '*');
            DrawSText(fHD, ast.c_str(), rL, 28, WHITE, !login, frames, "KEY");
        }

        if (transX < -1) {
            float ox = transX + cw;
            DrawRectangle(ox, 0, cw, 70, GetColor(0x1A1A1AFF));
            
            Rectangle rLog = { ox + 30, 20, 100, 35 };
            bool hLog = CheckCollisionPointRec(mPos, rLog);
            if (hLog && IsMouseButtonPressed(0)) { login = false; caricato = false; elenco.clear(); focus = 0; }
            DrawTextEx(fHD, "LOGOUT", { rLog.x, rLog.y }, 25, 1, hLog ? WHITE : MAROON);

            Rectangle rSearch = { ox + (float)cw - 350, 17, 320, 35 };
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(mPos, rSearch)) focus = 1;
            DrawSText(fHD, cIn, rSearch, 20, WHITE, (focus==1), frames, "SEARCH...");

            BeginScissorMode((int)ox, 75, cw, (int)menuY - 75);
                float curY = 90 + scrollY;
                for(int i = 0; i < (int)elenco.size(); i++) {
                    if(strlen(cIn) > 0 && (elenco[i].dati.find(cIn) == string::npos)) continue;
                    
                    float targetH = elenco[i].espanso ? 110.0f : 45.0f;
                    elenco[i].altezzaCorrente += (targetH - elenco[i].altezzaCorrente) * 0.2f;

                    Rectangle itR = { ox + 30, curY, (float)cw-60, elenco[i].altezzaCorrente };
                    DrawRectangleRec(itR, GetColor(0x1E1E1EFF));
                    
                    if(IsMouseButtonPressed(0) && CheckCollisionPointRec(mPos, {itR.x, itR.y, itR.width - 400, itR.height})) {
                        bool statoPrecedente = elenco[i].espanso;
                        for(auto &c : elenco) c.espanso = false;
                        elenco[i].espanso = !statoPrecedente;
                    }

                    string sDisp = elenco[i].dati;
                    if(!elenco[i].visibile) {
                        size_t p = sDisp.find(" : ");
                        if(p != string::npos) sDisp = sDisp.substr(0, p+3) + "********";
                    }
                    DrawTextEx(fHD, sDisp.c_str(), { ox + 50, curY+13 }, 20, 1, WHITE);
                    
                    if(elenco[i].espanso && elenco[i].altezzaCorrente > 75) {
                        DrawTextEx(fHD, TextFormat("Notes: %s", elenco[i].note.c_str()), { ox + 55, curY+55 }, 17, 1, GRAY);
                    }

                    Rectangle bVw = { ox+cw-380, curY+5, 80, 35 }, bCp = { ox+cw-290, curY+5, 80, 35 }, bEd = { ox+cw-200, curY+5, 80, 35 }, bDl = { ox+cw-110, curY+5, 80, 35 };
                    
                    if(DrawBtn(bVw, ORANGE, "VIEW")) elenco[i].visibile = !elenco[i].visibile;
                    if(DrawBtn(bCp, BLUE, "COPY")) {
                        size_t s = elenco[i].dati.find(" : ");
                        if(s != string::npos) { SetClipboardText(elenco[i].dati.substr(s+3).c_str()); copyTimer = 1.5f; }
                    }
                    if(DrawBtn(bEd, GRAY, "EDIT")) {
                        editingIndex = i; menuAperto = true; focus = 2;
                        size_t pos = elenco[i].dati.find(" : ");
                        if(pos != string::npos) {
                            strncpy(sIn, elenco[i].dati.substr(0, pos).c_str(), 127);
                            strncpy(passIn, elenco[i].dati.substr(pos+3).c_str(), 127);
                        }
                        strncpy(noteIn, elenco[i].note.c_str(), 255);
                    }
                    if(DrawBtn(bDl, MAROON, "DEL")) deleteIndex = i;

                    curY += elenco[i].altezzaCorrente + 10;
                }
            EndScissorMode();
            scrollY += GetMouseWheelMove() * 30;

            // --- PANNELLO ADD/EDIT (MODIFICATO) ---
            DrawRectangle(ox, (int)menuY, cw, ch, GetColor(0x181818FF)); 
            Rectangle hdl = { ox, menuY, (float)cw, 45 };
            bool hHdl = CheckCollisionPointRec(mPos, hdl);
            
            // LOGICA DI RESET ALLA CHIUSURA
            if(hHdl && IsMouseButtonPressed(0)) { 
                menuAperto = !menuAperto; 
                // Se chiudo il menu, resetto tutto
                editingIndex = -1; 
                focus = 0; 
                memset(sIn, 0, 128); 
                memset(passIn, 0, 128); 
                memset(noteIn, 0, 256);
            }
            
            DrawCenteredText(fHD, editingIndex != -1 ? "[ EDIT MODE - CLICK TO CLOSE/RESET ]" : (menuAperto ? "[ CLOSE ]" : "[ ADD NEW CREDENTIAL ]"), hdl, 20, hHdl ? WHITE : RED);
            
            float mY = menuY + 65;
            Rectangle rS = {ox+30, mY, 250, 40}, rP = {ox+290, mY, 250, 40}, bGen = {ox+550, mY, 40, 40}, bA = {ox+(float)cw-320, mY, 290, 40}, rN = {ox+30, mY+60, (float)cw-60, 40};
            
            if(IsMouseButtonPressed(0)) {
                if(CheckCollisionPointRec(mPos, rS)) focus = 2; 
                else if(CheckCollisionPointRec(mPos, rP)) focus = 3; 
                else if(CheckCollisionPointRec(mPos, rN)) focus = 5;
            }

            DrawSText(fHD, sIn, rS, 20, WHITE, (focus==2), frames, "SERVICE");
            DrawSText(fHD, passIn, rP, 20, WHITE, (focus==3), frames, "PASSWORD");
            DrawSText(fHD, noteIn, rN, 20, WHITE, (focus==5), frames, "NOTES");

            if(DrawBtn(bGen, GRAY, "#")) strncpy(passIn, generaPass().c_str(), 127);
            
            if(DrawBtn(bA, GREEN, editingIndex != -1 ? "UPDATE" : "SAVE")) {
                if(strlen(sIn) > 0) {
                    string d = string(sIn) + " : " + string(passIn);
                    if(editingIndex != -1) { elenco[editingIndex].dati = d; elenco[editingIndex].note = noteIn; editingIndex = -1; }
                    else { elenco.push_back({d, string(noteIn), 45.0f, false, false}); }
                    salva(elenco);
                }
                menuAperto = false; focus = 0;
                memset(sIn, 0, 128); memset(passIn, 0, 128); memset(noteIn, 0, 256);
            }
        }

        if (deleteIndex != -1) {
            DrawRectangle(0,0,cw,ch, {0,0,0,200});
            DrawCenteredText(fHD, "DELETE? (Y/N)", {0, (float)ch/2, (float)cw, 50}, 30, WHITE);
            if(IsKeyPressed(KEY_Y)) { elenco.erase(elenco.begin()+deleteIndex); salva(elenco); deleteIndex = -1; }
            if(IsKeyPressed(KEY_N)) deleteIndex = -1;
        }

        if(copyTimer > 0) DrawCenteredText(fHD, "COPIED TO CLIPBOARD!", {0, 20, (float)cw, 40}, 20, GREEN);

        EndDrawing();
    }
    CloseWindow(); return 0;
}