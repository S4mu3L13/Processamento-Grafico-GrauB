#ifndef M6_TILEMAP_VIEW_H
#define M6_TILEMAP_VIEW_H

// Interface para estratégias de conversão mapa -> tela
class TilemapView {
public:
    virtual ~TilemapView() {
    }

    // Calcula onde um tile deve ser desenhado em pixels de tela
    virtual void computeDrawPosition(
        float col,
        float row,
        float tileW,
        float tileH,
        float originX,
        float originY,
        float& targetX,
        float& targetY
    ) const = 0;
};



#endif
