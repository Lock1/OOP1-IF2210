// 13519214
// Render class, used internally on engine
// NOTE : Render coordinate (0, 0) starting from upper left corner
// NOTE : Charset used = Extended ASCII
#ifndef RENDER_HPP
#define RENDER_HPP

#include "entities/map.hpp"
#include "message.hpp"
#include <vector>

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
        char mapBorderNE;
        char mapBorderNW;
        char mapBorderSE;
        char mapBorderSW;
        char mapBorderNS;
        char mapBorderWE;

        char msgBorderNE;
        char msgBorderNW;
        char msgBorderSE;
        char msgBorderSW;
        char msgBorderNS;
        char msgBorderWE;

        std::vector<std::vector<char>> mapFrameBuffer;
        // Map Frame buffer
        bool isEmptyMapBuffer;
        // Flag whether buffer is already filled or not
        bool isMessageBorderDrawn;
        // Flag whether message box border is drawn or not

        void setCursorPosition(int x, int y);

        // Border method, size need to be initialize first
        void drawMapBorder();
        // Drawing map border

        void drawMsgBorder();
        // Drawing message border
    public:
        Render(Map& target, Message& msgTarget);
        // User Constructor

        // Setup
        void setCursorRestLocation(unsigned int x, unsigned int y);
        // Set cursor location after drawing
        void setMapOffset(unsigned int offx, unsigned int offy);
        void setMessageBoxOffset(unsigned int msgoffx, unsigned int msgoffy);
        void setMapBorder();
        void setMessageBorder();
        // Offset setting

        // -- Draw Method --
        // Use double buffering
        // TODO : Const
        void drawMap(Map& target);
        // Drawing map at offset location

        void drawMessageBox(Message& target);
        // Draw message box at offset location

};

#endif
