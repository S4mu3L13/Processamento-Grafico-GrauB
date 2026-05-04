#ifndef M6_DIAMOND_VIEW_H
#define M6_DIAMOND_VIEW_H

#include "TilemapView.h"

// Projeção isométrica em formato de diamante
class DiamondView : public TilemapView {
public:
    // Converte coordenadas da matriz para coordenadas isométricas na tela
    void computeDrawPosition(
        float col,
        float row,
        float tileW,
        float tileH,
        float originX,
        float originY,
        float& targetX,
        float& targetY
    ) const override {
        targetX = originX + (col - row) * (tileW * 0.5f);
        targetY = originY + (col + row) * (tileH * 0.5f);
    }
};

#endif
