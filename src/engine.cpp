// 13519214
#include "header/render.hpp"
#include "header/engine.hpp"
#include "header/playerinput.hpp"
#include "header/entities/map.hpp"
#include "header/entities/player.hpp"
#include "header/entities/engimon.hpp"
#include "header/entities/species.hpp"
#include "header/skilldatabase.hpp"
#include "header/speciesdatabase.hpp"
#include "header/battle.hpp"
#include "header/breeding.hpp"
#include "header/entities/attributes/elementtype.hpp"
#include "header/inventory"
#include <iostream>
#include <string>
#include <chrono>       // Time and tick system
#include <thread>       // For sleep()
#include <map>
#include <stdlib.h>
#include <Windows.h>
#include <time.h>

using namespace std;


Engine::Engine() : messageList(MAX_MESSAGE, MSG_MAX_X), statMessage(MAX_MESSAGE-10, MSG_MAX_X-5),
        battleMessage(MAX_MESSAGE-12, MSG_MAX_X-5),
        player(MAX_INVENTORY, MAX_SKILL_ID), maxSkillID(MAX_SKILL_ID),
        // map(MAP_MAX_X, MAP_MAX_Y, SEA_STARTING_X, SEA_STARTING_Y),
        map("../other/mapfile.txt"),
        userInput(INPUT_BUFFER_COUNT, INPUT_DELAY_MS),
        wildEngimonSpawnProbability(4), wildEngimonDropProbability(30),
        entitySpawnLimit(40), xpMultiplier(XP_MULTIPLIER),
        renderer(map, messageList), statRenderer(statMessage), battleRenderer(battleMessage) {
    // Internal variable setup
    srand((unsigned) time(NULL));
    isEngineRunning = true;
    isCommandMode = false;
    spawnLevelCap = 10;
    renderer.setMapOffset(MAP_OFFSET_X, MAP_OFFSET_Y);
    renderer.setMessageBoxOffset(MESSAGE_OFFSET_X, MESSAGE_OFFSET_Y);
    renderer.setCursorRestLocation(CURSOR_REST_X, CURSOR_REST_Y);

    statRenderer.setMessageBoxOffset(MESSAGE_OFFSET_X+messageList.getMaxStringLength()+3, MESSAGE_OFFSET_Y);
    statRenderer.setCursorRestLocation(CURSOR_REST_X, CURSOR_REST_Y);

    battleRenderer.setMessageBoxOffset(MESSAGE_OFFSET_X+messageList.getMaxStringLength()+3, MESSAGE_OFFSET_Y+12);
    battleRenderer.setCursorRestLocation(CURSOR_REST_X, CURSOR_REST_Y);

    // Set message box titles
    string titleBar = "\xC4";
    renderer.setMessageTitle(titleBar + "Message");
    statRenderer.setMessageTitle(titleBar + "Current Engimon Stats");
    battleRenderer.setMessageTitle(titleBar + "Battle");

    // TODO : Add prompt (?)
    // TODO : Add splash screen (?)
    // try {
    //     skillDB.loadSkillDatabase("../other/skilldb.txt");
    // }
    // catch (string e) {
    // }
    try {
        skillDB.loadSkillDatabase("./skilldb.txt");
    }
    catch (string) {}

    // try {
    //     speciesDB.loadSpeciesDatabase("../other/speciesdb.txt", skillDB);
    // }
    // catch (string e) {
    // }
    try {
        speciesDB.loadSpeciesDatabase("./speciesdb.txt", skillDB);
    }
    catch (string) {}
}

Engine::~Engine() {
}

void Engine::clearConsoleInputBuffer() {
    // Source : http://cplusplus.com/forum/beginner/248262/
    PINPUT_RECORD ClearingVar1 = new INPUT_RECORD[256];
    DWORD ClearingVar2;
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),ClearingVar1,256,&ClearingVar2);
    delete[] ClearingVar1;
}

double Engine::getCurrentTimeDelta() {
    endClock = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = endClock - startClock;
    return elapsed_seconds.count();
}

void Engine::startGame() {
    system(CLEAR_SCREEN_CMD);

    startClock = std::chrono::system_clock::now();
    map.setTileEntity(player.getPos(), &player);
    // Starting item
    Engimon *starterEngimon = new Engimon(speciesDB.getSpecies((rand() % 10) + 1), false, Position(0, 0), (rand() % 10) + 1);
    engimonList.push_back(starterEngimon);
    player.changeEngimon(starterEngimon);
    player.addEngimonItem(starterEngimon);
    map.setTileEntity(starterEngimon->getPos(), starterEngimon);


    userInput.startReadInput();
    updateCurrentEngimonMessageStatus();
    showLegendHelp();

    while (isEngineRunning) {
        // Drawing map and message box
        #ifdef LINE_OF_SIGHT
        renderer.drawMap(map, player.getPos());
        #endif

        #ifndef LINE_OF_SIGHT
        renderer.drawMap(map);
        #endif
        renderer.drawMessageBox(messageList);
        statRenderer.drawMessageBox(statMessage);
        battleRenderer.drawMessageBox(battleMessage);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (evaluteInput() && not isCommandMode) {
            evaluteTick();
            startClock = std::chrono::system_clock::now();
        }
        else if (isCommandMode) {
            startClock = std::chrono::system_clock::now();
            commandMode();
            // Call command mode
        }
        // Release turn based tick lock
        #ifdef RELEASE_TICK_LOCK
        if (getCurrentTimeDelta() > 1) {
            evaluteTick();
            startClock = std::chrono::system_clock::now();
        }
        #endif

    }
    userInput.stopReadInput();
}

bool Engine::evaluteInput() {
    InputType inputKey = userInput.getUserInput();
    Tile targetTile = Tile(0, 0, Grass);
    bool isMovementValid = false;
    Direction targetDirection;
    switch (inputKey) {
        case EscKey:
            isEngineRunning = false;
            break;
        case Up:
            if (player.getPos().getY() > 0) {
                targetTile = map.getTileAt(player.getPos().getX(), player.getPos().getY()-1);
                if (player.isMoveLocationValid(targetTile)) {
                    targetDirection = North;
                    isMovementValid = true;
                }
            }
            break;
        case Down:
            if (player.getPos().getY() < map.getSizeY() - 1) {
                targetTile = map.getTileAt(player.getPos().getX(), player.getPos().getY()+1);
                if (player.isMoveLocationValid(targetTile)) {
                    targetDirection = South;
                    isMovementValid = true;
                }
            }
            break;
        case Left:
            if (player.getPos().getX() > 0) {
                targetTile = map.getTileAt(player.getPos().getX()-1, player.getPos().getY());
                if (player.isMoveLocationValid(targetTile)) {
                    targetDirection = West;
                    isMovementValid = true;
                }
            }
            break;
        case Right:
            if (player.getPos().getX() < map.getSizeX() - 1) {
                targetTile = map.getTileAt(player.getPos().getX()+1, player.getPos().getY());
                if (player.isMoveLocationValid(targetTile)) {
                    targetDirection = East;
                    isMovementValid = true;
                }
            }
            break;
        case KeyboardE:
            isCommandMode = true;
            break;
    }

    if (isMovementValid) {
        map.moveEntity(player.getPos(), targetDirection);
        map.moveEntity(player.getCurrentEngimon()->getPos(), player.getLastDirection());
        player.getLastDirectionRef() = targetDirection;
        return true;
    }
    else if (targetTile.getEntity() != NULL) {
        Entity* targetEntity = targetTile.getEntity();
        if (targetEntity->getEntityID() == EntityEngimon) {
            Engimon *targetEngimon = (Engimon *) targetEntity;
            if (not targetEngimon->isWildEngimon()) {
                string interactionMessage = targetEngimon->getEngimonName();
                interactionMessage = interactionMessage + " : ";
                interactionMessage = interactionMessage + targetEngimon->getInteractString();
                messageList.addMessage(interactionMessage);
            }
            else {
                Battle doBattle = Battle(player.getCurrentEngimon(), targetEngimon);
                battleRenderer.clearMessageBox(battleMessage);

                string wildEngimonName = "Species \xB3 ";
                wildEngimonName = wildEngimonName + targetEngimon->getName();
                battleMessage.addMessage(wildEngimonName);

                string wildEngimonLevel = "Lvl     \xB3 ";
                wildEngimonLevel = wildEngimonLevel + to_string(targetEngimon->getLevel());
                battleMessage.addMessage(wildEngimonLevel);

                set<ElementType> elements = targetEngimon->getElements();
                string typeMsg = "Type    \xB3 ";
                if (elements.find(Fire) != elements.end())
                    typeMsg = typeMsg + "Fire ";
                if (elements.find(Ice) != elements.end())
                    typeMsg = typeMsg + "Ice ";
                if (elements.find(Water) != elements.end())
                    typeMsg = typeMsg + "Water ";
                if (elements.find(Ground) != elements.end())
                    typeMsg = typeMsg + "Ground ";
                if (elements.find(Electric) != elements.end())
                    typeMsg = typeMsg + "Electric ";
                battleMessage.addMessage(typeMsg);


                userInput.toggleReadInput();
                // Temporary stop input thread from queueing movement input
                clearConsoleInputBuffer();
                // Clearing current input buffer (GetKeyState() does not clear buffer)
                battleMessage.addMessage("Power   \xB3 " + to_string(doBattle.getEngimon2Power()));
                battleMessage.addMessage("");
                battleMessage.addMessage("Your engimon stat");
                battleMessage.addMessage("Power   \xB3 " + to_string(doBattle.getEngimon1Power()));
                battleMessage.addMessage("Fight ? (yes/no)");
                battleRenderer.drawMessageBox(battleMessage);

                string commandBuffer;
                bool isPromptDone = false;
                while (not isPromptDone) {
                    commandModeInput(commandBuffer);
                    if (commandBuffer == "yes" || commandBuffer == "y") {
                        bool isEnemyDied = false, isPlayerEngimonDied = false;
                        messageList.addMessage("");
                        switch (doBattle.getBattleWinner()) {
                            case 1:
                                messageList.addMessage(targetEngimon->getEngimonName() + " defeated");
                                isEnemyDied = true;
                                break;
                            case 2:
                                messageList.addMessage("Your " + player.getCurrentEngimon()->getEngimonName() + " defeated");
                                isPlayerEngimonDied = true;
                                break;
                            default:
                                isEnemyDied = true;
                                break;
                        }
                        // Branch if player engimon died
                        if (isPlayerEngimonDied) {
                            killCurrentEngimon();
                        }

                        // Branch if enemy died
                        if (isEnemyDied && not isPlayerEngimonDied) {
                            // Level up checking
                            int levelGained = player.getCurrentEngimon()->xpGain(targetEngimon->getLevel()*xpMultiplier);
                            if (levelGained > 0) {
                                spawnLevelCap += levelGained;
                            }

                            // Random mastery level up
                            vector<Skill>& SkillReference = player.getCurrentEngimon()->getSkillListRef();
                            for (unsigned i = 0; i < SkillReference.size(); i++) {
                                if ((rand() % 100) < 40)
                                    SkillReference[i].levelUpMastery();
                            }

                            // Random skill drop
                            int dropRoll = rand() % 100;
                            if ((unsigned) dropRoll < wildEngimonDropProbability) {
                                SkillItem droppedSkill = 1; // Placeholder
                                string droppedSkillName = "";
                                bool isSkillDropValid = false;
                                set<ElementType> targetEngimonElements = targetEngimon->getElements();
                                while (not isSkillDropValid) {
                                    // Try catch block for random skill picking
                                    try {
                                        Skill randomSkill = skillDB.getSkill(rand() % maxSkillID);
                                        if (randomSkill.isElementCompatible(*(targetEngimonElements.begin()))) {
                                            isSkillDropValid = true;
                                        }
                                        else if (++(targetEngimonElements.begin()) != targetEngimonElements.end()) {
                                            // If multi element, check other one
                                            if (randomSkill.isElementCompatible(*(++(targetEngimonElements.begin()))))
                                                isSkillDropValid = true;
                                        }

                                        if (isSkillDropValid) {
                                            droppedSkill = randomSkill.getSkillID();
                                            droppedSkillName = randomSkill.getSkillName();
                                        }
                                    }
                                    catch (int) {
                                        // Empty catch block
                                    }

                                }

                                messageList.addMessage(droppedSkillName + " dropped");
                                renderer.drawMessageBox(messageList);
                                // If inventory is full
                                if (not player.addSkillItem(droppedSkill)) {
                                    messageList.addMessage("Inventory is full, take? (y/n)");
                                    renderer.drawMessageBox(messageList);
                                    commandModeInput(commandBuffer);
                                    if (commandBuffer == "y") {
                                        if (deleteInventory()) {
                                            Skill targetSkill = skillDB.getSkill(droppedSkill);
                                            player.addSkillItem(droppedSkill);
                                            messageList.addMessage(targetSkill.getSkillName() + " added!");
                                        }
                                    }
                                }
                                else {
                                    Skill targetSkill = skillDB.getSkill(droppedSkill);
                                    messageList.addMessage(targetSkill.getSkillName() + " added!");
                                }
                            }

                            map.setTileEntity(targetEngimon->getPos(), NULL);
                            targetEngimon->tameEngimon();
                            // Catching prompt
                            bool validCatchCommand = false, catchEngimon = false;
                            while (not validCatchCommand) {
                                messageList.addMessage("Catch engimon? (y/n)");
                                renderer.drawMessageBox(messageList);
                                commandModeInput(commandBuffer);
                                if (commandBuffer == "y") {
                                    // If success adding item / inventory is not full
                                    if (player.addEngimonItem(targetEngimon)) {
                                        messageList.addMessage(targetEngimon->getEngimonName() + " catched!");
                                        catchEngimon = true;
                                        validCatchCommand = true;
                                    }
                                    else {
                                        // If failed to add and delete succeed
                                        if (deleteInventory()) {
                                            player.addEngimonItem(targetEngimon);
                                            messageList.addMessage(targetEngimon->getEngimonName() + " catched!");
                                            catchEngimon = true;
                                            validCatchCommand = true;
                                        }
                                    }
                                    // Remove from map and add to inventory
                                }
                                else if (commandBuffer == "n") {
                                    validCatchCommand = true;
                                }
                            }

                            if (not catchEngimon) {
                                delete targetEngimon;
                                targetEngimon = NULL;
                            }

                            updateCurrentEngimonMessageStatus();
                        }

                        // Maximum cummulative XP check
                        if (player.getCurrentEngimon()->isMaxCXP()) {
                            messageList.addMessage("Over cummulative XP");
                            killCurrentEngimon();
                        }

                        isPromptDone = true;
                    }
                    else if (commandBuffer == "no" || commandBuffer == "n")
                        isPromptDone = true;
                }
                battleRenderer.clearMessageBox(battleMessage);

                userInput.toggleReadInput();
                renderer.clearCursorRestArea();
            }
        }
    }

    return false;
}

void Engine::evaluteTick() {
    map.wildEngimonRandomMove();
    unsigned int randomNumber = rand() % 100;
    if (Entity::getEntityCount() < entitySpawnLimit && randomNumber < wildEngimonSpawnProbability) {
        unsigned int randomSpeciesID = (rand() % (speciesDB.getSpeciesCount() - 1)) + 1;
        // TODO : Extra, fix mod operator
        Engimon *spawnedEngimon = map.spawnWildEngimon(speciesDB.getSpecies(randomSpeciesID), spawnLevelCap);

        // Random skill adder, 3x roll chance to get additional skill
        // Higher level cap also grant higher chance to get multiple skill,
        // up to 100% chance getting 4 skill
        set<ElementType> engimonElements = spawnedEngimon->getElements();
        for (int i = 0; i < 3; i++) {
            if ((rand() % 100) < 10 + spawnLevelCap) {
                // Try catch block random skill
                Skill generatedSkill = skillDB.getSkill(1); // Placeholder
                bool isSkillGeneratedValid = false;
                while (not isSkillGeneratedValid) {
                    try {
                        generatedSkill = skillDB.getSkill(rand() % maxSkillID);
                        if (generatedSkill.isElementCompatible(*engimonElements.begin())) {
                            isSkillGeneratedValid = true;
                        }
                        else if (++engimonElements.begin() != engimonElements.end()) {
                            if (generatedSkill.isElementCompatible(*(++engimonElements.begin())))
                                isSkillGeneratedValid = true;
                        }

                        if (isSkillGeneratedValid) {
                            int masteryLevelUp = (rand() % (1 + spawnLevelCap/5)) + 1;
                            for (int j = 0; j < masteryLevelUp; j++)
                                generatedSkill.levelUpMastery();

                            spawnedEngimon->addSkill(generatedSkill);
                        }
                    }
                    catch (int) {

                    }
                }
            }
        }
        engimonList.push_back(spawnedEngimon);
    }
}

void Engine::commandMode() {
    // TODO : Extra, super-duper-repetition :(
    userInput.toggleReadInput();
    // Temporary stop input thread from queueing movement input
    clearConsoleInputBuffer();
    // Clearing current input buffer (GetKeyState() does not clear buffer)

    renderer.clearMessageBox(messageList);
    // Clearing message list window

    messageList.addMessage(" \xCD\xCD\xCD\xCD Command list \xCD\xCD\xCD\xCD ");
    messageList.addMessage("1. inventory   ");
    messageList.addMessage("2. engimon     ");
    messageList.addMessage("3. change      ");
    messageList.addMessage("4. item        ");
    messageList.addMessage("5. breed       ");
    messageList.addMessage("6. delete      ");
    messageList.addMessage("7. legend      ");
    messageList.addMessage("8. rename      ");


    renderer.drawMessageBox(messageList);

    string commandBuffer;
    commandModeInput(commandBuffer);

    renderer.clearMessageBox(messageList);
    // Clearing message list window

    if (commandBuffer == "dbg") { // DEBUG
        player.addEngimonItem(new Engimon(speciesDB.getSpecies(5), false, Position(0, 0), 50));
        player.addEngimonItem(new Engimon(speciesDB.getSpecies(6), false, Position(0, 0), 50));
        // player.addEngimonItem(new Engimon(speciesDB.getSpecies(2), false, Position(0, 0)));
        // player.addEngimonItem(new Engimon(speciesDB.getSpecies(rand()%10+1), false, Position(0, 0)));
        player.addSkillItem(5);
        player.addSkillItem(9);
        player.addSkillItem(11);
        player.addSkillItem(14);
        player.addSkillItem(15);
        player.addSkillItem(rand()%10+1);
    }
    else if (commandBuffer == "legend") {
        showLegendHelp();
    }
    else if (commandBuffer == "rename") {
        renameEngimon();
    }
    else if (commandBuffer == "delete") {
        deleteInventory();
    }
    else if (commandBuffer == "breed") {
        messageList.addMessage("Select Engimon Number");
        list<EngimonItem> engimonInv = player.getEngimonInventory();
        bool doneSelecting = false;
        while (not doneSelecting) {
            showEngimonInventory();
            messageList.addMessage("End of inventory list");
            messageList.addMessage("");
            messageList.addMessage("Input two engimon number ");
            messageList.addMessage("or exit ");

            string engimonStr1, engimonStr2;
            renderer.drawMessageBox(messageList);
            commandModeInput(engimonStr1);
            if (engimonStr1 == "exit")
                break;
            messageList.addMessage("Parent 1 is " + engimonStr1);
            renderer.drawMessageBox(messageList);
            commandModeInput(engimonStr2);
            if (engimonStr2 == "exit")
                break;
            messageList.addMessage("Parent 2 is " + engimonStr2);
            renderer.drawMessageBox(messageList);


            // Trying to parsing to int
            int targetNumber1, targetNumber2;
            bool successParsing = false;
            try {
                targetNumber1 = stoi(engimonStr1);
                targetNumber2 = stoi(engimonStr2);
                successParsing = true;
            }
            catch (invalid_argument e) {
                messageList.addMessage("Invalid input");
            }

            // If number are in valid range, then change
            if (successParsing && 0 < targetNumber1 && targetNumber1 <= (int) engimonInv.size() &&
                    0 < targetNumber2 && targetNumber2 <= (int) engimonInv.size() && targetNumber1 != targetNumber2) {
                auto it = engimonInv.begin();
                int i = 0;
                while (i < targetNumber1-1) {
                    i++;
                    ++it;
                }
                Engimon *targetEngimon1 = *it;

                it = engimonInv.begin();
                i = 0;
                while (i < targetNumber2-1) {
                    i++;
                    ++it;
                }
                Engimon *targetEngimon2 = *it;

                Engimon *newEngimon = NULL;
                bool breedSuccess = false;
                try {
                    Breed newBreed = Breed(targetEngimon1, targetEngimon2);
                    newEngimon = newBreed.startBreeding(speciesDB);
                    breedSuccess = true;
                }
                catch (int) {

                }

                if (breedSuccess) {
                    player.addEngimonItem(newEngimon);

                    string breedingString = "Breed success ";
                    breedingString = breedingString + newEngimon->getEngimonName();
                    messageList.addMessage("");
                    messageList.addMessage(breedingString);
                    messageList.addMessage("Use rename command");
                    messageList.addMessage("to change engimon name");
                    statRenderer.clearMessageBox(statMessage);
                    updateCurrentEngimonMessageStatus();
                    doneSelecting = true;
                }
                else {
                    renderer.clearMessageBox(messageList);
                    messageList.addMessage("Parent level is too low");
                    messageList.addMessage("");
                    renderer.drawMessageBox(messageList);
                }
            }
            else if (successParsing) {
                messageList.addMessage("Number is out of range");
            }
        }
    }
    else if (commandBuffer == "engimon") {
        list<EngimonItem> engimonInv = player.getEngimonInventory();
        int number = 1;
        for (auto it = engimonInv.begin(); it != engimonInv.end(); ++it) {
            Engimon *targetEngimon = *it;
            string speciesNameMsg = "Species \xB3 ";
            speciesNameMsg = speciesNameMsg + targetEngimon->getName();
            messageList.addMessage(speciesNameMsg);

            string nameMsg = "Name    \xB3 ";
            nameMsg = nameMsg + targetEngimon->getEngimonName();
            messageList.addMessage(nameMsg);

            string levelMsg = "Lvl     \xB3 ";
            levelMsg = levelMsg + to_string(targetEngimon->getLevel());
            messageList.addMessage(levelMsg);

            string xpMsg = "XP      \xB3 ";
            xpMsg = xpMsg + to_string(targetEngimon->getXP());
            messageList.addMessage(xpMsg);

            set<ElementType> elements = targetEngimon->getElements();
            string typeMsg = "Type    \xB3 ";
            if (elements.find(Fire) != elements.end())
                typeMsg = typeMsg + "Fire ";
            if (elements.find(Ice) != elements.end())
                typeMsg = typeMsg + "Ice ";
            if (elements.find(Water) != elements.end())
                typeMsg = typeMsg + "Water ";
            if (elements.find(Ground) != elements.end())
                typeMsg = typeMsg + "Ground ";
            if (elements.find(Electric) != elements.end())
                typeMsg = typeMsg + "Electric ";
            messageList.addMessage(typeMsg);

            messageList.addMessage(" \xCD\xCD\xCD\xCD Learned skill \xCD\xCD\xCD\xCD ");
            vector<Skill> skillList = targetEngimon->getSkillList();
            for (int i = 0; (unsigned) i < skillList.size(); i++) {
                string skillRow = skillList[i].getSkillName();
                skillRow = skillRow + " Lvl-" + to_string(skillList[i].getMasteryLevel());
                skillRow = skillRow + " Pow-" + to_string(skillList[i].getBasePower());
                messageList.addMessage(skillRow);
            }

            vector<Species> speciesList = targetEngimon->getParentSpecies();
            vector<string> parentName = targetEngimon->getParentNames();
            if (parentName.size() > 0) {
                messageList.addMessage(" \xCD\xCD\xCD\xCD     Parent     \xCD\xCD\xCD\xCD ");
                for (int i = 0; (unsigned) i < speciesList.size(); i++) {
                    string speciesRow = "Species ";
                    speciesRow = speciesRow + speciesList[i].getName();
                    messageList.addMessage(speciesRow);
                    speciesRow = "Name ";
                    speciesRow = speciesRow + parentName[i];
                    messageList.addMessage(speciesRow);
                    if (i == 0)
                        messageList.addMessage(" ");
                }
            }


            messageList.addMessage(" \xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD ");
            messageList.addMessage("");
            number++;
            if (number > 1) {
                messageList.addMessage("Press enter to print next");
                renderer.drawMessageBox(messageList);
                commandModeInput(commandBuffer);
                messageList.addMessage("");
            }
        }
        messageList.addMessage("End of inventory");
        renderer.drawMessageBox(messageList);
    }
    else if (commandBuffer == "inventory") {
        showItemInventory();
    }
    else if (commandBuffer == "change") {
        changeCurrentEngimon();
    }
    else if (commandBuffer == "item") {
        showItemInventory();
        std::map<SkillItem,int> skillInv = player.getSkillInventory();
        messageList.addMessage("");
        messageList.addMessage("Input item ID");

        renderer.drawMessageBox(messageList);
        commandModeInput(commandBuffer);

        // Trying to parsing to int
        int targetNumber;
        bool successParsing = false;
        try {
            targetNumber = stoi(commandBuffer);
            successParsing = true;
        }
        catch (invalid_argument e) {
            messageList.addMessage("Invalid input");
        }

        // If number are in valid range, then process
        if (successParsing && 0 < targetNumber && targetNumber <= (int) maxSkillID) {
            if (skillInv[targetNumber] > 0) {
                int skillIDTarget = targetNumber;
                list<EngimonItem> engimonInv = player.getEngimonInventory();
                showEngimonInventory();

                messageList.addMessage("Input engimon number");
                renderer.drawMessageBox(messageList);
                commandModeInput(commandBuffer);

                // Trying to parsing to int
                targetNumber = 0;
                successParsing = false;
                try {
                    targetNumber = stoi(commandBuffer);
                    successParsing = true;
                }
                catch (invalid_argument e) {
                    messageList.addMessage("Invalid input");
                }

                // If number are in valid range, then change
                if (successParsing && 0 < targetNumber && targetNumber <= (int) engimonInv.size()) {
                    auto it = engimonInv.begin();
                    int i = 0;
                    while (i < targetNumber-1) {
                        i++;
                        ++it;
                    }
                    Engimon *targetEngimon = *it;
                    if (skillDB.isCompatible(*targetEngimon, skillIDTarget)) {
                        Skill targetSkill = skillDB.getSkill(skillIDTarget);
                        string learningString = targetEngimon->getEngimonName();
                        learningString = learningString + " learned ";
                        learningString = learningString + targetSkill.getSkillName();

                        targetSkill.levelUpMastery();
                        if (targetEngimon->getLearnedSkillCount() < 4) {
                            if (targetEngimon->addSkill(targetSkill)) {
                                messageList.addMessage("");
                                messageList.addMessage(learningString);
                                player.delSkillItem(skillIDTarget);
                            }
                            else
                                messageList.addMessage("Skill already learned");
                        }
                        else {
                            messageList.addMessage("Skill slot is full");
                            messageList.addMessage("Forget skill ? (y/n)");
                            renderer.drawMessageBox(messageList);
                            commandModeInput(commandBuffer);
                            if (commandBuffer == "y") {
                                if (engimonForgetSkill(targetEngimon)) {
                                    // If success removing skill, learn skill
                                    targetEngimon->addSkill(targetSkill);
                                    messageList.addMessage("");
                                    messageList.addMessage(learningString);
                                    player.delSkillItem(skillIDTarget);
                                }
                            }
                        }
                        updateCurrentEngimonMessageStatus();
                        messageList.addMessage("");
                        messageList.addMessage("");
                        messageList.addMessage("");
                    }
                    else {
                        messageList.addMessage("Skill not compatible");
                    }
                }
                else if (successParsing) {
                    messageList.addMessage("Number is out of range");
                }

            }
            else
                messageList.addMessage("Item not found");
        }
        else if (successParsing) {
            messageList.addMessage("Number is out of range");
        }
    }


    userInput.toggleReadInput();
    renderer.clearCursorRestArea();
    isCommandMode = false;
}

void Engine::updateCurrentEngimonMessageStatus() {
    statMessage.clearMessage();
    string speciesNameMsg = "Species \xB3 ";
    speciesNameMsg = speciesNameMsg + player.getCurrentEngimon()->getName();
    statMessage.addMessage(speciesNameMsg);

    string nameMsg = "Name    \xB3 ";
    nameMsg = nameMsg + player.getCurrentEngimon()->getEngimonName();
    statMessage.addMessage(nameMsg);

    string levelMsg = "Lvl     \xB3 ";
    levelMsg = levelMsg + to_string(player.getCurrentEngimon()->getLevel());
    statMessage.addMessage(levelMsg);

    string xpMsg = "XP      \xB3 ";
    xpMsg = xpMsg + to_string(player.getCurrentEngimon()->getXP());
    statMessage.addMessage(xpMsg);

    set<ElementType> elements = player.getCurrentEngimon()->getElements();
    string typeMsg = "Type    \xB3 ";
    if (elements.find(Fire) != elements.end())
        typeMsg = typeMsg + "Fire ";
    if (elements.find(Ice) != elements.end())
        typeMsg = typeMsg + "Ice ";
    if (elements.find(Water) != elements.end())
        typeMsg = typeMsg + "Water ";
    if (elements.find(Ground) != elements.end())
        typeMsg = typeMsg + "Ground ";
    if (elements.find(Electric) != elements.end())
        typeMsg = typeMsg + "Electric ";
    statMessage.addMessage(typeMsg);

    statMessage.addMessage(" \xCD\xCD\xCD\xCD Learned skill \xCD\xCD\xCD\xCD ");
    vector<Skill> skillList = player.getCurrentEngimon()->getSkillList();
    for (int i = 0; (unsigned) i < skillList.size(); i++) {
        string skillRow = skillList[i].getSkillName();
        skillRow = skillRow + " Lvl-" + to_string(skillList[i].getMasteryLevel());
        skillRow = skillRow + " Pow-" + to_string(skillList[i].getBasePower());
        statMessage.addMessage(skillRow);
    }

    statRenderer.drawMessageBox(statMessage);
}

void Engine::showItemInventory() {
    // TODO : Extra, maybe need to enter to more ?
    messageList.addMessage("Inventory list");
    messageList.addMessage("ID Name       Count Type Power");
    std::map<SkillItem,int> skillInv = player.getSkillInventory();
    for (int i = 0; i < maxSkillID; i++) {
        if (skillInv[i] > 0) {
            string skillRow;
            Skill target = skillDB.getSkill(i);
            string paddedID = to_string(target.getSkillID());
            for (int i = paddedID.length(); i < 2; i++)
                paddedID = paddedID + " ";
            string paddedSkillName = target.getSkillName();
            for (int i = paddedSkillName.length(); i < 12; i++)
                paddedSkillName = paddedSkillName + " ";
            skillRow = paddedID + " " + paddedSkillName + " " + to_string(skillInv[i]) + " ";
            switch (target.getSkillElement()) {
                case Fire:
                    skillRow = skillRow + "Fire     ";
                    break;
                case Ice:
                    skillRow = skillRow + "Ice      ";
                    break;
                case Water:
                    skillRow = skillRow + "Water    ";
                    break;
                case Ground:
                    skillRow = skillRow + "Ground   ";
                    break;
                case Electric:
                    skillRow = skillRow + "Electric ";
                    break;
            }
            skillRow = skillRow + to_string(target.getBasePower());
            messageList.addMessage(skillRow);
        }
    }
}

void Engine::showEngimonInventory() {
    string commandBuffer;
    list<EngimonItem> engimonInv = player.getEngimonInventory();
    int number = 1;
    for (auto it = engimonInv.begin(); it != engimonInv.end(); ++it) {
        Engimon *targetEngimon = *it;
        string numberMsg = "\xCD\xCD\xCD\xCD\xCD\xCD\xCD";
        numberMsg = numberMsg + "  " + to_string(number) + "  " + numberMsg;
        messageList.addMessage(numberMsg);
        number++;

        string nameMsg = "Name    \xB3 ";
        nameMsg = nameMsg + targetEngimon->getEngimonName();
        messageList.addMessage(nameMsg);

        string levelMsg = "Lvl     \xB3 ";
        levelMsg = levelMsg + to_string(targetEngimon->getLevel());
        messageList.addMessage(levelMsg);

        set<ElementType> elements = targetEngimon->getElements();
        string typeMsg = "Type    \xB3 ";
        if (elements.find(Fire) != elements.end())
            typeMsg = typeMsg + "Fire ";
        if (elements.find(Ice) != elements.end())
            typeMsg = typeMsg + "Ice ";
        if (elements.find(Water) != elements.end())
            typeMsg = typeMsg + "Water ";
        if (elements.find(Ground) != elements.end())
            typeMsg = typeMsg + "Ground ";
        if (elements.find(Electric) != elements.end())
            typeMsg = typeMsg + "Electric ";
        messageList.addMessage(typeMsg);

        messageList.addMessage("");
        if ((number-1) % 3 == 0 && number > 1) {
            messageList.addMessage("Press enter to print next");
            renderer.drawMessageBox(messageList);
            commandModeInput(commandBuffer);
            messageList.addMessage("");
        }
    }
}

bool Engine::deleteInventory() {
    string commandBuffer;
    renderer.clearMessageBox(messageList);
    messageList.addMessage("Delete");
    messageList.addMessage("1. engimon");
    messageList.addMessage("2. item");
    messageList.addMessage("3. exit");
    renderer.drawMessageBox(messageList);
    commandModeInput(commandBuffer);
    renderer.clearMessageBox(messageList);
    if (commandBuffer == "engimon") {
        showEngimonInventory();
        messageList.addMessage("");
        messageList.addMessage("Select Engimon Number or exit");

        list<EngimonItem> engimonInv = player.getEngimonInventory();
        bool doneDeleting = false;
        while (not doneDeleting) {
            renderer.drawMessageBox(messageList);
            commandModeInput(commandBuffer);

            if (commandBuffer == "exit") {
                renderer.clearMessageBox(messageList);
                break;
            }

            // Trying to parsing to int
            int targetNumber;
            bool successParsing = false;
            try {
                targetNumber = stoi(commandBuffer);
                successParsing = true;
            }
            catch (invalid_argument e) {
                messageList.addMessage("Invalid input");
            }

            // If number are in valid range, then change
            if (successParsing && 0 < targetNumber && targetNumber <= (int) engimonInv.size()) {
                auto it = engimonInv.begin();
                int i = 0;
                while (i < targetNumber-1) {
                    i++;
                    ++it;
                }
                Engimon *targetEngimon = *it;

                // Checking whether selected engimon is not current engimon
                if (targetEngimon != player.getCurrentEngimon()) {
                    string deletingString = "Deleted ";
                    deletingString = deletingString + targetEngimon->getEngimonName();
                    messageList.addMessage("");
                    messageList.addMessage(deletingString);
                    player.delEngimonItem(targetEngimon);
                    doneDeleting = true;
                    delete targetEngimon;
                }
                else {
                    messageList.addMessage("Cannot delete current engimon");
                }
            }
            else if (successParsing) {
                messageList.addMessage("Number is out of range");
            }
        }
        if (doneDeleting) {
            return true;
            // Returning delete success
        }
    }
    else if (commandBuffer == "item") {
        showItemInventory();
        std::map<SkillItem,int> skillInv = player.getSkillInventory();
        messageList.addMessage("");
        messageList.addMessage("Input item ID");

        renderer.drawMessageBox(messageList);
        commandModeInput(commandBuffer);

        // Trying to parsing to int
        int targetNumber;
        bool successParsing = false;
        try {
            targetNumber = stoi(commandBuffer);
            successParsing = true;
        }
        catch (invalid_argument e) {
            messageList.addMessage("Invalid input");
        }

        // If number are in valid range, then process
        if (successParsing && 0 < targetNumber && targetNumber <= (int) maxSkillID) {
            if (skillInv[targetNumber] > 0) {
                renderer.clearMessageBox(messageList);
                Skill targetSkill = skillDB.getSkill(targetNumber);
                player.delSkillItem(targetNumber);
                string deletingString = targetSkill.getSkillName();
                deletingString = deletingString + " deleted";
                messageList.addMessage(deletingString);
            }
            else
                messageList.addMessage("Item not found");
        }
        else if (successParsing) {
            messageList.addMessage("Number is out of range");
        }
    }

    return false;
}

void Engine::changeCurrentEngimon() {
    string commandBuffer;
    messageList.addMessage("Select Engimon Number");
    list<EngimonItem> engimonInv = player.getEngimonInventory();
    showEngimonInventory();
    bool doneChanging = false;
    messageList.addMessage("End of inventory list");
    messageList.addMessage("");
    messageList.addMessage("Input engimon number or exit");
    while (not doneChanging) {
        renderer.drawMessageBox(messageList);
        commandModeInput(commandBuffer);

        if (commandBuffer == "exit")
            break;

        // Trying to parsing to int
        int targetNumber;
        bool successParsing = false;
        try {
            targetNumber = stoi(commandBuffer);
            successParsing = true;
        }
        catch (invalid_argument e) {
            messageList.addMessage("Invalid input");
        }

        // If number are in valid range, then change
        if (successParsing && 0 < targetNumber && targetNumber <= (int) engimonInv.size()) {
            auto it = engimonInv.begin();
            int i = 0;
            while (i < targetNumber-1) {
                i++;
                ++it;
            }
            Engimon *targetEngimon = *it;
            string changingString = "Changed to ";
            changingString = changingString + targetEngimon->getEngimonName();
            messageList.addMessage("");
            messageList.addMessage(changingString);
            map.setTileEntity(player.getCurrentEngimon()->getPos(), targetEngimon);
            player.changeEngimon(targetEngimon);
            statRenderer.clearMessageBox(statMessage);
            updateCurrentEngimonMessageStatus();
            doneChanging = true;
        }
        else if (successParsing) {
            messageList.addMessage("Number is out of range");
        }
    }
}

void Engine::loseScreen() {
    renderer.drawLoseScreen();
    isEngineRunning = false;
}

void Engine::killCurrentEngimon() {
    Engimon *currentEngimon = player.getCurrentEngimon();
    if (player.getEngimonInventoryCount() > 1) {
        // Doing force changing
        player.delEngimonItem(currentEngimon);
        messageList.addMessage("- Select another engimon -");
        messageList.addMessage("");
        renderer.clearCursorRestArea();
        renderer.drawMessageBox(messageList);
        changeCurrentEngimon();
        updateCurrentEngimonMessageStatus();
        delete currentEngimon;
    }
    else
        loseScreen();
}

void Engine::commandModeInput(string& target) {
    renderer.clearCursorRestArea();
    cout << ">>> ";
    getline(cin, target);
}

void Engine::showLegendHelp() {
    renderer.clearMessageBox(messageList);
    messageList.addMessage("        \xCD\xCD Legend \xCD\xCD ");
    messageList.addMessage("");
    messageList.addMessage("   W \xAF Water type");
    messageList.addMessage("   F \xAF Fire type");
    messageList.addMessage("   G \xAF Ground type");
    messageList.addMessage("   E \xAF Electric type");
    messageList.addMessage("   I \xAF Ice type");
    messageList.addMessage("   L \xAF Fire & Electric type");
    messageList.addMessage("   S \xAF Water & Ice type");
    messageList.addMessage("   N \xAF Water & Ground type");


    renderer.drawMessageBox(messageList);
}

void Engine::renameEngimon() {
    // TODO : Extra, generalize picking engimon, skill, item
    string commandBuffer;
    messageList.addMessage("Select Engimon Number");
    list<EngimonItem> engimonInv = player.getEngimonInventory();
    showEngimonInventory();
    bool doneChanging = false;
    messageList.addMessage("End of inventory list");
    messageList.addMessage("");
    messageList.addMessage("Input engimon number or exit");
    while (not doneChanging) {
        renderer.drawMessageBox(messageList);
        commandModeInput(commandBuffer);

        if (commandBuffer == "exit")
            break;

        // Trying to parsing to int
        int targetNumber;
        bool successParsing = false;
        try {
            targetNumber = stoi(commandBuffer);
            successParsing = true;
        }
        catch (invalid_argument e) {
            messageList.addMessage("Invalid input");
        }

        // If number are in valid range, then change
        if (successParsing && 0 < targetNumber && targetNumber <= (int) engimonInv.size()) {
            auto it = engimonInv.begin();
            int i = 0;
            while (i < targetNumber-1) {
                i++;
                ++it;
            }
            Engimon *targetEngimon = *it;

            messageList.addMessage("");
            messageList.addMessage("Type new name");
            renderer.drawMessageBox(messageList);
            commandModeInput(commandBuffer);
            targetEngimon->setEngimonName(commandBuffer);

            string changingString = "Changed to ";
            changingString = changingString + targetEngimon->getEngimonName();
            messageList.addMessage("");
            messageList.addMessage(changingString);
            updateCurrentEngimonMessageStatus();
            doneChanging = true;
        }
        else if (successParsing) {
            messageList.addMessage("Number is out of range");
        }
    }
}

bool Engine::engimonForgetSkill(Engimon *targetEngimon) {
    string commandBuffer;
    messageList.addMessage(" \xC4\xC4 Skill List \xC4\xC4 ");
    vector<Skill> skillListRef = targetEngimon->getSkillList();
    for (unsigned i = 0; i < skillListRef.size(); i++) {
        string skillID = to_string(skillListRef[i].getSkillID());
        messageList.addMessage(skillID + " " + skillListRef[i].getSkillName());
    }

    messageList.addMessage("");
    messageList.addMessage("Input skill ID");
    renderer.drawMessageBox(messageList);
    commandModeInput(commandBuffer);

    // Trying to parsing to int
    int targetNumber;
    bool successParsing = false;
    try {
        targetNumber = stoi(commandBuffer);
        successParsing = true;
    }
    catch (invalid_argument e) {
        messageList.addMessage("Invalid input");
    }

    bool isFound = false;
    if (successParsing && 0 < targetNumber && targetNumber < maxSkillID) {
        for (unsigned i = 0; i < skillListRef.size(); i++) {
            if (skillListRef[i].getSkillID() == targetNumber) {
                messageList.addMessage(skillListRef[i].getSkillName() + " removed");
                targetEngimon->deleteSkill(targetNumber);
                isFound = true;
                break;
            }
        }
        if (not isFound) {
            messageList.addMessage("Skill not found");
        }
    }
    else {
        messageList.addMessage("Number out of range");
    }
    statRenderer.clearMessageBox(statMessage);
    updateCurrentEngimonMessageStatus();

    return isFound;
}
