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
    string tag; 
    float altezzaCorrente = 45.0f;
    float alpha = 0.0f; 
    float alphaNote = 0.0f; 
    bool espanso = false;
    bool visibile = false; 
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float size;
    float alpha;
    float speedConf;
};

// --- LOGICA DI BACKEND ---
// --- LOGICA DI SICUREZZA ---
string stringToHex(const string& input) {
    static const char hex_digits[] = "0123456789ABCDEF";
    string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input) {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}

string hexToString(const string& input) {
    if (input.length() % 2 != 0) return "";
    string output;
    output.reserve(input.length() / 2);
    for (size_t i = 0; i < input.length(); i += 2) {
        string byteString = input.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), nullptr, 16);
        output.push_back(byte);
    }
    return output;
}

string xorCipher(string data, string key) {
    if (key.empty()) return data;
    string result = data;
    for (size_t i = 0; i < data.size(); i++) {
        result[i] = data[i] ^ key[i % key.size()];
    }
    return result;
}

// Chiave statica per offuscare la Master Password salvata (Security through Obscurity, ma meglio di plain text)
const string SYSTEM_KEY = "DrawerSysKey_99!"; 

void salvaMaster(string p) {
    ofstream f("config.dat");
    // Cifra la master pass con la chiave di sistema e converti in hex
    f << stringToHex(xorCipher(p, SYSTEM_KEY));
    f.close();
}

string caricaMaster() {
    ifstream f("config.dat");
    string pHex; getline(f, pHex);
    f.close();
    // Decodifica hex e decifra con la chiave di sistema
    return xorCipher(hexToString(pHex), SYSTEM_KEY);
}

void salva(const vector<Credenziale>& elenco, const string& masterKey) {
    ofstream f("vault.txt");
    for (const auto& c : elenco) {
        // Cifra i dati usando la Master Password dell'utente come chiave
        string encDati = stringToHex(xorCipher(c.dati, masterKey));
        string encNote = stringToHex(xorCipher(c.note, masterKey));
        string encTag = stringToHex(xorCipher(c.tag, masterKey));
        f << encDati << "|" << encNote << "|" << encTag << endl;
    }
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

// --- NUOVI EFFETTI GRAFICI ---
void DrawRoundedCard(Rectangle rec, Color col) {
    // Ombra
    DrawRectangleRounded({rec.x + 3, rec.y + 3, rec.width, rec.height}, 0.2, 6, Fade(BLACK, 0.4f));
    // Corpo
    DrawRectangleRounded(rec, 0.2, 6, col);
    // Bordo sottile
    DrawRectangleRoundedLines(rec, 0.2, 6, Fade(WHITE, 0.05f));
}

void DrawNeonFocus(Rectangle rec, Color glowCol, float frames) {
    float pulse = sinf(frames * 0.1f) * 0.5f + 0.5f; // 0.0 to 1.0 pulsante
    // Glow esterno (simulato disegnando un rettangolo leggermente più largo)
    Rectangle rGlow = {rec.x - 2, rec.y - 2, rec.width + 4, rec.height + 4};
    DrawRectangleRoundedLines(rGlow, 0.2, 6, Fade(glowCol, 0.3f * pulse));
    DrawRectangleRoundedLines(rec, 0.2, 6, glowCol);
}

bool DrawButtonModern(Rectangle rec, const char* text, Color baseCol, Font font, float fontSize) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), rec);
    Rectangle drawRec = rec;
    Color drawCol = baseCol;
    
    if (hover) {
        // Effetto scala leggero e luminosità
        drawRec = { rec.x - 2, rec.y - 2, rec.width + 4, rec.height + 4 };
        drawCol = ColorBrightness(baseCol, 0.2f);
    }
    
    DrawRectangleRounded(drawRec, 0.3, 6, drawCol);
    DrawRectangleRoundedLines(drawRec, 0.3, 6, Fade(WHITE, 0.2f));
    DrawCenteredText(font, text, drawRec, fontSize, WHITE);
    
    return hover && IsMouseButtonPressed(0);
}

// Versione aggiornata di DrawSText per usare i nuovi stili
void DrawSTextModern(Font font, const char* text, Rectangle rec, float size, Color col, bool focus, int frms, const char* ph, Color glowCol) {
    if (focus) DrawNeonFocus(rec, glowCol, (float)frms);
    else DrawRectangleRoundedLines(rec, 0.1, 6, Fade(GRAY, 0.5f));
    
    float tw = MeasureTextEx(font, text, size, 1).x;
    float off = (tw > rec.width - 20) ? tw - (rec.width - 20) : 0;
    
    BeginScissorMode((int)rec.x + 5, (int)rec.y, (int)rec.width - 10, (int)rec.height);
        if (strlen(text) == 0 && !focus) DrawTextEx(font, ph, { rec.x + 10, rec.y + rec.height/2 - size/2 }, size, 1, Fade(GRAY, 0.7f));
        else DrawTextEx(font, text, { rec.x + 10 - off, rec.y + rec.height/2 - size/2 }, size, 1, col);
        if (focus && ((frms/30)%2)==0) DrawRectangle((int)(rec.x + 10 + tw - off + 2), (int)(rec.y + rec.height/2 - size/2), 2, (int)size, glowCol);
    EndScissorMode();
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
    static char cIn[128] = { 0 }; static char noteIn[256] = { 0 }; static char tagIn[64] = { 0 };
    
    vector<Credenziale> elenco;
    string masterPass = primoAvvio ? "" : caricaMaster();

    // Inizializzazione Particelle
    vector<Particle> particles;
    for(int i=0; i<60; i++) {
        particles.push_back({
            {(float)GetRandomValue(0, sw), (float)GetRandomValue(0, sh)},
            {0, (float)GetRandomValue(-20, -5) / 10.0f}, // Verso l'alto
            (float)GetRandomValue(2, 5),
            (float)GetRandomValue(1, 5) / 10.0f,
            (float)GetRandomValue(5, 10) / 10.0f
        });
    }

    float transX = 0.0f, menuY = (float)sh - 40, copyTimer = 0.0f, scrollY = 0.0f, errorShake = 0.0f;
    float clipboardWipeTimer = 0.0f; 
    float timerInattivita = 0.0f; 

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        frames++;
        if (copyTimer > 0) copyTimer -= dt; // Solo per UI "Copied!"
        if (clipboardWipeTimer > 0) {
            clipboardWipeTimer -= dt;
            if (clipboardWipeTimer <= 0) {
                SetClipboardText(""); // WIPE!
            }
        }
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
            DrawCenteredText(fHD, "SECURE OFFLINE VAULT  |  XOR-HEX ENCRYPTION  |  LOCAL STORAGE", {0, (float)ch/2 - 30, (float)cw, 30}, 20, DARKGRAY);
            
            Rectangle bStart = { (float)cw/2-150, (float)ch/2 + 180, 300, 65 };
            if (DrawButtonModern(bStart, "START SESSION", RED, fHD, 24)) mostraIntro = false;
            EndDrawing(); continue;
        }

        float targetX = login ? -(float)cw : 0.0f;
        transX += (targetX - transX) * (1.0f - expf(-10.0f * dt));
        
        if (login && !caricato && transX < -cw/2) {
            ifstream f("vault.txt"); string r;
            while (getline(f, r)) {
                size_t pos1 = r.find('|');
                if (pos1 != string::npos) {
                    string rawDati = hexToString(r.substr(0, pos1));
                    
                    size_t pos2 = r.find('|', pos1 + 1);
                    string rawNote, rawTag;
                    
                    if (pos2 != string::npos) {
                        // Formato nuovo: Dati|Note|Tag
                        rawNote = hexToString(r.substr(pos1 + 1, pos2 - (pos1 + 1)));
                        rawTag = hexToString(r.substr(pos2 + 1));
                    } else {
                        // Formato vecchio (compatibilità): Dati|Note
                        rawNote = hexToString(r.substr(pos1 + 1));
                        rawTag = "";
                    }

                    // Decifra usando la masterPass inserita (pIn validato o caricato)
                    elenco.push_back({xorCipher(rawDati, masterPass), xorCipher(rawNote, masterPass), xorCipher(rawTag, masterPass), 45.0f, 0.0f, 0.0f, false, false});
                }
            }
            f.close(); caricato = true;
            // Ordina per Tag (i vuoti alla fine o inizio, qui li metto in fondo se non gestito diversamente, ma string compare li gestisce)
            sort(elenco.begin(), elenco.end(), [](const Credenziale& a, const Credenziale& b) {
                if (a.tag.empty() && !b.tag.empty()) return false; // Vuoti dopo
                if (!a.tag.empty() && b.tag.empty()) return true;
                if (a.tag == b.tag) return a.dati < b.dati; // Alfabetico secondario
                return a.tag < b.tag;
            });
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
            
            // Draw Dynamic Background Particles
            for(auto& p : particles) {
                p.pos.y += p.vel.y * p.speedConf;
                if(p.pos.y < -10) { p.pos.y = sh + 10; p.pos.x = GetRandomValue(0, cw); }
                DrawRectangleRounded({p.pos.x + ox, p.pos.y, p.size, p.size}, 0.5, 2, Fade(RED, p.alpha * 0.4f));
            }
            
            float lAlpha = 1.0f + (transX / cw); 
            // Removed static grid lines for cleaner look
            const char* tit = primoAvvio ? "CREATE MASTER PASSWORD" : (modalitaReset ? "VERIFY OLD PASSWORD" : "LOGIN");
            DrawCenteredText(fHD, tit, {ox, (float)ch/2 - 100, (float)cw, 50}, 32, Fade(RED, lAlpha));
            Rectangle rL = { ox + (float)cw/2 - 150, (float)ch/2 - 30, 300, 50 };
            
            DrawSTextModern(fHD, string(strlen(pIn), '*').c_str(), rL, 28, Fade(WHITE, lAlpha), !login, frames, "KEY", RED);
            
            if (erroreLogin) DrawCenteredText(fHD, "ACCESS DENIED", {ox, (float)ch/2 + 80, (float)cw, 20}, 20, Fade(RED, lAlpha));
            
            if (!primoAvvio) {
                Rectangle bR = { ox + (float)cw/2 - 120, (float)ch/2 + 35, 240, 30 };
                if (DrawButtonModern(bR, modalitaReset ? "CANCEL" : "CHANGE MASTER PASSWORD", Fade(GRAY, lAlpha * 0.2f), fHD, 15)) {
                     modalitaReset = !modalitaReset; memset(pIn, 0, 64); erroreLogin = false; 
                }
            }
        }

        if (transX < -1) { 
            float ox = transX + cw;
            float vAlpha = -transX / cw; 
            
            // Sfondo Dashboard
            DrawRectangle(ox, 0, cw, 80, GetColor(0x101010FF));
            // Ombra Header
            DrawRectangleGradientV(ox, 80, cw, 20, {0,0,0,100}, {0,0,0,0});

            Rectangle rLogout = { ox + 30, 25, 120, 35 };
            if(DrawButtonModern(rLogout, "LOGOUT", MAROON, fHD, 20)) { login = false; focus = 0; memset(pIn, 0, 64); }

            Rectangle rSearch = { ox + (float)cw - 350, 25, 320, 35 };
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), rSearch)) focus = 1;
            
            DrawSTextModern(fHD, cIn, rSearch, 20, Fade(WHITE, vAlpha), (focus==1), frames, "SEARCH...", RED);
            
            float totalH = 20.0f; 
            string lastTagCalc = "";
            bool firstCalc = true;
            for(const auto& c : elenco) {
                if(strlen(cIn) == 0 || (c.dati.find(cIn) != string::npos || c.note.find(cIn) != string::npos || c.tag.find(cIn) != string::npos)) {
                     // Check se serve header
                     if (firstCalc || c.tag != lastTagCalc) {
                         totalH += 40.0f; // Spazio Header
                         lastTagCalc = c.tag;
                         firstCalc = false;
                     }
                     totalH += c.altezzaCorrente + 10;
                }
            }
            float viewportH = menuY - 75.0f;
            scrollY += GetMouseWheelMove() * 38.0f;
            if (scrollY < -(totalH - viewportH + 50) && totalH > viewportH) scrollY = -(totalH - viewportH + 50);
            if (scrollY > 0 || totalH <= viewportH) scrollY = 0;

            BeginScissorMode((int)ox, 75, cw, (int)menuY - 75);
                float curY = 90 + scrollY;
                string currentTag = "###_START_###";
                
                for(int i = 0; i < (int)elenco.size(); i++) {
                    if(strlen(cIn) > 0 && (elenco[i].dati.find(cIn) == string::npos && elenco[i].note.find(cIn) == string::npos && elenco[i].tag.find(cIn) == string::npos)) continue;
                    
                    // Header Sezione
                    if (elenco[i].tag != currentTag) {
                        currentTag = elenco[i].tag;
                        DrawTextEx(fHD, currentTag.empty() ? "GENERAL" : currentTag.c_str(), { ox + 30, curY }, 24, 1, RED);
                        DrawLine(ox+30, curY+28, ox+cw-30, curY+28, Fade(RED, 0.5f));
                        curY += 40;
                    }

                    Rectangle itR = { ox + 30, curY, (float)cw-60, elenco[i].altezzaCorrente };
                    DrawRoundedCard(itR, Fade(GetColor(0x1E1E1EFF), elenco[i].alpha)); // Usa Card invece di Rect
                    
                    Color txtC = Fade(WHITE, elenco[i].alpha);
                    string sDisp = elenco[i].dati;
                    
                    if(!elenco[i].visibile) { size_t posS = sDisp.find(" : "); if(posS != string::npos) sDisp = sDisp.substr(0, posS+3) + "********"; }
                    DrawTextEx(fHD, sDisp.c_str(), { ox + 45, curY+13 }, 20, 1, txtC);
                    
                    Rectangle bVw = { ox+cw-380, curY+5, 80, 35 }, bCp = { ox+cw-290, curY+5, 80, 35 }, bEd = { ox+cw-200, curY+5, 80, 35 }, bDl = { ox+cw-110, curY+5, 80, 35 };
                    
                    if(DrawButtonModern(bVw, elenco[i].visibile ? "HIDE" : "VIEW", ORANGE, fHD, 16) && deleteIndex == -1) elenco[i].visibile = !elenco[i].visibile;
                    
                    if(DrawButtonModern(bCp, "COPY", BLUE, fHD, 16) && deleteIndex == -1) { 
                        size_t s = elenco[i].dati.find(" : "); if(s != string::npos) { SetClipboardText(elenco[i].dati.substr(s+3).c_str()); copyTimer = 1.5f; clipboardWipeTimer = 30.0f; } 
                    }
                    
                    if(DrawButtonModern(bEd, "EDIT", GRAY, fHD, 16) && deleteIndex == -1) {
                         editingIndex = i; string d = elenco[i].dati; size_t s = d.find(" : "); if(s != string::npos) { strncpy(sIn, d.substr(0, s).c_str(), 127); strncpy(passIn, d.substr(s+3).c_str(), 127); } strncpy(noteIn, elenco[i].note.c_str(), 255); strncpy(tagIn, elenco[i].tag.c_str(), 63); menuAperto = true; focus = 2; 
                    }
                    
                    if(DrawButtonModern(bDl, "DEL", MAROON, fHD, 16) && deleteIndex == -1) deleteIndex = i;
                    
                    if(elenco[i].alphaNote > 0.05f) DrawTextEx(fHD, TextFormat("NOTE: %s", elenco[i].note.empty() ? "NONE" : elenco[i].note.c_str()), { ox+95, curY+55 }, 18, 1, Fade(GRAY, elenco[i].alphaNote * elenco[i].alpha));
                    
                    // Click espansione su tutta la card (tranne bottoni)
                    if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), {ox+30, curY, (float)cw-400, 45})) { bool s = elenco[i].espanso; for(auto& x : elenco) x.espanso = false; elenco[i].espanso = !s; }
                    
                    curY += elenco[i].altezzaCorrente + 10;
                }
            EndScissorMode();

            // Menu Popup
            DrawRectangle(ox, (int)menuY, cw, ch, GetColor(0x181818FF)); 
            DrawRectangleGradientV((int)ox, (int)menuY - 20, cw, 20, {0,0,0,0}, {0,0,0,100}); // Ombra chiusura
            DrawLine(ox, (int)menuY, ox+cw, (int)menuY, RED);
            
            Rectangle hdl = { ox, menuY, (float)cw, 40 };
            DrawCenteredText(fHD, menuAperto ? "[ CLOSE ]" : "[ ADD NEW CREDENTIAL ]", hdl, 18, RED);
            if(IsMouseButtonPressed(0) && CheckCollisionPointRec(GetMousePosition(), hdl)) { if(menuAperto) { memset(sIn,0,128); memset(passIn,0,128); memset(noteIn,0,256); memset(tagIn,0,64); editingIndex = -1; } menuAperto = !menuAperto; focus = 0; }
            float mY = menuY + 55;
            // Ripristino Layout Originale per Riga 1
            Rectangle rS = {ox+30, mY, 250, 35};
            Rectangle rP = {ox+290, mY, 250, 35};
            Rectangle bGen = {ox+550, mY, 40, 35};
            Rectangle bA = {ox+(float)cw-320, mY, 290, 35};
            
            // Riga 2 modificata: Tag + Note
            Rectangle rTag = {ox+30, mY+55, 180, 35};
            Rectangle rN = {ox+220, mY+55, (float)cw-250, 35};

            if(IsMouseButtonPressed(0)) { 
                if(CheckCollisionPointRec(GetMousePosition(), rS)) focus = 2; 
                else if(CheckCollisionPointRec(GetMousePosition(), rTag)) focus = 6;
                else if(CheckCollisionPointRec(GetMousePosition(), rP)) focus = 3; 
                else if(CheckCollisionPointRec(GetMousePosition(), rN)) focus = 5; 
            }

            DrawSTextModern(fHD, sIn, rS, 20, WHITE, (focus==2), frames, "SERVICE/SITE", RED);
            DrawSTextModern(fHD, passIn, rP, 20, WHITE, (focus==3), frames, "PASSWORD", RED);
            
            // Forza password sotto rP
            Color sCol = GetPassStrengthColor(passIn);
            float sWidth = fminf(strlen(passIn) * 15.0f, 250.0f);
            DrawRectangle((int)rP.x, (int)rP.y + 38, 250, 4, DARKGRAY);
            DrawRectangle((int)rP.x, (int)rP.y + 38, (int)sWidth, 4, sCol);

            // Generatore
            if(DrawButtonModern(bGen, "#", DARKGRAY, fHD, 18)) strncpy(passIn, generaPass().c_str(), 127);
            
            // Riga 2: Tag e Note
            DrawSTextModern(fHD, tagIn, rTag, 20, WHITE, (focus==6), frames, "TAG (opt)", SKYBLUE);
            DrawSTextModern(fHD, noteIn, rN, 20, WHITE, (focus==5), frames, "NOTE...", RED);

            // Bottone Salva
            if(DrawButtonModern(bA, editingIndex != -1 ? "UPDATE" : "SAVE", GetColor(0x008010FF), fHD, 20)) {
                if(editingIndex != -1) { elenco[editingIndex].dati = string(sIn) + " : " + string(passIn); elenco[editingIndex].note = noteIn; elenco[editingIndex].tag = tagIn; editingIndex = -1; }
                else if(strlen(sIn)>0) elenco.push_back({string(sIn)+" : "+string(passIn), string(noteIn), string(tagIn), 45.0f, 0.0f, 0.0f, false, false});
                
                // Riorganizza dopo salvataggio
                sort(elenco.begin(), elenco.end(), [](const Credenziale& a, const Credenziale& b) {
                    if (a.tag.empty() && !b.tag.empty()) return false;
                    if (!a.tag.empty() && b.tag.empty()) return true;
                    if (a.tag == b.tag) return a.dati < b.dati; 
                    return a.tag < b.tag;
                });
                
                salva(elenco, masterPass); menuAperto = false; memset(sIn,0,128); memset(passIn,0,128); memset(noteIn,0,256); memset(tagIn,0,64);
            }
        }

        // --- GESTIONE INPUT ---
        char* t = (focus==1)?cIn:(focus==2)?sIn:(focus==3)?passIn:(focus==5)?noteIn:(focus==6)?tagIn:(!login?pIn:nullptr);
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


        // --- GESTIONE POPUP ELIMINAZIONE ---
        if (deleteIndex != -1) {
             // Overlay
             DrawRectangle(0,0,cw,ch, {0,0,0,200});
             
             // Popup Card
             Rectangle rPop = {(float)cw/2 - 200, (float)ch/2 - 100, 400, 200};
             DrawRoundedCard(rPop, GetColor(0x202020FF));
             
             // Testo
             DrawCenteredText(fHD, "DELETE CREDENTIAL?", {rPop.x, rPop.y + 20, rPop.width, 40}, 24, WHITE);
             DrawCenteredText(fHD, "This action cannot be undone.", {rPop.x, rPop.y + 60, rPop.width, 30}, 18, GRAY);
             
             // Bottoni
             Rectangle bYes = {rPop.x + 40, rPop.y + 130, 140, 40};
             Rectangle bNo = {rPop.x + 220, rPop.y + 130, 140, 40};
             
             if(DrawButtonModern(bYes, "DELETE", RED, fHD, 20)) {
                 elenco.erase(elenco.begin()+deleteIndex); 
                 salva(elenco, masterPass); 
                 deleteIndex = -1; 
             }
             
             if(DrawButtonModern(bNo, "CANCEL", GRAY, fHD, 20)) {
                 deleteIndex = -1;
             }
        }
        if(copyTimer > 0) DrawRectangle(cw/2-100, 20, 200, 40, DARKGREEN), DrawCenteredText(fHD, "SUCCESS!", {(float)cw/2-100, 20, 200, 40}, 20, WHITE);
        if (login && timerInattivita > 25.0f) DrawRectangle(0, 0, (int)(cw * (1.0f - (timerInattivita-25.0f)/5.0f)), 3, RED);
        EndDrawing();
    }
    UnloadFont(fHD); CloseWindow(); return 0;
}