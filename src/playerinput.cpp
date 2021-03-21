// PlayerInput class
#include "header/playerinput.hpp"
#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

PlayerInput::PlayerInput(unsigned int maxBuf, unsigned int inpDelay) : inputDelayMillisecond(inpDelay), maxInputBuffer(maxBuf), inputLock(), inputThread(NULL), isRunning(false) {

}

PlayerInput::~PlayerInput() {
    stopReadInput();
}

void PlayerInput::inputLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(inputDelayMillisecond));
        if (inputBuffer.size() < maxInputBuffer) {
            // Critical section, blocking input buffer processing
            inputLock.lock();
            if ((GetKeyState(VK_UP) & 0x8000) || (GetKeyState('W') & 0x8000))
                inputBuffer.push(Up);
            if ((GetKeyState(VK_DOWN) & 0x8000) || (GetKeyState('S') & 0x8000))
                inputBuffer.push(Down);
            if ((GetKeyState(VK_LEFT) & 0x8000) || (GetKeyState('A') & 0x8000))
                inputBuffer.push(Left);
            if ((GetKeyState(VK_RIGHT) & 0x8000) || (GetKeyState('D') & 0x8000))
                inputBuffer.push(Right);
            if ((GetKeyState('1') & 0x8000))
                inputBuffer.push(Number1);
            if ((GetKeyState('2') & 0x8000))
                inputBuffer.push(Number2);
            if ((GetKeyState('3') & 0x8000))
                inputBuffer.push(Number3);
            if ((GetKeyState('4') & 0x8000))
                inputBuffer.push(Number4);
            if ((GetKeyState('E') & 0x8000))
                inputBuffer.push(KeyboardE);
            if ((GetKeyState(VK_ESCAPE) & 0x8000))
                inputBuffer.push(EscKey);
            inputLock.unlock();
        }
    }
}

void PlayerInput::startReadInput() {
    // TODO : Extra, potential memory leak
    inputThread = new std::thread(&PlayerInput::inputLoop, this);
    isRunning = true;
}
// Start input reading
void PlayerInput::stopReadInput() {
    isRunning = false;
}

InputType PlayerInput::getUserInput() {
    // Critical section, blocking input buffer from changing
    // Until empty() and pop() method completed or return EmptyInput
    inputLock.lock();
    if (!inputBuffer.empty()) {
        InputType frontQueueInput = inputBuffer.front();
        inputBuffer.pop();
        inputLock.unlock();
        return frontQueueInput;
    }
    else {
        inputLock.unlock();
        return EmptyInput;
    }
}
