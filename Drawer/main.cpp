#define RAYLIB_PLATFORM_DESKTOP
#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cmath>

using namespace std;

struct Credenziale {
    string dati; string note;
    float altezzaCorrente = 45.0f; 
    bool espanso = false;
};

// --- LOGICA ---
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

// --- DISEGNO ---
void DrawSText(Font font, const char* text, Rectangle rec, float size, Color col, bool focus, int frms, const char* ph) {
    float tw = MeasureTextEx(font, text, size, 1).x;
    float off = (tw > rec.width - 20) ? tw - (rec.width - 20) : 0;
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

int main() {
    const int sw = 1000; const int sh = 750;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(sw, sh, "Drawer - The Ultimate Password Manager");
    SetTargetFPS(60);

    Font fHD = LoadFontEx("arial.ttf", 64, 0, 250); 
    SetTextureFilter(fHD.texture, TEXTURE_FILTER_BILINEAR);

    bool login = false, erroreLogin = false, caricato = false, menuAperto = false;
    bool primoAvvio = !FileExists("config.dat");
    
    static char pIn[64] = ""; static char sIn[128] = ""; static char passIn[128] = ""; 
    static char cIn[64] = ""; static char idIn[8] = ""; static char noteIn[256] = "";
    
    vector<Credenziale> elenco;
    int focus = 0, frames = 0, editingIndex = -1;
    float menuY = (float)sh - 40;
    
    // Variabili Animazione
    float loginXOffset = 0.0f;
    float vaultXOffset = (float)sw;
    float startupAlpha = 0.0f;
    float startupY = 40.0f;

    string masterPass = primoAvvio ? "" : caricaMaster();

    while (!WindowShouldClose()) {
        frames++;

        // Animazione iniziale e transizioni
        if (!login) {
            startupAlpha += (1.0f - startupAlpha) * 0.1f;
            startupY += (0.0f - startupY) * 0.1f;
            loginXOffset += (0.0f - loginXOffset) * 0.12f;
            vaultXOffset += (sw - vaultXOffset) * 0.12f;
        } else {
            loginXOffset += (-sw - loginXOffset) * 0.12f;
            vaultXOffset += (0.0f - vaultXOffset) * 0.12f;
            if (!caricato && vaultXOffset < 100) {
                ifstream f("vault.txt"); string r;
                while (getline(f, r)) {
                    size_t pos = r.find('|');
                    if (pos != string::npos) elenco.push_back({decripta(r.substr(0, pos)), decripta(r.substr(pos+1)), 45.0f, false});
                }
                f.close(); caricato = true;
            }
        }

        menuY += ((menuAperto ? (float)sh - 240 : (float)sh - 40) - menuY) * 0.15f;
        for(auto& c : elenco) {
            float targetH = c.espanso ? 110.0f : 45.0f;
            c.altezzaCorrente += (targetH - c.altezzaCorrente) * 0.15f;
        }

        BeginDrawing();
        ClearBackground(GetColor(0x121212FF));

        // --- LOGIN ---
        if (loginXOffset > -sw + 10) {
            float ox = loginXOffset;
            Color mainCol = RED; mainCol.a = (unsigned char)(startupAlpha * 255);
            Color fadeWhite = WHITE; fadeWhite.a = (unsigned char)(startupAlpha * 255);

            DrawRectangle(ox, 0, sw, sh, GetColor(0x1A1A1AFF));
            DrawCenteredText(fHD, primoAvvio ? "SET MASTER PASSWORD" : "VAULT LOGIN", {ox, 250 + startupY, (float)sw, 40}, 30, mainCol);
            
            Rectangle rL = { ox + sw/2 - 150, 320 + startupY, 300, 50 };
            DrawRectangleLinesEx(rL, 2, mainCol);
            
            if (erroreLogin) DrawCenteredText(fHD, "WRONG PASSWORD!", {ox, 385 + startupY, (float)sw, 20}, 18, RED);
            
            string ast(strlen(pIn), '*');
            DrawSText(fHD, ast.c_str(), rL, 26, fadeWhite, true, frames, "PASSWORD");
            
            if (!primoAvvio) {
                Rectangle bR = { ox + sw/2 - 60, 450 + startupY, 120, 30 };
                bool hR = CheckCollisionPointRec(GetMousePosition(), bR);
                DrawCenteredText(fHD, "RESET VAULT", bR, 14, hR ? RED : GRAY);
                if (IsMouseButtonPressed(0) && hR) { primoAvvio = true; remove("config.dat"); memset(pIn,0,64); }
            }

            int k = GetCharPressed();
            while (k > 0) { if (k >= 32 && k <= 125 && strlen(pIn) < 63) { int l = (int)strlen(pIn); pIn[l] = (char)k; pIn[l+1] = '\0'; erroreLogin = false; } k = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE) && strlen(pIn) > 0) pIn[strlen(pIn)-1] = '\0';
            if (IsKeyPressed(KEY_ENTER)) {
                if (primoAvvio) { masterPass = pIn; salvaMaster(masterPass); primoAvvio = false; memset(pIn, 0, 64); }
                else { if (string(pIn) == masterPass) login = true; else { erroreLogin = true; memset(pIn, 0, 64); } }
            }
        }

        // --- VAULT ---
        if (vaultXOffset < sw - 10) {
            float vx = vaultXOffset;
            DrawRectangle(vx, 0, sw, 70, GetColor(0x1A1A1AFF));
            Rectangle rTitle = { vx + 30, 20, 180, 35 };
            bool hTitle = CheckCollisionPointRec(GetMousePosition(), rTitle);
            
            DrawTextEx(fHD, "RETURN TO LOGIN", { vx + 30, 20 }, 28, 2, hTitle ? RED : GetColor(0xAA0000FF));
            if(hTitle) {
                DrawRectangle(vx+30, 50, (int)MeasureTextEx(fHD, "RETURN TO LOGIN", 28, 2).x, 2, RED);
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if(IsMouseButtonPressed(0)) { login = false; caricato = false; elenco.clear(); memset(pIn, 0, 64); startupAlpha = 0; startupY = 40; }
            } else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

            Rectangle rC = { vx + 700, 17, 270, 35 };
            DrawRectangleLinesEx(rC, (focus==1)?2:1, (focus==1)?RED:DARKGRAY);
            DrawSText(fHD, cIn, rC, 18, WHITE, (focus==1), frames, "SEARCH...");
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), rC)) focus = 1;

            float curY = 90;
            for(size_t i = 0; i < elenco.size(); i++) {
                if(strlen(cIn) > 0 && elenco[i].dati.find(cIn) == string::npos) continue;
                Rectangle itR = { vx + 30, curY, (float)sw-60, elenco[i].altezzaCorrente };
                DrawRectangleRec(itR, GetColor(0x1E1E1EFF));
                
                if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), {vx+30, curY, (float)sw-200, 45})) {
                    bool s = elenco[i].espanso; for(auto& c : elenco) c.espanso = false; elenco[i].espanso = !s;
                }
                
                DrawTextEx(fHD, TextFormat("%02d", (int)i+1), { vx + 50, curY+10 }, 20, 1, RED);
                DrawTextEx(fHD, elenco[i].dati.c_str(), { vx + 100, curY+10 }, 20, 1, WHITE);
                
                if(elenco[i].altezzaCorrente > 60.0f) {
                    float a = (elenco[i].altezzaCorrente-45.0f)/65.0f; 
                    Color cN = elenco[i].note.empty()?MAROON:GRAY; cN.a=(unsigned char)(a*255);
                    DrawTextEx(fHD, TextFormat("NOTE: %s", elenco[i].note.empty()?"EMPTY":elenco[i].note.c_str()), { vx + 100, curY+45 }, 16, 1, cN);
                    
                    // --- BOTTONE MODIFICA ANIMATO ---
                    Rectangle bM = { vx + sw-180, curY+40, 140, 30 };
                    bool hM = CheckCollisionPointRec(GetMousePosition(), bM);
                    if(hM) { bM.x += 5; bM.width -= 5; } // Piccolo shift a destra
                    
                    float pulse = (sinf(frames * 0.1f) * 0.5f + 0.5f);
                    Color bCol = hM ? Color{ (unsigned char)(100 + pulse*50), (unsigned char)(100 + pulse*50), (unsigned char)(100 + pulse*50), 255 } : DARKGRAY;
                    
                    DrawRectangleRec(bM, bCol);
                    DrawCenteredText(fHD, "EDIT", bM, 14, WHITE);
                    if(IsMouseButtonPressed(0) && hM) { 
                        editingIndex = (int)i; strncpy(noteIn, elenco[i].note.c_str(), 255); focus = 5; menuAperto = true; 
                    }
                }
                curY += elenco[i].altezzaCorrente + 10;
            }

            // --- MENU ---
            DrawRectangle(vx, (int)menuY, sw, sh, GetColor(0x181818FF));
            DrawLine(vx, (int)menuY, vx + sw, (int)menuY, RED);
            Rectangle hdl = { vx, menuY, (float)sw, 40 };
            DrawCenteredText(fHD, menuAperto ? "V" : "^", hdl, 20, RED);
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), hdl)) menuAperto = !menuAperto;

            float mY = menuY + 55;
            Rectangle rS = {vx+30, mY, 300, 35}, rP = {vx+350, mY, 300, 35}, bA = {vx+680, mY, 290, 35};
            Rectangle rN = {vx+30, mY+50, 620, 35}, rID = {vx+680, mY+50, 80, 35}, bE = {vx+770, mY+50, 200, 35};

            DrawRectangleLinesEx(rS, (focus==2)?2:1, (focus==2)?RED:DARKGRAY); DrawSText(fHD, sIn, rS, 18, WHITE, (focus==2), frames, "SITE");
            DrawRectangleLinesEx(rP, (focus==3)?2:1, (focus==3)?RED:DARKGRAY); DrawSText(fHD, passIn, rP, 18, WHITE, (focus==3), frames, "PASS");
            DrawRectangleLinesEx(rN, (focus==5)?2:1, (focus==5)?RED:DARKGRAY); DrawSText(fHD, noteIn, rN, 18, WHITE, (focus==5), frames, "NOTE...");
            DrawRectangleLinesEx(rID, (focus==4)?2:1, (focus==4)?RED:DARKGRAY); DrawSText(fHD, idIn, rID, 18, WHITE, (focus==4), frames, "ID");

            bool hA = CheckCollisionPointRec(GetMousePosition(), bA);
            DrawRectangleRec(bA, hA ? GREEN : GetColor(0x008010FF)); DrawCenteredText(fHD, editingIndex != -1 ? "SAVE" : "ADD", bA, 18, WHITE);
            bool hE = CheckCollisionPointRec(GetMousePosition(), bE);
            DrawRectangleRec(bE, hE ? RED : MAROON); DrawCenteredText(fHD, "DELETE", bE, 18, WHITE);

            if(IsMouseButtonPressed(0)) {
                if(CheckCollisionPointRec(GetMousePosition(), rS)) focus = 2;
                else if(CheckCollisionPointRec(GetMousePosition(), rP)) focus = 3;
                else if(CheckCollisionPointRec(GetMousePosition(), rN)) focus = 5;
                else if(CheckCollisionPointRec(GetMousePosition(), rID)) focus = 4;
                else if(hA) {
                    if(editingIndex != -1) { elenco[editingIndex].note = noteIn; editingIndex = -1; memset(noteIn, 0, 256); }
                    else if(strlen(sIn) > 0) { elenco.push_back({string(sIn)+" : "+string(passIn), string(noteIn), 45.0f, false}); memset(sIn,0,128); memset(passIn,0,128); }
                    salva(elenco);
                }
                else if(hE && strlen(idIn)>0) { int id = atoi(idIn); if(id>0 && id<=(int)elenco.size()) { elenco.erase(elenco.begin()+id-1); salva(elenco); } }
            }
            char* t = (focus==1)?cIn:(focus==2)?sIn:(focus==3)?passIn:(focus==4)?idIn:(focus==5)?noteIn:nullptr;
            if(t) {
                int k = GetCharPressed();
                while(k > 0) { if(strlen(t) < 63) { int l = (int)strlen(t); t[l] = (char)k; t[l+1] = '\0'; } k = GetCharPressed(); }
                if(IsKeyPressed(KEY_BACKSPACE) && strlen(t) > 0) t[strlen(t)-1] = '\0';
            }
        }
        EndDrawing();
    }
    UnloadFont(fHD); CloseWindow();
    return 0;
}
