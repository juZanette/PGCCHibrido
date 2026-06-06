#ifndef AV3_DIAMOND_VIEW_H
#define AV3_DIAMOND_VIEW_H

#include "TilemapView.h"

class DiamondView : public TilemapView {
public:
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
