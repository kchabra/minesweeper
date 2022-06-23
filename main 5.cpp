#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

struct Tile {
    bool isFlagged;
    bool isMine;
    bool isRevealed;
    sf::FloatRect rect;
    int numNeighbors; // if !isMine

    Tile() : isMine(0), isFlagged(0), isRevealed(0), rect(0,0,0,0), numNeighbors(0) {}
};

struct Config {
    int rows,cols,numMines;
};

struct GameBoard {
    std::vector<Tile> tiles;
    sf::FloatRect parentRect;
    Config cfg;
    int mineCount;
    int flagCount;

    void flagAllMines() {
        for (auto& tile : tiles) {
            if (tile.isMine) {
                tile.isFlagged = true;
            } else {
                tile.isFlagged = false;
            }
        }
        flagCount = mineCount;
    }

    void updateParentRect(const sf::FloatRect &rect) {
        parentRect = rect;

        for (int y = 0; y < cfg.rows; ++y) {
            for (int x = 0; x < cfg.cols; ++x) {
                float tileX = rect.left + (static_cast<float>(x)/static_cast<float>(cfg.cols)) * rect.width;
                float tileY = rect.top + ((static_cast<float>(y)/static_cast<float>(cfg.rows))) * rect.height;
                float tileWidth = rect.width / static_cast<float>(cfg.cols);
                float tileHeight = rect.height / static_cast<float>(cfg.rows);

                Tile &tile = accessTile({x,y});
                tile.rect = sf::FloatRect(tileX,tileY,tileWidth,tileHeight);
            }
        }
    }

    void computeNeighbors()
    {
        for (int y = 0; y < cfg.rows; ++y) {
            for (int x = 0; x < cfg.cols; ++x) {
                Tile& tile = accessTile({x,y});
                if (!tile.isMine) {
                    auto neighborTiles = gatherFromCoords({
                                                                  {x-1,y-1},
                                                                  {x,  y-1},
                                                                  {x+1,y-1},
                                                                  {x-1,y  },
                                                                  {x+1,y  },
                                                                  {x-1,y+1},
                                                                  {x,  y+1},
                                                                  {x+1,y+1}
                                                          });

                    for (const Tile &neighbor : neighborTiles)
                        tile.numNeighbors += neighbor.isMine;
                }
            }
        }
    }

    GameBoard(sf::FloatRect rect, const Config &config) : parentRect(rect), cfg(config), mineCount(0), flagCount(0) {
        tiles.resize(cfg.cols*cfg.rows);
        generate();
    }

    void generate()
    {
        tiles.clear();
        tiles.resize(cfg.cols*cfg.rows);
        mineCount = 0;
        flagCount = 0;
        srand(time(NULL));

        std::vector<sf::Vector2i> minePositions;

        mineCount = cfg.numMines;
        for (int i = 0; i < mineCount; ++i) {
            bool alreadyExists = true;
            sf::Vector2i pos;

            while (alreadyExists) {
                alreadyExists = false;
                pos = sf::Vector2i(rand()%cfg.cols,rand()%cfg.rows);
                for (const auto& minePos: minePositions) {
                    if (pos == minePos) {
                        alreadyExists = true;
                        break;
                    }
                }
            }

            minePositions.push_back(pos);
        }

        // write default tiles
        for (int y = 0; y < cfg.rows; ++y) {
            for (int x = 0; x < cfg.cols; ++x) {
                float tileX = parentRect.left + static_cast<float>(x)/static_cast<float>(cfg.cols) * parentRect.width;
                float tileY = parentRect.top + static_cast<float>(y)/static_cast<float>(cfg.rows) * parentRect.height;
                float tileWidth = parentRect.width / static_cast<float>(cfg.cols);
                float tileHeight = parentRect.height / static_cast<float>(cfg.rows);

                Tile tile;
                tile.rect = sf::FloatRect(tileX,tileY,tileWidth,tileHeight);
                tile.isRevealed = false;
                tile.numNeighbors = 0;
                tile.isMine = false;

                tiles[y*cfg.cols+x] = tile;
            }
        }

        // write mines
        for (const auto& minePos: minePositions) {
            tiles[minePos.y*cfg.cols+minePos.x].isMine = true;
        }

        computeNeighbors();
    }

    Tile &accessTile(sf::Vector2i coords) {
        return tiles[coords.y*cfg.cols+coords.x];
    }

    bool coordsExist(sf::Vector2i coords) {
        return coords.x >= 0 && coords.x < cfg.cols && coords.y >= 0 && coords.y < cfg.rows;
    }

    void floodFill(sf::Vector2i coords) {
        Tile &currTile = accessTile(coords);
        if (!currTile.isRevealed && !currTile.isMine) {
            currTile.isRevealed = true;
            if (currTile.isFlagged) {
                flagCount--;
                currTile.isFlagged= false;
            }

            auto fillNeighbor = [&](int x, int y) {
                sf::Vector2i adjacentCoords = {x,y};
                if (coordsExist(adjacentCoords)) {
                    floodFill(adjacentCoords);
                }
            };

            if (currTile.numNeighbors == 0) {
                fillNeighbor(coords.x-1,coords.y-1);
                fillNeighbor(coords.x,coords.y-1);
                fillNeighbor(coords.x+1,coords.y-1);
                fillNeighbor(coords.x-1,coords.y);
                fillNeighbor(coords.x+1,coords.y);
                fillNeighbor(coords.x-1,coords.y+1);
                fillNeighbor(coords.x,coords.y+1);
                fillNeighbor(coords.x+1,coords.y+1);
            }
        }

    }

    std::vector<Tile> gatherFromCoords(const std::vector<sf::Vector2i> &positions) {
        std::vector<Tile> coordTiles;
        for (auto &pos : positions) {
            if (coordsExist(pos)) {
                coordTiles.push_back(accessTile(pos));
            }
        }
        return coordTiles;
    }

    void loadBoard(char board[]) {
        tiles.clear();
        mineCount = 0;
        flagCount = 0;
        tiles.resize(cfg.cols*cfg.rows);

        for (int y = 0; y < cfg.rows; ++y) {
            for (int x = 0; x < cfg.cols; ++x) {
                float tileX = parentRect.left + static_cast<float>(x)/static_cast<float>(cfg.cols) * parentRect.width;
                float tileY = parentRect.top + static_cast<float>(y)/static_cast<float>(cfg.rows) * parentRect.height;
                float tileWidth = parentRect.width / static_cast<float>(cfg.cols);
                float tileHeight = parentRect.height / static_cast<float>(cfg.rows);

                Tile tile;

                tile.rect = sf::FloatRect(tileX,tileY,tileWidth,tileHeight);
                tile.isRevealed = false;
                tile.numNeighbors = 0;

                long off1 = y*(cfg.cols+1)+x; // +1 for \n in string
                long off2 = y*cfg.cols+x;
                if (board[off1] == '1') {
                    tile.isMine = true;
                }  else if (board[off1] == '0') {
                    tile.isMine = false;
                }

                if (tile.isMine) ++mineCount;

                tiles[off2] = tile;
            }
        }
        computeNeighbors();
    }

    bool mouseOverTile(sf::Vector2i &tileCoords, const sf::Vector2f &mousePos) {
        for (int y = 0; y < cfg.rows; ++y) {
            for (int x = 0; x < cfg.cols; ++x) {
                Tile& tile = accessTile({x,y});
                if (tile.rect.contains( mousePos )) {
                    // this is the tile that was clicked, break out
                    tileCoords = {x,y};
                    return true;
                }
            }
        }

        return false;
    }

    // check winning state by making sure all unrevealed cells are mines
    bool areWeWinners() {
        for (const Tile &tile : tiles) {
            if (!tile.isRevealed && !tile.isMine) {
                return false;
            }
        }

        return true;
    }
};

struct Button {
    sf::FloatRect rect;
    sf::Sprite sprite;
    Button(const sf::FloatRect& r, const sf::Texture &texture) : rect(r) {
        sprite.setTexture(texture);
        const auto size = texture.getSize();
        sprite.setScale(1.0f/size.x * r.width, 1.0f/size.y * r.height);
        sprite.setPosition(r.left,r.top);
    }
};

char *loadFile(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file != nullptr) {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *data = new char[size +1]();
        fread(data,1,size,file);
        data[size]='\0';

        fclose(file);

        return data;
    } else {
        fprintf(stderr, "Failed to load file %s!\n", path);
    }

    return nullptr;
}


bool loadConfig(Config *config, const char *filename)
{
    FILE *fp = fopen(filename, "rb");

    if (fp != nullptr) {
        fscanf(fp,"%d",&config->cols);
        fscanf(fp,"%d",&config->rows);
        fscanf(fp,"%d",&config->numMines);

        fclose(fp);
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    sf::Texture mineTex;
    mineTex.loadFromFile("images/mine.png");
    sf::Texture flag_tex;
    flag_tex.loadFromFile("images/flag.png");
    sf::Texture debug_tex;
    debug_tex.loadFromFile("images/debug.png");
    sf::Texture digits_tex;
    digits_tex.loadFromFile("images/digits.png");
    sf::Texture face_happy_tex;
    face_happy_tex.loadFromFile("images/face_happy.png");
    sf::Texture face_win_tex;
    face_win_tex.loadFromFile("images/face_win.png");
    sf::Texture face_lose_tex;
    face_lose_tex.loadFromFile("images/face_lose.png");
    sf::Texture number1_tex;
    number1_tex.loadFromFile("images/number_1.png");
    sf::Texture number2_tex;
    number2_tex.loadFromFile("images/number_2.png");
    sf::Texture number3_tex;
    number3_tex.loadFromFile("images/number_3.png");
    sf::Texture number4_tex;
    number4_tex.loadFromFile("images/number_4.png");
    sf::Texture number5_tex;
    number5_tex.loadFromFile("images/number_5.png");
    sf::Texture number6_tex;
    number6_tex.loadFromFile("images/number_6.png");
    sf::Texture number7_tex;
    number7_tex.loadFromFile("images/number_7.png");
    sf::Texture number8_tex;
    number8_tex.loadFromFile("images/number_8.png");
    sf::Texture tile_hidden_tex;
    tile_hidden_tex.loadFromFile("images/tile_hidden.png");
    sf::Texture tile_revealed_tex;
    tile_revealed_tex.loadFromFile("images/tile_revealed.png");
    sf::Texture test1_tex;
    test1_tex.loadFromFile("images/test_1.png");
    sf::Texture test2_tex;
    test2_tex.loadFromFile("images/test_2.png");
    sf::Texture test3_tex;
    test3_tex.loadFromFile("images/test_3.png");

    const sf::Texture *numberTextures[8] = {
            &number1_tex,
            &number2_tex,
            &number3_tex,
            &number4_tex,
            &number5_tex,
            &number6_tex,
            &number7_tex,
            &number8_tex,
    };

    sf::RenderWindow window (sf::VideoMode(800,600), "Minesweeper", sf::Style::Titlebar | sf::Style::Close);

    window.setVerticalSyncEnabled(false); // call it once, after creating the window

    sf::View view = window.getDefaultView();

    sf::Vector2f viewSize = view.getSize();
    sf::FloatRect targetRect{
            0.0f,
            0.0f,
            viewSize.x,
            viewSize.y*.9f
    };


    Config config;
    loadConfig(&config, "boards/config.cfg");
    GameBoard gameBoard = GameBoard(targetRect, config);

    bool showAllMines = false;
    int gameOverState = 0; // 0 : not-done, 1 : failed , 2 : success

    while (window.isOpen()) {
        sf::Event event;
        bool lmbClicked = false, rmbClicked = false;

        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::Resized) {
                // resize my view
                view.setSize({
                                     static_cast<float>(event.size.width),
                                     static_cast<float>(event.size.height)
                             });
                window.setView(view);
                // and align shape
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    lmbClicked = true;
                }

                if (event.mouseButton.button == sf::Mouse::Right) {
                    rmbClicked = true;
                }
            }
        }

        viewSize = view.getSize();
        targetRect = {
                0.0f,
                0.0f,
                viewSize.x,
                viewSize.y*.9f
        };
        gameBoard.updateParentRect(targetRect);

        const float buttonLength = targetRect.height*.1f;

        auto smilyTexSize = face_happy_tex.getSize();
        sf::FloatRect smilyButtonRect = {targetRect.width/2-buttonLength/2, targetRect.height,
                                         buttonLength, buttonLength};

        Button debugBtn ({targetRect.width-buttonLength*4,targetRect.height,buttonLength, buttonLength},
                         debug_tex);

        Button test1Btn({targetRect.width-buttonLength*3,targetRect.height,buttonLength,buttonLength},
                        test1_tex);

        Button test2Btn({targetRect.width-buttonLength*2,targetRect.height,buttonLength,buttonLength},
                        test2_tex);

        Button test3Btn({targetRect.width-+buttonLength*1,targetRect.height,buttonLength,buttonLength},
                        test3_tex);

        // update board
        if (!gameOverState) {
            if (lmbClicked) {
                // left click...
                sf::Vector2i tileCoords;

                if (gameBoard.mouseOverTile(tileCoords, sf::Vector2f(sf::Mouse::getPosition(window)))) {
                    Tile& tile  = gameBoard.accessTile(tileCoords);
                    if (!tile.isFlagged) {
                        if (tile.isMine) {
                            // game over, failed!
                            gameOverState = 1;
                        } else {
                            if (tile.numNeighbors) {
                                tile.isRevealed = true;
                            } else
                                gameBoard.floodFill(tileCoords);
                        }
                    }

                    if (gameBoard.areWeWinners()) {
                        gameBoard.flagAllMines();
                        gameOverState = 2;// won!
                        showAllMines = false;
                    }
                }
            }

            if (rmbClicked) {
                // right click ... place flag

                sf::Vector2i tileCoords;
                if (gameBoard.mouseOverTile(tileCoords, sf::Vector2f(sf::Mouse::getPosition(window)))) {
                    Tile& tile  = gameBoard.accessTile(tileCoords);
                    if (!tile.isRevealed) {
                        if (tile.isFlagged) {
                            gameBoard.flagCount--;
                        } else {
                            gameBoard.flagCount++;
                        }

                        tile.isFlagged = !tile.isFlagged;
                    }
                }
            }

            if (lmbClicked) {
                if (debugBtn.rect.contains(sf::Vector2f(sf::Mouse::getPosition(window)))) {
                    showAllMines = !showAllMines;
                }
            }
        }

        // update game
        if (lmbClicked) {
            sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));
            if (smilyButtonRect.contains(mousePos)) {
                // reset game by clicking on smily
                gameOverState = 0;
                gameBoard.generate();
            }

            if (test1Btn.rect.contains(mousePos)) {
                char *board = loadFile("boards/testboard1.brd");
                if (board != nullptr) {
                    gameOverState = 0;
                    gameBoard.loadBoard(board);
                }
                delete[] board;
            }

            if (test2Btn.rect.contains(mousePos)) {
                char *board = loadFile("boards/testboard2.brd");
                if (board != nullptr) {
                    gameOverState = 0;
                    gameBoard.loadBoard(board);
                }
                delete[] board;
            }

            if (test3Btn.rect.contains(mousePos)) {
                char *board = loadFile("boards/testboard3.brd");
                if (board != nullptr) {
                    gameOverState = 0;
                    gameBoard.loadBoard(board);
                }

                delete[] board;
            }
        }

        // clear to black color
        window.clear(sf::Color::Black);

        // draw game here
        for (const Tile& tile : gameBoard.tiles) {
            sf::Sprite sprite;

            {
                // draw grid cells whether revealed or not
                const sf::Texture *tex = tile.isRevealed ? &tile_revealed_tex : &tile_hidden_tex;
                sprite.setTexture(*tex);
                auto size = tex->getSize();
                sprite.setPosition(tile.rect.left, tile.rect.top);
                sprite.setScale(1.0f/size.x * tile.rect.width, 1.0f/size.y * tile.rect.height);
                window.draw(sprite);
            }

            if (tile.isMine && (gameOverState == 1 || showAllMines)) {
                // mine hit, just display mine
                sprite.setTexture(mineTex);
                auto size = mineTex.getSize();
                sprite.setPosition(tile.rect.left, tile.rect.top);
                sprite.setScale(1.0f/size.x * tile.rect.width, 1.0f/size.y * tile.rect.height);
                window.draw(sprite);
            }

            if (tile.isRevealed && tile.numNeighbors > 0) {
                const sf::Texture* num_tex = numberTextures[tile.numNeighbors - 1];

                sprite.setTexture(*num_tex);
                auto size = num_tex->getSize();
                sprite.setPosition(tile.rect.left, tile.rect.top);
                sprite.setScale(1.0f/size.x * tile.rect.width, 1.0f/size.y * tile.rect.height);
                window.draw(sprite);
            }

            if (tile.isFlagged) {
                // render flag
                sprite.setTexture(flag_tex);
                auto size = flag_tex.getSize();
                sprite.setPosition(tile.rect.left, tile.rect.top);
                sprite.setScale(1.0f/size.x * tile.rect.width, 1.0f/size.y * tile.rect.height);
                window.draw(sprite);
            }

        }

        {
            // draw smily at bottom of screen
            sf::Sprite sprite;
            sf::Texture *faceList[3] = {&face_happy_tex,&face_lose_tex,&face_win_tex};
            sf::Texture *expressionTex = faceList[gameOverState];

            sprite.setTexture(*expressionTex);
            sprite.setPosition(smilyButtonRect.left, smilyButtonRect.top);
            sprite.setScale(1.0f/smilyTexSize.x *smilyButtonRect.width, 1.0f/smilyTexSize.x *smilyButtonRect.height);
            window.draw(sprite);
        }

        {
            // draw debug at bottom of screen
            window.draw(debugBtn.sprite);
        }

        {
            // draw test 1 button
            window.draw(test1Btn.sprite);
            window.draw(test2Btn.sprite);
            window.draw(test3Btn.sprite);
        }

        {
            // draw mine counter
            sf::Sprite sprite;
            sprite.setTexture(digits_tex);

            int value = gameBoard.mineCount - gameBoard.flagCount;

            std::string valueString = std::to_string(value);
            for (int i = 0; i < valueString.size(); ++i) {
                char ch = valueString[i];
                int texOff = ch >= '0' && ch <= '9' ? ch-'0' : 10;

                sprite.setTextureRect({ texOff * 21,0,21,32});
                sprite.setPosition(i * 21, viewSize.y - buttonLength);
                window.draw(sprite);
            }
        }

        // end the current frame
        window.display();
    }

    return 0;
}