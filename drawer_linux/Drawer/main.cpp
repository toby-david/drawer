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
    float alpha = 0.0f; 
    float alphaNote = 0.0f; 
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

Color GetPassStrengthColor(const char* p) {
    string s = p;
    if (s.length() == 0) return DARKGRAY;
    int score = 0;
    if (s.length() >= 8) score++;
    if (s.length() >= 12) score++;
    if (s.find_first_of("0123456789") != string::npos) score++;
    if (s.find_first_of("!@#$%^&*()") != string::npos) score++;
    if (score <= 1) return RED;
    if (score <= 3) return YELLOW;
    return GREEN;
}

// --- UTILITY DISEGNO ---
void DrawSText(Font font, const char* text, Rectangle rec, float size, Color col, bool focus, int frms, const char* ph) {
    float tw = MeasureTextEx(font, text, size, 1).x;
    float off = (tw > rec.width - 20) ? tw - (rec.width - 20) : 0;
    if (focus) DrawRectangleLinesEx({rec.x-2, rec.y-2, rec.width+4, rec.height+4}, 1, Fade(RED, sinf(GetTime()*10)*0.5f + 0.5f));
    
    // Testo visibile senza ScissorMode per evitare bug di coordinate su Linux
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

// --- MAIN ---
int main() {
    const int sw = 1000; const int sh = 750;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(sw, sh, "Drawer - The Ultimate Password Manager");
    SetTargetFPS(60);

    Image icon = LoadImage("icon.png"); 
    if (icon.data != NULL) { SetWindowIcon(icon); UnloadImage(icon); }

    Font fHD = LoadFontEx("font.ttf", 96, 0, 250);
    if (fHD.texture.id == 0) fHD = GetFontDefault();
    SetTextureFilter(fHD.texture, TEXTURE_FILTER_BILINEAR);

    bool login = false, erroreLogin = false, caricato = false, menuAperto = false, modalitaReset = false;
    bool primoAvvio = !FileExists("config.dat");
    
    int deleteIndex = -1, focus = 0, frames = 0, editingIndex = -1;
    static char pIn[64] = { 0 }, sIn[128] = { 0 }, passIn[128] = { 0 }, cIn[128] = { 0 }, noteIn[256] = { 0 };
    
    vector<Credenziale> elenco;
    string masterPass = primoAvvio ? "" : caricaMaster();

    float transX = 0.0f, menuY = (float)sh, copyTimer = 0.0f, scrollY = 0.0f, errorShake = 0.0f, timerInattivita = 0.0f; 

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        frames++;
        if (copyTimer > 0) copyTimer -= dt;
        if (errorShake > 0) errorShake -= dt * 5.0f;
        
        int cw = GetScreenWidth(); 
        int ch = GetScreenHeight();
        Vector2 mPos = GetMousePosition();

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

        // --- LOGIN LOGIC ---
        if (!login && IsKeyPressed(KEY_ENTER)) {
            if (primoAvvio) { masterPass = pIn; salvaMaster(masterPass); primoAvvio = false; memset(pIn, 0, 64); }
            else if (string(pIn) == masterPass) { if(modalitaReset){primoAvvio=true; modalitaReset=false;} else login = true; erroreLogin = false; memset(pIn, 0, 64); }
            else { erroreLogin = true; errorShake = 1.0f; memset(pIn, 0, 64); }
        }

        // --- ANIMAZIONI ---
        float targetX = login ? -(float)cw : 0.0f;
        transX += (targetX - transX) * (1.0f - expf(-10.0f * dt));
        
        if (login && !caricato && transX < -cw/2) {
            ifstream f("vault.txt"); string r;
            while (getline(f, r)) {
                size_t pos = r.find('|');
                if (pos != string::npos) elenco.push_back({decripta(r.substr(0, pos)), decripta(r.substr(pos+1)), 45.0f, 1.0f, 0.0f, false, false});
            }
            f.close(); caricato = true;
        }

        float targetMenuY = menuAperto ? (float)ch - 240 : (float)ch - 45;
        menuY += (targetMenuY - menuY) * (1.0f - expf(-12.0f * dt));

        BeginDrawing();
        ClearBackground(GetColor(0x121212FF));

        // --- RENDER LOGIN ---
        if (transX > -cw + 1) { 
            float ox = transX + sinf(GetTime()*60)*(errorShake*10);
            float lAlpha = 1.0f + (transX / cw); 
            const char* tit = primoAvvio ? "CREATE MASTER PASSWORD" : (modalitaReset ? "VERIFY OLD PASSWORD" : "LOGIN");
            DrawCenteredText(fHD, tit, {ox, (float)ch/2 - 100, (float)cw, 50}, 32, Fade(RED, lAlpha));
            Rectangle rL = { ox + (float)cw/2 - 150, (float)ch/2 - 30, 300, 50 };
            DrawRectangleLinesEx(rL, 2, Fade(RED, lAlpha));
            string ast(strlen(pIn), '*'); 
            DrawSText(fHD, ast.c_str(), rL, 28, Fade(WHITE, lAlpha), (!login), frames, "KEY");
        }

        // --- RENDER VAULT ---
        if (transX < -1) { 
            float ox = transX + cw;
            DrawRectangle(ox, 0, cw, 70, GetColor(0x1A1A1AFF));
            
            Rectangle rLogout = { ox + 30, 20, 100, 35 };
            bool hLog = CheckCollisionPointRec(mPos, rLogout);
            if(hLog && IsMouseButtonPressed(0)) { login = false; focus = 0; caricato = false; elenco.clear(); }
            DrawTextEx(fHD, "LOGOUT", { ox + 30, 20 }, 30, 2, hLog ? RED : MAROON);
            
            Rectangle rSearch = { ox + (float)cw - 350, 17, 320, 35 };
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(mPos, rSearch)) focus = 1;
            DrawSText(fHD, cIn, rSearch, 20, WHITE, (focus==1), frames, "SEARCH...");
            
            // Lista Credenziali
            BeginScissorMode((int)ox, 75, cw, (int)menuY - 75);
                float curY = 90 + scrollY;
                for(int i = 0; i < (int)elenco.size(); i++) {
                    if(strlen(cIn) > 0 && (elenco[i].dati.find(cIn) == string::npos)) continue;
                    Rectangle itR = { ox + 30, curY, (float)cw-60, 45 };
                    DrawRectangleRec(itR, GetColor(0x1E1E1EFF));
                    
                    string sDisp = elenco[i].dati;
                    if(!elenco[i].visibile) { size_t posS = sDisp.find(" : "); if(posS != string::npos) sDisp = sDisp.substr(0, posS+3) + "********"; }
                    DrawTextEx(fHD, sDisp.c_str(), { ox + 95, curY+13 }, 20, 1, WHITE);
                    
                    // BOTTONI AZIONE CORAZZATI
                    Rectangle bVw = { ox+cw-380, curY+5, 80, 35 }, bCp = { ox+cw-290, curY+5, 80, 35 }, bEd = { ox+cw-200, curY+5, 80, 35 }, bDl = { ox+cw-110, curY+5, 80, 35 };
                    
                    auto DrawActionBtn = [&](Rectangle r, Color c, const char* t, int act) {
                        bool h = CheckCollisionPointRec(mPos, r);
                        if (h) {
                            DrawRectangleRec(r, Fade(c, 0.4f));
                            if (IsMouseButtonPressed(0) && deleteIndex == -1) {
                                if(act == 0) elenco[i].visibile = !elenco[i].visibile;
                                if(act == 1) { size_t s = elenco[i].dati.find(" : "); if(s != string::npos) { SetClipboardText(elenco[i].dati.substr(s+3).c_str()); copyTimer = 1.5f; } }
                                if(act == 2) { editingIndex = i; menuAperto = true; focus = 2; }
                                if(act == 3) deleteIndex = i;
                            }
                        }
                        DrawRectangleLinesEx(r, 1, h ? WHITE : c);
                        DrawCenteredText(fHD, t, r, 16, h ? WHITE : c);
                    };

                    DrawActionBtn(bVw, ORANGE, "VIEW", 0);
                    DrawActionBtn(bCp, BLUE, "COPY", 1);
                    DrawActionBtn(bEd, GRAY, "EDIT", 2);
                    DrawActionBtn(bDl, MAROON, "DEL", 3);

                    curY += 55;
                }
            EndScissorMode();
            
            scrollY += GetMouseWheelMove() * 30;
            if (scrollY > 0) scrollY = 0;

            // --- PANNELLO AGGIUNGI ---
            DrawRectangle(ox, (int)menuY, cw, ch, GetColor(0x181818FF)); 
            DrawLine(ox, (int)menuY, ox+cw, (int)menuY, RED);
            
            Rectangle hdl = { ox, menuY, (float)cw, 45 };
            bool hHdl = CheckCollisionPointRec(mPos, hdl);
            if(hHdl && IsMouseButtonPressed(0)) { menuAperto = !menuAperto; focus = 0; }
            DrawCenteredText(fHD, menuAperto ? "[ CLOSE ]" : "[ ADD NEW CREDENTIAL ]", hdl, 20, hHdl ? WHITE : RED);
            
            float mY = menuY + 65;
            Rectangle rS = {ox+30, mY, 250, 40}, rP = {ox+290, mY, 250, 40}, bGen = {ox+550, mY, 40, 40}, bA = {ox+(float)cw-320, mY, 290, 40}, rN = {ox+30, mY+60, (float)cw-60, 40};
            
            if(IsMouseButtonPressed(0)) {
                if(CheckCollisionPointRec(mPos, rS)) focus = 2; 
                else if(CheckCollisionPointRec(mPos, rP)) focus = 3; 
                else if(CheckCollisionPointRec(mPos, rN)) focus = 5;
            }

            DrawSText(fHD, sIn, rS, 20, WHITE, (focus==2), frames, "SITE/SERVICE");
            DrawSText(fHD, passIn, rP, 20, WHITE, (focus==3), frames, "PASSWORD");
            DrawSText(fHD, noteIn, rN, 20, WHITE, (focus==5), frames, "NOTES...");

            // Tasto Genera
            bool hGen = CheckCollisionPointRec(mPos, bGen);
            if(hGen && IsMouseButtonPressed(0)) strncpy(passIn, generaPass().c_str(), 127);
            DrawRectangleLinesEx(bGen, 1, hGen ? WHITE : GRAY);
            DrawCenteredText(fHD, "#", bGen, 20, hGen ? WHITE : GRAY);
            
            // Tasto Salva
            bool hSave = CheckCollisionPointRec(mPos, bA);
            if(hSave && IsMouseButtonPressed(0)) {
                if(strlen(sIn)>0) { 
                    if(editingIndex != -1) { elenco[editingIndex].dati = string(sIn) + " : " + string(passIn); elenco[editingIndex].note = noteIn; editingIndex = -1; }
                    else elenco.push_back({string(sIn)+" : "+string(passIn), string(noteIn), 45.0f, 1.0f, 0.0f, false, false}); 
                    salva(elenco); 
                }
                menuAperto = false; memset(sIn,0,128); memset(passIn,0,128); memset(noteIn,0,256);
            }
            DrawRectangleRec(bA, hSave ? GREEN : DARKGREEN);
            DrawCenteredText(fHD, editingIndex != -1 ? "UPDATE" : "SAVE", bA, 20, WHITE);
        }

        // --- OVERLAYS ---
        if (deleteIndex != -1) {
            DrawRectangle(0,0,cw,ch, {0,0,0,220}); 
            DrawCenteredText(fHD, "CONFIRM DELETE? (Y/N)", {0,(float)ch/2 - 25, (float)cw, 50}, 30, WHITE);
            if(IsKeyPressed(KEY_Y)) { elenco.erase(elenco.begin()+deleteIndex); salva(elenco); deleteIndex = -1; }
            if(IsKeyPressed(KEY_N)) deleteIndex = -1;
        }
        if(copyTimer > 0) {
            DrawRectangle(cw/2-100, 20, 200, 40, DARKGREEN);
            DrawCenteredText(fHD, "COPIED!", { (float)cw/2-100, 20, 200, 40 }, 20, WHITE);
        }

        EndDrawing();
    }
    CloseWindow(); return 0;
}