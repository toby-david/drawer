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
    BeginScissorMode((int)rec.x + 5, (int)rec.y, (int)rec.width - 10, (int)rec.height);
        if (strlen(text) == 0 && !focus) DrawTextEx(font, ph, { rec.x + 10, rec.y + rec.height/2 - size/2 }, size, 1, DARKGRAY);
        else DrawTextEx(font, text, { rec.x + 10 - off, rec.y + rec.height/2 - size/2 }, size, 1, col);
        if (focus && ((frms/30)%2)==0) DrawRectangle((int)(rec.x + 10 + tw - off + 2), (int)(rec.y + rec.height/2 - size/2), 2, (int)size, RED);
    EndScissorMode();
}

void DrawCenteredText(Font font, const char* text, Rectangle rec, float size, Color col) {
    Vector2 ts = MeasureTextEx(font, text, size, 1);
    DrawTextEx(font, text, { rec.x + rec.width/2 - ts.x/2, rec.y + rec.height/2 - ts.y/2 }, size, 1, col);
}

// --- MAIN ---
int main() {
    const int sw = 1000; const int sh = 750;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
    InitWindow(sw, sh, "Drawer - The Ultimate Password Manager");
    SetTargetFPS(60);

    // ... dopo SetTargetFPS(60);

    // Carica l'icona in fase di esecuzione
    Image icon = LoadImage("icon.png"); 
    SetWindowIcon(icon);
    UnloadImage(icon); // Libera la memoria dell'immagine originale

    // Questa riga deve esserci solo UNA volta
    Font fHD = LoadFontEx("font.ttf", 96, 0, 250);
    SetTextureFilter(fHD.texture, TEXTURE_FILTER_BILINEAR);
    // ... il resto del tuo main ...

    SetTextureFilter(fHD.texture, TEXTURE_FILTER_BILINEAR);

    bool login = false, erroreLogin = false, caricato = false, menuAperto = false, modalitaReset = false;
    bool primoAvvio = !FileExists("config.dat");
    bool mostraIntro = primoAvvio; 
    
    int deleteIndex = -1, focus = 0, frames = 0, editingIndex = -1;
    static char pIn[64] = { 0 }; static char sIn[128] = { 0 }; static char passIn[128] = { 0 }; 
    static char cIn[128] = { 0 }; static char noteIn[256] = { 0 };
    
    vector<Credenziale> elenco;
    string masterPass = primoAvvio ? "" : caricaMaster();

    float transX = 0.0f, menuY = (float)sh - 40, copyTimer = 0.0f, scrollY = 0.0f, errorShake = 0.0f;
    float timerInattivita = 0.0f; 

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        frames++;
        if (copyTimer > 0) copyTimer -= dt;
        if (errorShake > 0) errorShake -= dt * 5.0f;
        int cw = GetScreenWidth(); int ch = GetScreenHeight();

        // --- GESTIONE INATTIVITA' (CORRETTA) ---
        if (login) {
            timerInattivita += dt;
            // Controllo movimento mouse, click (SX, DX, Middle), tastiera o rotella
            if (GetMouseDelta().x != 0 || GetMouseDelta().y != 0 || 
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || 
                IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) || GetKeyPressed() != 0 || GetMouseWheelMove() != 0) {
                timerInattivita = 0.0f;
            }
            if (timerInattivita >= 30.0f) {
                login = false;
                timerInattivita = 0.0f;
                memset(pIn, 0, 64);
                focus = 0;
            }
        } else timerInattivita = 0.0f;

        if (mostraIntro) {
            BeginDrawing(); ClearBackground(GetColor(0x0A0A0AFF));
            for(int i=0; i<cw; i+=50) DrawLine(i, 0, i, ch, {25, 25, 25, 255});
            for(int i=0; i<ch; i+=50) DrawLine(0, i, cw, i, {25, 25, 25, 255});
            float scanPos = fmodf(GetTime() * 150.0f, (float)ch);
            DrawRectangleGradientV(0, (int)scanPos-100, cw, 100, {0,0,0,0}, {255, 0, 0, 80});
            DrawRectangle(0, (int)scanPos, cw, 2, RED);
            DrawCenteredText(fHD, "DRAWER", {0, (float)ch/2 - 120, (float)cw, 100}, 85, RED);
            Rectangle bStart = { (float)cw/2-150, (float)ch/2 + 180, 300, 65 };
            bool hS = CheckCollisionPointRec(GetMousePosition(), bStart);
            DrawRectangleLinesEx(bStart, 2, hS ? WHITE : RED);
            DrawCenteredText(fHD, "START SESSION", bStart, 24, hS ? WHITE : RED);
            if (hS && IsMouseButtonPressed(0)) mostraIntro = false;
            EndDrawing(); continue;
        }

        float targetX = login ? -(float)cw : 0.0f;
        transX += (targetX - transX) * (1.0f - expf(-10.0f * dt));
        
        if (login && !caricato && transX < -cw/2) {
            ifstream f("vault.txt"); string r;
            while (getline(f, r)) {
                size_t pos = r.find('|');
                if (pos != string::npos) elenco.push_back({decripta(r.substr(0, pos)), decripta(r.substr(pos+1)), 45.0f, 0.0f, 0.0f, false, false});
            }
            f.close(); caricato = true;
        }
        if (!login && transX > -5.0f && caricato) { elenco.clear(); caricato = false; }

        menuY += (((menuAperto) ? (float)ch - 180 : (float)ch - 40) - menuY) * (1.0f - expf(-12.0f * dt));
        for(auto& c : elenco) {
            c.altezzaCorrente += (((c.espanso) ? 110.0f : 45.0f) - c.altezzaCorrente) * (1.0f - expf(-12.0f * dt));
            c.alphaNote += (((c.espanso) ? 1.0f : 0.0f) - c.alphaNote) * (1.0f - expf(-10.0f * dt));
            c.alpha += (((login) ? 1.0f : 0.0f) - c.alpha) * (1.0f - expf(-8.0f * dt));
        }

        BeginDrawing(); ClearBackground(GetColor(0x121212FF));

        if (transX > -cw + 1) { 
            float ox = transX + sinf(GetTime()*60)*(errorShake*10);
            float lAlpha = 1.0f + (transX / cw); 
            for(int i=0; i<cw; i+=80) DrawLine(i+ox, 0, cw/2+ox, ch/2, Fade(RED, 0.1f*lAlpha));
            const char* tit = primoAvvio ? "CREATE MASTER PASSWORD" : (modalitaReset ? "VERIFY OLD PASSWORD" : "LOGIN");
            DrawCenteredText(fHD, tit, {ox, (float)ch/2 - 100, (float)cw, 50}, 32, Fade(RED, lAlpha));
            Rectangle rL = { ox + (float)cw/2 - 150, (float)ch/2 - 30, 300, 50 };
            DrawRectangleLinesEx(rL, 2, Fade(RED, lAlpha));
            if (erroreLogin) DrawCenteredText(fHD, "ACCESS DENIED", {ox, (float)ch/2 + 80, (float)cw, 20}, 20, Fade(RED, lAlpha));
            string ast(strlen(pIn), '*'); DrawSText(fHD, ast.c_str(), rL, 28, Fade(WHITE, lAlpha), !login, frames, "KEY");
            if (!primoAvvio) {
                Rectangle bR = { ox + (float)cw/2 - 120, (float)ch/2 + 35, 240, 30 };
                bool hR = CheckCollisionPointRec(GetMousePosition(), bR);
                if (hR && IsMouseButtonPressed(0)) { modalitaReset = !modalitaReset; memset(pIn, 0, 64); erroreLogin = false; }
                DrawCenteredText(fHD, modalitaReset ? "CANCEL" : "CHANGE MASTER PASSWORD", bR, 15, hR ? WHITE : Fade(GRAY, lAlpha));
            }
        }

        if (transX < -1) { 
            float ox = transX + cw;
            float vAlpha = -transX / cw; 
            DrawRectangle(ox, 0, cw, 70, GetColor(0x1A1A1AFF));
            Rectangle rLogout = { ox + 30, 20, 100, 35 };
            if(CheckCollisionPointRec(GetMousePosition(), rLogout) && IsMouseButtonPressed(0)) { login = false; focus = 0; memset(pIn, 0, 64); }
            DrawTextEx(fHD, "LOGOUT", { ox + 30, 20 }, 30, 2, CheckCollisionPointRec(GetMousePosition(), rLogout) ? RED : MAROON);
            Rectangle rSearch = { ox + (float)cw - 350, 17, 320, 35 };
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), rSearch)) focus = 1;
            DrawRectangleLinesEx(rSearch, (focus==1)?2:1, (focus==1)?RED:DARKGRAY);
            DrawSText(fHD, cIn, rSearch, 20, Fade(WHITE, vAlpha), (focus==1), frames, "SEARCH...");
            
            float totalH = 20.0f; 
            for(auto& c : elenco) if(strlen(cIn) == 0 || (c.dati.find(cIn) != string::npos || c.note.find(cIn) != string::npos)) totalH += c.altezzaCorrente + 10;
            float viewportH = menuY - 75.0f;
            scrollY += GetMouseWheelMove() * 38.0f;
            if (scrollY < -(totalH - viewportH + 50) && totalH > viewportH) scrollY = -(totalH - viewportH + 50);
            if (scrollY > 0 || totalH <= viewportH) scrollY = 0;

            BeginScissorMode((int)ox, 75, cw, (int)menuY - 75);
                float curY = 90 + scrollY;
                for(int i = 0; i < (int)elenco.size(); i++) {
                    if(strlen(cIn) > 0 && (elenco[i].dati.find(cIn) == string::npos && elenco[i].note.find(cIn) == string::npos)) continue;
                    Rectangle itR = { ox + 30, curY, (float)cw-60, elenco[i].altezzaCorrente };
                    DrawRectangleRec(itR, Fade(GetColor(0x1E1E1EFF), elenco[i].alpha));
                    Color txtC = Fade(WHITE, elenco[i].alpha);
                    string sDisp = elenco[i].dati;
                    if(!elenco[i].visibile) { size_t posS = sDisp.find(" : "); if(posS != string::npos) sDisp = sDisp.substr(0, posS+3) + "********"; }
                    DrawTextEx(fHD, sDisp.c_str(), { ox + 95, curY+13 }, 20, 1, txtC);
                    Rectangle bVw = { ox+cw-380, curY+5, 80, 35 }, bCp = { ox+cw-290, curY+5, 80, 35 }, bEd = { ox+cw-200, curY+5, 80, 35 }, bDl = { ox+cw-110, curY+5, 80, 35 };
                    auto DrawAnimBtn = [&](Rectangle r, Color c, const char* t, int type) {
                        bool h = CheckCollisionPointRec(GetMousePosition(), r);
                        if(h) { r.y -= 2; DrawRectangleRec(r, c); if(IsMouseButtonPressed(0) && deleteIndex == -1) {
                            if(type == -1) elenco[i].visibile = !elenco[i].visibile;
                            if(type == 0) { size_t s = elenco[i].dati.find(" : "); if(s != string::npos) { SetClipboardText(elenco[i].dati.substr(s+3).c_str()); copyTimer = 1.5f; } }
                            if(type == 1) { editingIndex = i; string d = elenco[i].dati; size_t s = d.find(" : "); if(s != string::npos) { strncpy(sIn, d.substr(0, s).c_str(), 127); strncpy(passIn, d.substr(s+3).c_str(), 127); } strncpy(noteIn, elenco[i].note.c_str(), 255); menuAperto = true; focus = 2; }
                            if(type == 2) deleteIndex = i;
                        }} else DrawRectangleLinesEx(r, 1, c);
                        DrawCenteredText(fHD, t, r, 16, txtC);
                    };
                    DrawAnimBtn(bVw, ORANGE, elenco[i].visibile ? "HIDE" : "VIEW", -1);
                    DrawAnimBtn(bCp, BLUE, "COPY", 0); DrawAnimBtn(bEd, GRAY, "EDIT", 1); DrawAnimBtn(bDl, MAROON, "DEL", 2);
                    if(elenco[i].alphaNote > 0.05f) DrawTextEx(fHD, TextFormat("NOTE: %s", elenco[i].note.empty() ? "NONE" : elenco[i].note.c_str()), { ox+95, curY+55 }, 18, 1, Fade(GRAY, elenco[i].alphaNote * elenco[i].alpha));
                    if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), {ox+30, curY, (float)cw-400, 45})) { bool s = elenco[i].espanso; for(auto& x : elenco) x.espanso = false; elenco[i].espanso = !s; }
                    curY += elenco[i].altezzaCorrente + 10;
                }
            EndScissorMode();

            DrawRectangle(ox, (int)menuY, cw, ch, GetColor(0x181818FF)); DrawLine(ox, (int)menuY, ox+cw, (int)menuY, RED);
            Rectangle hdl = { ox, menuY, (float)cw, 40 };
            DrawCenteredText(fHD, menuAperto ? "[ CLOSE ]" : "[ ADD NEW CREDENTIAL ]", hdl, 18, RED);
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), hdl)) { menuAperto = !menuAperto; focus = 0; }
            float mY = menuY + 55;
            Rectangle rS = {ox+30, mY, 250, 35}, rP = {ox+290, mY, 250, 35}, bGen = {ox+550, mY, 40, 35}, bA = {ox+(float)cw-320, mY, 290, 35}, rN = {ox+30, mY+55, (float)cw-60, 35};
            if(IsMouseButtonPressed(0)) { if(CheckCollisionPointRec(GetMousePosition(), rS)) focus = 2; else if(CheckCollisionPointRec(GetMousePosition(), rP)) focus = 3; else if(CheckCollisionPointRec(GetMousePosition(), rN)) focus = 5; }
            DrawRectangleLinesEx(rS, (focus==2)?2:1, (focus==2)?RED:DARKGRAY); DrawSText(fHD, sIn, rS, 20, WHITE, (focus==2), frames, "SERVICE/SITE");
            DrawRectangleLinesEx(rP, (focus==3)?2:1, (focus==3)?RED:DARKGRAY); DrawSText(fHD, passIn, rP, 20, WHITE, (focus==3), frames, "PASSWORD");
            DrawRectangleLinesEx(rN, (focus==5)?2:1, (focus==5)?RED:DARKGRAY); DrawSText(fHD, noteIn, rN, 20, WHITE, (focus==5), frames, "NOTE...");
            Color sCol = GetPassStrengthColor(passIn);
            float sWidth = fminf(strlen(passIn) * 15.0f, 250.0f);
            DrawRectangle((int)rP.x, (int)rP.y + 38, 250, 4, DARKGRAY);
            DrawRectangle((int)rP.x, (int)rP.y + 38, (int)sWidth, 4, sCol);
            if(CheckCollisionPointRec(GetMousePosition(), bGen) && IsMouseButtonPressed(0)) strncpy(passIn, generaPass().c_str(), 127);
            DrawRectangleRec(bGen, DARKGRAY); DrawCenteredText(fHD, "#", bGen, 18, WHITE);
            bool hA = CheckCollisionPointRec(GetMousePosition(), bA);
            DrawRectangleRec(bA, hA ? GREEN : GetColor(0x008010FF)); DrawCenteredText(fHD, editingIndex != -1 ? "UPDATE" : "SAVE", bA, 20, WHITE);
            if(hA && IsMouseButtonPressed(0)) {
                if(editingIndex != -1) { elenco[editingIndex].dati = string(sIn) + " : " + string(passIn); elenco[editingIndex].note = noteIn; editingIndex = -1; }
                else if(strlen(sIn)>0) elenco.push_back({string(sIn)+" : "+string(passIn), string(noteIn), 45.0f, 0.0f, 0.0f, false, false});
                salva(elenco); menuAperto = false; memset(sIn,0,128); memset(passIn,0,128); memset(noteIn,0,256);
            }
        }

        // --- GESTIONE INPUT ---
        char* t = (focus==1)?cIn:(focus==2)?sIn:(focus==3)?passIn:(focus==5)?noteIn:(!login?pIn:nullptr);
        if(t) {
            int k = GetCharPressed();
            while(k > 0) { if(strlen(t) < 120) { int l = (int)strlen(t); t[l] = (char)k; t[l+1] = '\0'; } k = GetCharPressed(); }
            if(IsKeyPressed(KEY_BACKSPACE) && strlen(t) > 0) t[strlen(t)-1] = '\0';
            if(IsKeyPressed(KEY_ENTER) && !login) {
                if (primoAvvio) { masterPass = pIn; salvaMaster(masterPass); primoAvvio = false; memset(pIn, 0, 64); }
                else if (modalitaReset) { if (string(pIn) == masterPass) { primoAvvio = true; modalitaReset = false; memset(pIn, 0, 64); } else { erroreLogin = true; errorShake = 1.0f; } }
                else { if (string(pIn) == masterPass) login = true; else { erroreLogin = true; errorShake = 1.0f; } }
            }
        }
        if (deleteIndex != -1) {
            DrawRectangle(0,0,cw,ch, {0,0,0,200}); DrawCenteredText(fHD, "CONFIRM DELETE? (Y/N)", {0,(float)ch/2, (float)cw, 50}, 30, WHITE);
            if(IsKeyPressed(KEY_Y)) { elenco.erase(elenco.begin()+deleteIndex); salva(elenco); deleteIndex = -1; }
            if(IsKeyPressed(KEY_N)) deleteIndex = -1;
        }
        if(copyTimer > 0) DrawRectangle(cw/2-100, 20, 200, 40, DARKGREEN), DrawCenteredText(fHD, "SUCCESS!", {(float)cw/2-100, 20, 200, 40}, 20, WHITE);
        if (login && timerInattivita > 25.0f) DrawRectangle(0, 0, (int)(cw * (1.0f - (timerInattivita-25.0f)/5.0f)), 3, RED);
        EndDrawing();
    }
    UnloadFont(fHD); CloseWindow(); return 0;
}