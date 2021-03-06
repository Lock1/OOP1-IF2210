// 13519214
// Render class, used internally on engine
// NOTE : Render coordinate (0, 0) starting from upper left corner
// NOTE : Charset used = Extended ASCII
#ifndef RENDER_HPP
#define RENDER_HPP

#include "entities/map.hpp"
#include "message.hpp"
#include <vector>
#include <string>

class Box {
    // Data class for drawing box
    public:
        char cornerBottomLeft;
        char cornerBottomRight;
        char cornerTopLeft;
        char cornerTopRight;
        char lineVertical;
        char lineHorizontal;
        unsigned int sizeX;
        unsigned int sizeY;

        unsigned int offsetX;
        unsigned int offsetY;
};

class Render {
    private:
        // Offset and size values for drawing
        unsigned int mapOffsetX;
        unsigned int mapOffsetY;

        unsigned int msgOffsetX;
        unsigned int msgOffsetY;

        unsigned int cursorRestX;
        unsigned int cursorRestY;

        const unsigned int mapSizeX;
        const unsigned int mapSizeY;

        const unsigned int msgSizeX;
        const unsigned int msgSizeY;

        // Character list for drawing border
        char mapBorderCornerBottomLeft;
        char mapBorderCornerBottomRight;
        char mapBorderCornerTopLeft;
        char mapBorderCornerTopRight;
        char mapBorderLineVertical;
        char mapBorderLineHorizontal;

        std::string msgBoxTitle;
        char msgBorderCornerBottomLeft;
        char msgBorderCornerBottomRight;
        char msgBorderCornerTopLeft;
        char msgBorderCornerTopRight;
        char msgBorderLineVertical;
        char msgBorderLineHorizontal;

        std::vector<std::vector<char>> mapFrameBuffer;
        std::vector<std::vector<bool>> shadowMapFrameBuffer;
        // Map Frame buffer
        std::vector<Position> lastRenderPos;
        // Last drawn position, used for line of sight highlight
        bool isEmptyMapBuffer;
        // Flag whether buffer is already filled or not
        bool isMessageBorderDrawn;
        // Flag whether message box border is drawn or not

        void setCursorPosition(int x, int y);

        // Border method, size need to be initialize first
        void drawBox(Box& target);

        void drawMapBorder();
        void drawMsgBorder();
        // Drawing border

        int floorSqueezedEuclideanMetric(Position pos1, Position pos2); // TODO : Search fast truncated ver
        // Calculating floored euclidean distance

        void drawMessageTitle();
        // Drawing message box title at top left corner

        bool isRayBlocked(Position fromPos, Position toPos);
        // Raycasting checking

        Position rayEndLocation(Position fromPos, Position toPos);
        // Raycasting checking, but returning ray end location

        std::vector<Position> getRenderedArea(Position pos);
        // Get coordinate area for rendering

        int nearestInteger(double x);
        // Rounding to nearest integer

    public:
        Render(Map& target, Message& msgTarget);
        // User Constructor
        Render(Message& msgTarget);
        // User Constructor for message box only renderer

        // Setup
        void setCursorRestLocation(unsigned int x, unsigned int y);
        // Set cursor location after drawing
        void setMapOffset(unsigned int offx, unsigned int offy);
        void setMessageBoxOffset(unsigned int msgoffx, unsigned int msgoffy);
        // Offset setting

        // -- Draw Method --
        // Using double buffering
        // TODO : Extra, Const
        void drawMap(Map& target);
        void drawMap(Map& target, Position renderPos);
        // Drawing map at offset location

        void drawMessageBox(Message& target);
        // Draw message box at offset location

        void clearMessageBox(Message& target);
        // Clear message box

        void setMessageTitle(std::string title);
        // Set message box title, will redrawing if border already drawn

        void clearCursorRestArea();
        // Writing space at rest area

        void drawLoseScreen();
        // Draw lose screen
};

#endif
