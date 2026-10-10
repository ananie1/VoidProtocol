#pragma once
// Интерфейс: HUD, кнопки меню, карточки улучшений, прицел.
// Весь текст рисуется шрифтом raylib по умолчанию (латиница), поэтому внешних ассетов не нужно.
#include "Common.h"
#include "Enemy.h"
#include "Player.h"
#include "World.h"

#include <string>
#include <vector>

namespace ui {

void Label(const char* text, int x, int y, int size, Color color);
void LabelCentered(const char* text, int cx, int y, int size, Color color);
void LabelRight(const char* text, int rx, int y, int size, Color color);
void Bar(Rectangle r, float t, Color fill, Color back, Color border);

// Вертикальный список кнопок с навигацией мышью и клавиатурой (W/S, стрелки, Enter).
class ButtonList {
public:
    ButtonList(std::vector<std::string> labels, float topY);
    void Reset() { selected_ = 0; }
    int Update();  // индекс нажатой кнопки или -1
    void Draw(float time) const;

private:
    Rectangle RectOf(int index) const;
    std::vector<std::string> labels_;
    float topY_ = 0.0f;
    int selected_ = 0;
};

struct HudState {
    const Player* player = nullptr;
    const World* world = nullptr;
    const Enemy* boss = nullptr;  // активный босс (или nullptr)
    int score = 0;
    float runTime = 0.0f;
    float time = 0.0f;
    bool roomActive = false;
    int wave = 0;
    int waveTotal = 0;
    int hostiles = 0;
};

void DrawHUD(const HudState& hud);
void DrawCrosshair(Vector2 mouse, bool reloading, float time);

Rectangle UpgradeCardRect(int index);
void DrawUpgradeCard(Rectangle r, Upgrade u, int index, bool selected, float time);

}  // namespace ui
