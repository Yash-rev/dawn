#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

const int MAP_WIDTH = 200;
const int MAP_HEIGHT = 200;
const float TILE_SIZE = 64.f;

 sf::Music bgm; 
    
    sf::SoundBuffer shootBuffer; 
    sf::SoundBuffer killBuffer;
    
    std::optional<sf::Sound> shootSound;        
    std::optional<sf::Sound> killSound;

enum class State
{
    MENU,
    PLAYING,
    GAMEOVER
};
State currentState = State::MENU;

struct Bullet {
    Bullet() = default; 

    sf::CircleShape shape;
    std::optional<sf::Sprite> sprite; 
    bool isSprite = false;            

    sf::Vector2f velocity;
    float speed;

    Bullet(sf::Vector2f startPosition, sf::Vector2f direction) {
        shape.setRadius(5.f);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({5.f, 5.f});
        shape.setPosition(startPosition);
        velocity = direction;
        speed = 800.f;
        isSprite = false; 
    }

    
    void update(float dt) {
        if (isSprite && sprite) {
            sprite->move(velocity * speed * dt); 
        } else {
            shape.move(velocity * speed * dt);   
        }
    }
};

struct Boss {
    std::optional<sf::Sprite> sprite;
    
    int currentFrame = 0;
    int maxFrames = 4;
    int frameWidth = 79;  
    int frameHeight = 71; 
    float animTimer = 0.f;
    float animDuration = 0.15f;
    float hp = 100.f;
    float maxHp = 2000.f;
    float speed = 100.f;
    float shootTimer = 0.f;
    float shootCooldown = 1.5f;
    bool isActive = false;
    bool isDefeated = false;
};


std::vector<Bullet> activeBullets; 
std::vector<Bullet> enemyBullets;  
Boss boss;                        

sf::RectangleShape bossHealthBarBG; 
sf::RectangleShape bossHealthBarFG; 

struct Enemy
{
    sf::Sprite sprite;
    float speed;

    Enemy(sf::Vector2f startPosition, const sf::Texture &enemyTexture) : sprite(enemyTexture)
    {
        sprite.setTexture(enemyTexture);

        sf::Vector2u size = enemyTexture.getSize();
        sprite.setOrigin({static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f});

        sprite.setPosition(startPosition);

        sprite.setScale({1.0f, 1.0f});

        speed = 150.f;
    }

    void update(float dt, sf::Vector2f playerPos, const std::vector<std::vector<int>>& dungeonMap) {
        sf::Vector2f currentPos = sprite.getPosition();
        
      
        float dx = playerPos.x - currentPos.x;
        float dy = playerPos.y - currentPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (dx > 0.f) {
            
            sprite.setScale({-1.f, 1.f}); 
        } 
        else if (dx < 0.f) {
           
            sprite.setScale({1.f, 1.f}); 
        }
        
        if (distance > 0) {
            float moveX = (dx / distance) * speed * dt;
            float moveY = (dy / distance) * speed * dt;
            
            //  Wall Sliding
            float nextX = currentPos.x + moveX;
            int gridX = static_cast<int>(nextX / TILE_SIZE);
            int gridY = static_cast<int>(currentPos.y / TILE_SIZE); 
            
            if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
                if (dungeonMap[gridX][gridY] == 0) {
                    sprite.move({moveX, 0.f});
                }
            }

            currentPos = sprite.getPosition(); 
            float nextY = currentPos.y + moveY;
            gridX = static_cast<int>(currentPos.x / TILE_SIZE); 
            gridY = static_cast<int>(nextY / TILE_SIZE);
            
            if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
                if (dungeonMap[gridX][gridY] == 0) {
                    sprite.move({0.f, moveY});
                }
            }
        }
    }
};
class Player
{
public:
    sf::Texture idleTexture; 
    sf::Texture runTexture;
    sf::Sprite sprite;
    sf::Texture gunTexture;
    sf::Sprite gunSprite;
    sf::Vector2f idleScale = {0.1f, 0.1f};
    sf::Vector2f runScale = {0.14f, 0.14f};
    float speed = 300.f;
    int hp = 3;
    int maxHp = 3;
    bool isDead = false;
    float iFrameTimer = 0.f;
    float iFrameDuration = 1.0f;
    float fireRate = 0.15f;
    float fireCooldown = 0.f;

    int frameWidth;
    int frameHeight;
    int currentFrame = 0;
    int maxFrames;
    float animTimer = 0.f;
    float animDuration = 0.15f; 
    bool isMoving = false;
    bool isFacingRight = true;

   Player(float startX, float startY, int fWidth, int fHeight, int frames) 
        : sprite(idleTexture), gunSprite(gunTexture) 
    {
        frameWidth = fWidth;
        frameHeight = fHeight;
        maxFrames = frames;

        
        if (!idleTexture.loadFromFile("player.png")) {
            std::printf("ERROR: Could not load player.png\n");
        }
        idleTexture.setSmooth(false);

       
        if (!runTexture.loadFromFile("player_running.png")) {
            std::printf("ERROR: Could not load player_run.png\n");
        }
        runTexture.setSmooth(false);

        sprite.setTexture(idleTexture, true); 
        
        sf::Vector2u idleSize = idleTexture.getSize();
        
        sprite.setOrigin({static_cast<float>(idleSize.x) / 2.f, static_cast<float>(idleSize.y) / 2.f});
        sprite.setScale({0.1f, 0.1f}); 
        sprite.setPosition({startX, startY});

        // GUN 
        if (!gunTexture.loadFromFile("gun.png")) {
            std::printf("ERROR: Could not load gun.png\n");
        }
        gunTexture.setSmooth(false);
        
     
        gunSprite.setTexture(gunTexture, true);

        sf::Vector2u gunSize = gunTexture.getSize();
        gunSprite.setOrigin({-15.f, static_cast<float>(gunSize.y) / 2.f});
        gunSprite.setScale({0.05f, 0.05f});
        gunSprite.setPosition({startX, startY});
    }

    
    void update(float dt, const sf::RenderWindow &window, const sf::View &worldView, const std::vector<std::vector<int>> &dungeonMap)
    {

        if (iFrameTimer > 0.f)
        {
            iFrameTimer -= dt;
            sprite.setColor(sf::Color::Red); 
        }
        else
        {
            sprite.setColor(sf::Color::White);
        }

        sf::Vector2f movement({0.f, 0.f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) movement.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) movement.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) movement.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) movement.x += 1.f;

        isMoving = (movement.x != 0.f || movement.y != 0.f);

        if (isMoving)
        {
            float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            movement.x /= length;
            movement.y /= length;
        }

        // MAP COLLISION
        sf::Vector2f currentPos = sprite.getPosition();

        
        if (movement.x != 0.f)
        {
            float nextX = currentPos.x + (movement.x * speed * dt);
            int gridX = static_cast<int>(nextX / TILE_SIZE);
            int gridY = static_cast<int>(currentPos.y / TILE_SIZE);

            if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
                if (dungeonMap[gridX][gridY] == 0) {
                    sprite.move({movement.x * speed * dt, 0.f});
                }
            }
        }

    
        currentPos = sprite.getPosition();
        if (movement.y != 0.f)
        {
            float nextY = currentPos.y + (movement.y * speed * dt);
            int gridX = static_cast<int>(currentPos.x / TILE_SIZE);
            int gridY = static_cast<int>(nextY / TILE_SIZE);

            if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
                if (dungeonMap[gridX][gridY] == 0) {
                    sprite.move({0.f, movement.y * speed * dt});
                }
            }
        }

        // ANIMATION 
        
        if (movement.x > 0.f) isFacingRight = true;
        if (movement.x < 0.f) isFacingRight = false;

        if (isMoving) 
        {

            if (&sprite.getTexture() != &runTexture) {
                sprite.setTexture(runTexture);
                sprite.setOrigin({static_cast<float>(frameWidth) / 2.f, static_cast<float>(frameHeight) / 2.f});
            }

            
            sprite.setScale({isFacingRight ? runScale.x : -runScale.x, runScale.y});

            
            animTimer += dt;
            if (animTimer >= animDuration) {
                currentFrame++;
                if (currentFrame >= maxFrames) {
                    currentFrame = 0;
                }
                animTimer = 0.f;
            }
            
            
            sprite.setTextureRect(sf::IntRect({currentFrame * frameWidth, 0}, {frameWidth, frameHeight}));
        } 
        else 
        {
            
            if (&sprite.getTexture() != &idleTexture) {
                sprite.setTexture(idleTexture);
                
                sf::Vector2u idleSize = idleTexture.getSize();
                sprite.setTextureRect(sf::IntRect({0, 0}, {static_cast<int>(idleSize.x), static_cast<int>(idleSize.y)}));
                sprite.setOrigin({static_cast<float>(idleSize.x) / 2.f, static_cast<float>(idleSize.y) / 2.f});
                
                currentFrame = 0; 
                animTimer = 0.f;
            }

            
            sprite.setScale({isFacingRight ? idleScale.x : -idleScale.x, idleScale.y});
        }

        
        
        float currentHandOffsetX = isFacingRight ? 12.f : -12.f;
        sf::Vector2f handOffset(currentHandOffsetX, 10.f);
        gunSprite.setPosition(sprite.getPosition() + handOffset);


        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixelPos, worldView);
        float dx = mousePos.x - sprite.getPosition().x;
        float dy = mousePos.y - sprite.getPosition().y;
        float angle = std::atan2(dy, dx) * 180.f / 3.14159265f;
        gunSprite.setRotation(sf::degrees(angle));

        
        if (dx < 0.f) 
        {
            
            gunSprite.setScale({0.05f, -0.05f});
        } 
        else 
        {

            gunSprite.setScale({0.05f, 0.05f});
        }
        // SHOOTING 
        fireCooldown -= dt;
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fireCooldown <= 0.f)
        {
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance != 0)
            {
                sf::Vector2f fireDirection({dx / distance, dy / distance});
                sf::Vector2f spawnPos = sprite.getPosition() + (fireDirection * 45.f);
                
                activeBullets.emplace_back(spawnPos, fireDirection);
                std::cout << "Bullet Fired! Total in list: " << activeBullets.size() << std::endl;
                fireCooldown = fireRate;
            }
            shootSound->play();
            
        }
    }

    void takeDamage()
    {
        if (iFrameTimer <= 0.f && !isDead)
        {
            hp -= 1;
            iFrameTimer = iFrameDuration;
            if (hp <= 0)
            {
                isDead = true;
                sprite.setColor(sf::Color(100, 100, 100));
            }
        }
    }

    void draw(sf::RenderWindow &window)
    {
        window.draw(sprite);
        window.draw(gunSprite);
    }
};

sf::View getLetterboxView(sf::View view, int windowWidth, int windowHeight) {

    float windowRatio = windowWidth / static_cast<float>(windowHeight);
    float viewRatio = view.getSize().x / static_cast<float>(view.getSize().y);

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    
    if (windowRatio >= viewRatio) {
    
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) / 2.0f;
    } else {
    
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) / 2.0f;
    }

    
    view.setViewport(sf::FloatRect({posX, posY}, {sizeX, sizeY}));

    return view;
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Dusk");

    sf::Font mainFont;
    
  
    if (!mainFont.openFromFile("arial.ttf")) {
    }

    sf::Text loadingText(mainFont); 
    loadingText.setString("LOADING...");
    loadingText.setCharacterSize(60);
    loadingText.setFillColor(sf::Color::White);

    loadingText.setPosition({window.getSize().x / 2.0f - 150.f, window.getSize().y / 2.0f - 50.f});
    

    
    if (!shootBuffer.loadFromFile("shootSound.nmp3")) { 
        

    }
   
   


    if (!bgm.openFromFile("bgm.mp3")) { 
        
    }
    bgm.setLooping(true); 

   if (!shootBuffer.loadFromFile("shootSound.mp3")) { std::printf("ERROR: Could not find shootSound.mp3\n");  }
    
    
    shootSound.emplace(shootBuffer);
    shootSound->setVolume(70.f); 

    
    if (!killBuffer.loadFromFile("killSound.mp3")) { std::printf("ERROR: Could not find killSound.mp3\n"); }
    
    killSound.emplace(killBuffer);
    killSound->setVolume(80.f);

    float spawnX = (MAP_WIDTH * TILE_SIZE) / 2.f;
    float spawnY = (MAP_HEIGHT * TILE_SIZE) / 2.f;
    
    Player player(spawnX, spawnY, 500, 512, 4);
    sf::Font font;
    bool hasFont = font.openFromFile("arial.ttf");
    if (!hasFont){
        std::printf("ERROR: Could not find arial.ttf!\n");
    }
    // Menu Text
    sf::Text menuText(font);
    menuText.setString("DUSK\nPress 'S' to Start\nPress 'Q' to Quit");
    menuText.setCharacterSize(50);
    menuText.setFillColor(sf::Color::Cyan);
    menuText.setPosition({150.f, 200.f});

    // Game Over Text
    sf::Text gameOverText(font);
    gameOverText.setString("GAME OVER\nPress 'R' to Retry\nPress 'H' for Home");
    gameOverText.setCharacterSize(50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition({150.f, 200.f});

    sf::Text scoreText(font);
    if (hasFont)
    {
        scoreText.setCharacterSize(30);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setOutlineColor(sf::Color::Black);
        scoreText.setOutlineThickness(2.0f);
        scoreText.setPosition({20.f, 60.f});
        scoreText.setString("Kills: 0");
    }

   sf::Text bossNameText(font); 
    if (hasFont) {
        bossNameText.setString("THE HELLBAT"); 
        bossNameText.setCharacterSize(20);      
        bossNameText.setFillColor(sf::Color::Red);
        
        bossNameText.setOutlineColor(sf::Color::Black);
        bossNameText.setOutlineThickness(2.f); 
    }

    float victoryTimer = 0.f; 
    float victoryDuration = 4.f; 
    
    // The "Dull Screen" Overlay
    sf::RectangleShape dimOverlay({800.f, 600.f}); 
    dimOverlay.setFillColor(sf::Color(0, 0, 0, 150)); 
    
    sf::Text victoryText(font);
    if (hasFont) {
        victoryText.setString("CONGRATULATIONS! \n You have defeated the HELLBAT!!");
        victoryText.setCharacterSize(30);
        victoryText.setFillColor(sf::Color::Red);
        victoryText.setOutlineColor(sf::Color::Black);
        victoryText.setOutlineThickness(3.f);
        
       
        sf::FloatRect bounds = victoryText.getLocalBounds();
        victoryText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        victoryText.setPosition({400.f, 300.f}); 
    }

    // GRID
    sf::VertexArray grid(sf::PrimitiveType::Lines);
    sf::Color gridColor(30, 30, 30);
    for (int x = -5000; x <= 5000; x += 100)
    {
        sf::Vertex v1;
        v1.position = {static_cast<float>(x), -5000.f};
        v1.color = gridColor;
        grid.append(v1);
        sf::Vertex v2;
        v2.position = {static_cast<float>(x), 5000.f};
        v2.color = gridColor;
        grid.append(v2);
    }
    for (int y = -5000; y <= 5000; y += 100)
    {
        sf::Vertex v1;
        v1.position = {-5000.f, static_cast<float>(y)};
        v1.color = gridColor;
        grid.append(v1);
        sf::Vertex v2;
        v2.position = {5000.f, static_cast<float>(y)};
        v2.color = gridColor;
        grid.append(v2);
    }

    sf::View worldView(sf::Vector2f(400.f, 300.f), sf::Vector2f(800.f, 600.f));
    sf::View uiView = window.getDefaultView();

    
    std::vector<Enemy> enemies;
    float spawnTimer = 0.f;
    float spawnRate = 0.5f;
    int killCount = 0;
    sf::Clock clock;
    sf::Texture enemyTexture;
    if (!enemyTexture.loadFromFile("enemy.png"))
    {
        std::printf("ERROR: Could not load enemy.png\n");
    }

    sf::Texture bossTexture;
    if (!bossTexture.loadFromFile("boss_spritesheet.png")) { 
        std::printf("ERROR: Could not load boss image!\n");
    }
    
    sf::Texture bossBulletTexture;
    if (!bossBulletTexture.loadFromFile("boss_bullet.png")) { 
        std::printf("ERROR: Could not load boss bullet image!\n");
    }


    std::vector<std::string> obstacleFiles = {"brick.png", "tree.png", "rock.png"};
    std::vector<sf::Texture> obstacleTextures;

    sf::Texture floorTexture;
    if (floorTexture.loadFromFile("floor.png")) {

        floorTexture.setSmooth(false); 
    } else {
        std::printf("WARNING: Could not load floor.png\n");
    }
    for (const auto &file : obstacleFiles)
    {
        sf::Texture tex;
        if (tex.loadFromFile(file))
        {
            tex.setSmooth(false);
            obstacleTextures.push_back(tex);
        }
        else
        {
            std::printf("WARNING: Could not load %s\n", file.c_str());
        }
    }


    std::vector<std::vector<int>> dungeonMap(MAP_WIDTH, std::vector<int>(MAP_HEIGHT, 0));
    int numObstacleTypes = obstacleTextures.size();

   if (numObstacleTypes > 0) {
        
        int centerGridX = MAP_WIDTH / 2;
        int centerGridY = MAP_HEIGHT / 2;

        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                
                if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1) {
                    dungeonMap[x][y] = 1; 
                } 
                
                else if (std::rand() % 100 < 2 && (std::abs(x - centerGridX) > 3 || std::abs(y - centerGridY) > 3)) {
                    dungeonMap[x][y] = (std::rand() % numObstacleTypes) + 1;
                }
            }
        }
    }
    bgm.play();
    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

           if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                
               
                worldView = getLetterboxView(worldView, resized->size.x, resized->size.y);
                uiView = getLetterboxView(uiView, resized->size.x, resized->size.y);
            }
        }

        if (currentState == State::MENU)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            {
                window.clear(sf::Color::Black);
                    window.draw(loadingText);
                    window.display(); 

                    
                currentState = State::PLAYING;
                player.hp = player.maxHp; 
                player.isDead= false;
            player.sprite.setPosition({(MAP_WIDTH * TILE_SIZE) / 2.0f, (MAP_HEIGHT * TILE_SIZE) / 2.0f});
            
            killCount = 0;
            scoreText.setString("Kills: 0");
             
            enemies.clear();
            sf::sleep(sf::seconds(0.5f)); 
            clock.restart();
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            {
                window.close();
            }
        }

        else if (currentState == State::PLAYING)
        {
            player.update(dt, window, worldView, dungeonMap);

            worldView.setCenter(player.sprite.getPosition());

            spawnTimer -= dt;
            if (spawnTimer <= 0.f)
            {
                float randomAngle = static_cast<float>(std::rand() % 360) * 3.14159f / 180.f;
                sf::Vector2f spawnPos = player.sprite.getPosition() +
                                        sf::Vector2f(std::cos(randomAngle) * 600.f, std::sin(randomAngle) * 600.f);
                enemies.emplace_back(spawnPos, enemyTexture);
                spawnTimer = spawnRate;
            }

            for (auto& e : enemies) {
                e.update(dt, player.sprite.getPosition(), dungeonMap);
            }
            if (boss.isActive && boss.sprite) {
            
            // boss animation
            boss.animTimer += dt;
            if (boss.animTimer >= boss.animDuration) {
                boss.currentFrame++;
                if (boss.currentFrame >= boss.maxFrames) {
                    boss.currentFrame = 0;
                }
                boss.sprite->setTextureRect(sf::IntRect({boss.currentFrame * boss.frameWidth, 0}, {boss.frameWidth, boss.frameHeight}));
                boss.animTimer = 0.f;
            }

           
            sf::Vector2f dir = player.sprite.getPosition() - boss.sprite->getPosition(); 
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length > 0) {
                dir /= length; 
                boss.sprite->move(dir * boss.speed * dt); 
            }

            // boss shooting
            boss.shootTimer += dt;
            if (boss.shootTimer >= boss.shootCooldown) {
                
                enemyBullets.emplace_back();
                enemyBullets.back().velocity = dir;
                enemyBullets.back().speed = 250.f;
                enemyBullets.back().isSprite = true; 
                
                enemyBullets.back().sprite.emplace(bossBulletTexture);
                enemyBullets.back().sprite->setPosition(boss.sprite->getPosition());
                
            
                enemyBullets.back().sprite->setOrigin({16.f, 16.f}); 
                
                
                float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
                enemyBullets.back().sprite->setRotation(sf::degrees(angle));
                
                enemyBullets.back().sprite->setScale({-2.f, 2.f});

                boss.shootTimer = 0.f; 
            }

            
            float hpPercent = boss.hp / boss.maxHp;
            bossHealthBarFG.setSize({600.f * hpPercent, 15.f});
        }
        for (int i = 0; i < enemyBullets.size(); i++) {
            enemyBullets[i].update(dt); 
            
            if (enemyBullets[i].sprite && enemyBullets[i].sprite->getGlobalBounds().findIntersection(player.sprite.getGlobalBounds())) {
                player.takeDamage(); 
                enemyBullets.erase(enemyBullets.begin() + i);
                i--;
            }
        }

        
        if (boss.isActive && boss.sprite) {
            for (int i = 0; i < activeBullets.size(); i++) {
                if (activeBullets[i].shape.getGlobalBounds().findIntersection(boss.sprite->getGlobalBounds())) {
                    boss.hp -= 25; 
                    if(killSound) killSound->play(); 
                    
                    activeBullets.erase(activeBullets.begin() + i);
                    i--;

                    if (boss.hp <= 0) {
                        boss.isActive = false;
                        boss.isDefeated = true;

                        victoryTimer = victoryDuration;
                    }
                }
            }
        }

        if (boss.isDefeated && victoryTimer > 0.f) {
                victoryTimer -= dt;

                
                int alpha = 255; 
                if (victoryTimer < 2.f) {
                    alpha = static_cast<int>((victoryTimer / 2.f) * 255.f);
                    if (alpha < 0) alpha = 0; 
                }
                
               
                victoryText.setFillColor(sf::Color(255, 255, 0, alpha));
                
               
                victoryText.setOutlineColor(sf::Color(0, 0, 0, alpha)); 
            }

       
        for (int i = activeBullets.size() - 1; i >= 0; i--) {
            activeBullets[i].update(dt);
            bool bulletDestroyed = false;
            
            sf::Vector2f pos = activeBullets[i].shape.getPosition();
            int gridX = static_cast<int>(pos.x / TILE_SIZE);
            int gridY = static_cast<int>(pos.y / TILE_SIZE);
            
            // Wall Collision
            if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
                if (dungeonMap[gridX][gridY] > 0) {
                    activeBullets.erase(activeBullets.begin() + i);
                    bulletDestroyed = true;
                }
            }

            
            if (!bulletDestroyed) {
                for (int j = enemies.size() - 1; j >= 0; j--) {
                    if (activeBullets[i].shape.getGlobalBounds().findIntersection(enemies[j].sprite.getGlobalBounds())) {
                        
                        enemies.erase(enemies.begin() + j);
                        activeBullets.erase(activeBullets.begin() + i);
                        if(killSound) killSound->play();
                        
                        killCount++;
                        scoreText.setString("Kills: " + std::to_string(killCount));
                        
                      
                        if (killCount >= 35 && !boss.isActive && !boss.isDefeated) {
                            boss.isActive = true;
                            boss.hp = boss.maxHp;
                            
                            
                            boss.sprite.emplace(bossTexture);
                            boss.sprite->setTextureRect(sf::IntRect({0, 0}, {boss.frameWidth, boss.frameHeight}));
                            boss.sprite->setOrigin({boss.frameWidth / 2.f, boss.frameHeight / 2.f});
                            
                            boss.sprite->setScale({2.f, 2.f});

                            bossHealthBarBG.setSize({600.f, 15.f});
                            bossHealthBarBG.setFillColor(sf::Color(100, 0, 0));
                            bossHealthBarBG.setPosition({100.f, 100.f}); 
                            
                            bossHealthBarFG.setSize({600.f, 15.f});
                            bossHealthBarFG.setFillColor(sf::Color::Red);
                            bossHealthBarFG.setPosition({100.f, 100.f});
                            
                           
                            bossNameText.setPosition({325.f, 120.f});
                            
                            float randomAngle = (rand() % 360) * 3.14159f / 180.f; 
                            float spawnDist = 1200.f; 
                            float spawnX = player.sprite.getPosition().x + (std::cos(randomAngle) * spawnDist);
                            float spawnY = player.sprite.getPosition().y + (std::sin(randomAngle) * spawnDist);
                            
                            boss.sprite->setPosition({spawnX, spawnY});
                        }
                        
                        break; 
                    }
                }
            }
        }
        

           
            for (auto &e : enemies)
            {
                float dx = e.sprite.getPosition().x - player.sprite.getPosition().x;
                float dy = e.sprite.getPosition().y - player.sprite.getPosition().y;
                if (std::sqrt(dx * dx + dy * dy) < 30.f)
                    player.takeDamage();
            }

            if (player.hp<=0)
                currentState = State::GAMEOVER;
        }

        else if (currentState == State::GAMEOVER)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
                // RESET
                player.hp = player.maxHp;
                player.isDead = false;
                player.sprite.setPosition({spawnX, spawnY});
                player.sprite.setColor(sf::Color::White);
                
                enemies.clear();
                activeBullets.clear(); 
                enemyBullets.clear();  
                
                boss.isActive = false; 
                boss.isDefeated = false;
                
                killCount = 0;
                scoreText.setString("Kills: 0");
                currentState = State::PLAYING;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H))
            {
                currentState = State::MENU;
                player.hp = player.maxHp; 
            player.sprite.setPosition({(MAP_WIDTH * TILE_SIZE) / 2.0f, (MAP_HEIGHT * TILE_SIZE) / 2.0f});
            
            killCount = 0;
            scoreText.setString("Kills: 0");
            
            enemies.clear();
                
            }
        }

       
       window.clear(sf::Color::Black);

        if (currentState == State::MENU) {
            window.setView(uiView);
            window.draw(menuText);
        } 
        else if (currentState == State::PLAYING) {
            
          
            window.setView(worldView);
            window.draw(grid);

            for (int x = 0; x < MAP_WIDTH; x++) {
                for (int y = 0; y < MAP_HEIGHT; y++) {
                    int tileID = dungeonMap[x][y];
                    
                    sf::Sprite floorSprite(floorTexture);
                    sf::Vector2u floorTexSize = floorTexture.getSize();
                    floorSprite.setScale({TILE_SIZE / floorTexSize.x, TILE_SIZE / floorTexSize.y});
                    floorSprite.setPosition({x * TILE_SIZE, y * TILE_SIZE});
                    window.draw(floorSprite);

                    if (tileID > 0 && !obstacleTextures.empty()) {
                        sf::Sprite wallSprite(obstacleTextures[tileID - 1]);
                        sf::Vector2u wallTexSize = obstacleTextures[tileID - 1].getSize();
                        wallSprite.setScale({TILE_SIZE / wallTexSize.x, TILE_SIZE / wallTexSize.y});
                        wallSprite.setPosition({x * TILE_SIZE, y * TILE_SIZE});
                        window.draw(wallSprite);
                    }
                }
            }

            
            for (auto& e : enemies) window.draw(e.sprite);
            
            for (const auto& bullet : activeBullets) {
                window.draw(bullet.shape); 
            }
            
            player.draw(window);

           
            if (boss.isActive && boss.sprite) {
                window.draw(*boss.sprite);
            }

            

            for (const auto& bullet : enemyBullets) {
                if (bullet.isSprite && bullet.sprite) {
                    window.draw(*bullet.sprite); 
                }
            }

          
            sf::View savedWorldView = window.getView();
            window.setView(window.getDefaultView()); 

            sf::RectangleShape hpBar({200.f * ((float)player.hp / player.maxHp), 20.f});
            hpBar.setFillColor(sf::Color::Red);
            hpBar.setPosition({20.f, 20.f});
            window.draw(hpBar);

            window.draw(scoreText);

            if (boss.isActive) {
                window.draw(bossHealthBarBG);
                window.draw(bossHealthBarFG);
                window.draw(bossNameText);
            }

            if (boss.isDefeated && victoryTimer > 0.f) {
                window.draw(dimOverlay); 
                
                
                    window.draw(victoryText); 
                
            }

            window.setView(savedWorldView);
        } 

        
        else if (currentState == State::GAMEOVER) {
            window.setView(uiView);
            window.draw(gameOverText);
        }

        window.display();
    }
    return 0;
}