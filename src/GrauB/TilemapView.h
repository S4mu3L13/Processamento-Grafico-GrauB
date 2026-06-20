#ifndef AV3_TILEMAP_VIEW_H
#define AV3_TILEMAP_VIEW_H

class TilemapView {
public:
    virtual ~TilemapView() = default;

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
