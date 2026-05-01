#include <SFML/Graphics.hpp>
#include <cmath> 
#include <vector>
#include <cstdlib> 
#include <ctime>
#include <string>

enum class State { MENU, PLAYING, GAMEOVER };
State currentState = State::MENU;

struct Bullet {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float speed;

    Bullet(sf::Vector2f startPosition, sf::Vector2f direction) {
        shape.setRadius(5.f);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({5.f, 5.f});
        shape.setPosition(startPosition);
        velocity = direction;
        speed = 800.f;
    }

    void update(float dt) {
        shape.move(velocity * speed * dt);
    }
};

struct Enemy {
    sf::Sprite sprite;
    float speed;

    Enemy(sf::Vector2f startPosition, const sf::Texture& enemyTexture): sprite(enemyTexture) {
        sprite.setTexture(enemyTexture);
        
     
        sf::Vector2u size = enemyTexture.getSize();
        sprite.setOrigin({static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f});
        
        sprite.setPosition(startPosition);
        
        sprite.setScale({1.0f, 1.0f}); 
        
        speed = 150.f; 
    }

    void update(float dt, sf::Vector2f playerPosition) {
        float dx = playerPosition.x - sprite.getPosition().x;
        float dy = playerPosition.y - sprite.getPosition().y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 0) {
            sf::Vector2f direction({dx / distance, dy / distance});
            sprite.move(direction * speed * dt);
            
          
           
            if (dx > 0) {
             
                sprite.setScale({1.0f, 1.0f}); 
            } else if (dx < 0) {
              
                sprite.setScale({-1.0f, 1.0f}); 
            }
        }
    }
};
class Player {
public:
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Texture gunTexture;
    sf::Sprite gunSprite;
    
    float speed = 300.f;
    int hp = 3;
    int maxHp = 3;
    bool isDead = false;
    float iFrameTimer = 0.f;
    float iFrameDuration = 1.0f;
    float fireRate = 0.15f;
    float fireCooldown = 0.f;

    Player(float startX, float startY) : sprite(texture), gunSprite(gunTexture) {
        
        if (!texture.loadFromFile("player.png")) {
            std::printf("ERROR: Could not load player.png\n");
        }
        
        texture.setSmooth(false); 
        
        sprite = sf::Sprite(texture);

        sf::Vector2u size = texture.getSize();
        sprite.setOrigin({static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f});
        
        
        sprite.setScale({0.1f,0.1f});
        if (!gunTexture.loadFromFile("gun.png")) {
            std::printf("ERROR: Could not load gun.png\n");
        }
        gunTexture.setSmooth(false);
        gunSprite = sf::Sprite(gunTexture);

        gunSprite.setScale({0.05f, 0.05f});

        sf::Vector2u gunSize = gunTexture.getSize();
        gunSprite.setOrigin({-15.f, static_cast<float>(gunSize.y) / 2.f}); 

        sprite.setPosition({startX, startY});
        gunSprite.setPosition({startX, startY});
    }

    void update(float dt, const sf::RenderWindow& window, const sf::View& worldView, std::vector<Bullet>& activeBullets) {
        if (isDead) return;

        if (iFrameTimer > 0.f) {
            iFrameTimer -= dt;
            sprite.setColor(sf::Color::Cyan);
        } else {
            sprite.setColor(sf::Color::White);
        }

        sf::Vector2f movement({0.f, 0.f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) movement.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) movement.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) movement.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) movement.x += 1.f;

        if (movement.x != 0.f || movement.y != 0.f) {
            float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            movement.x /= length;
            movement.y /= length;
        }

        sprite.move(movement * speed * dt);
        sf::Vector2f handOffset(12.f, 10.f); 
        gunSprite.setPosition(sprite.getPosition() + handOffset); 

        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixelPos, worldView);
        float dx = mousePos.x - sprite.getPosition().x;
        float dy = mousePos.y - sprite.getPosition().y;
        float angle = std::atan2(dy, dx) * 180.f / 3.14159f;
        gunSprite.setRotation(sf::degrees(angle));

        fireCooldown -= dt; 
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fireCooldown <= 0.f) {
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance != 0) { 
                sf::Vector2f fireDirection({dx / distance, dy / distance});
                activeBullets.emplace_back(gunSprite.getPosition(), fireDirection);
                fireCooldown = fireRate;
            }
        }
    }

    void takeDamage() {
        if (iFrameTimer <= 0.f && !isDead) {
            hp -= 1;
            iFrameTimer = iFrameDuration; 
            if (hp <= 0) {
                isDead = true;
                sprite.setColor(sf::Color(100, 100, 100)); 
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
        window.draw(gunSprite);
    }
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Dusk");
    
    Player player(400.f, 300.f);
    
    sf::Font font;
    bool hasFont = font.openFromFile("arial.ttf"); 
    if (!hasFont) std::printf("ERROR: Could not find arial.ttf!\n");

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
    if (hasFont) {
        scoreText.setCharacterSize(30);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setOutlineColor(sf::Color::Black);
        scoreText.setOutlineThickness(2.0f);
        scoreText.setPosition({20.f, 60.f}); 
        scoreText.setString("Kills: 0");
    }

    // GRID 
    sf::VertexArray grid(sf::PrimitiveType::Lines);
    sf::Color gridColor(30, 30, 30);
    for (int x = -5000; x <= 5000; x += 100) {
        sf::Vertex v1; v1.position = { static_cast<float>(x), -5000.f }; v1.color = gridColor; grid.append(v1);
        sf::Vertex v2; v2.position = { static_cast<float>(x),  5000.f }; v2.color = gridColor; grid.append(v2);
    }
    for (int y = -5000; y <= 5000; y += 100) {
        sf::Vertex v1; v1.position = { -5000.f, static_cast<float>(y) }; v1.color = gridColor; grid.append(v1);
        sf::Vertex v2; v2.position = {  5000.f, static_cast<float>(y) }; v2.color = gridColor; grid.append(v2);
    }

    sf::View worldView(sf::Vector2f(400.f, 300.f), sf::Vector2f(800.f, 600.f));
    sf::View uiView = window.getDefaultView();

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies; 
    float spawnTimer = 0.f;
    float spawnRate = 0.5f; 
    int killCount = 0;
    sf::Clock clock;
    sf::Texture enemyTexture;
    if (!enemyTexture.loadFromFile("enemy.png")) {
        std::printf("ERROR: Could not load enemy.png\n");
    }

   while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        if (currentState == State::MENU) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                currentState = State::PLAYING;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
                window.close();
            }
        } 
        
        else if (currentState == State::PLAYING) {
            player.update(dt, window, worldView, bullets);
            
            worldView.setCenter(player.sprite.getPosition());

            spawnTimer -= dt;
            if (spawnTimer <= 0.f) {
                float randomAngle = static_cast<float>(std::rand() % 360) * 3.14159f / 180.f;
                sf::Vector2f spawnPos = player.sprite.getPosition() + 
                                       sf::Vector2f(std::cos(randomAngle) * 600.f, std::sin(randomAngle) * 600.f);
                enemies.emplace_back(spawnPos, enemyTexture);
                spawnTimer = spawnRate;
            }

            for (auto& e : enemies) e.update(dt, player.sprite.getPosition());
            for (auto& b : bullets) b.update(dt);

            for (int i = bullets.size() - 1; i >= 0; i--) {
                for (int j = enemies.size() - 1; j >= 0; j--) {
                    float dx = bullets[i].shape.getPosition().x - enemies[j].sprite.getPosition().x;
                    float dy = bullets[i].shape.getPosition().y - enemies[j].sprite.getPosition().y;
                    if (std::sqrt(dx*dx + dy*dy) < 20.f) {
                        enemies.erase(enemies.begin() + j);
                        bullets.erase(bullets.begin() + i);
                        killCount++;
                        break;
                    }
                }
            }

            // Enemy vs Player
            for (auto& e : enemies) {
                float dx = e.sprite.getPosition().x - player.sprite.getPosition().x;
                float dy = e.sprite.getPosition().y - player.sprite.getPosition().y;
                if (std::sqrt(dx*dx + dy*dy) < 30.f) player.takeDamage();
            }

            if (player.isDead) currentState = State::GAMEOVER;
        } 
        
        else if (currentState == State::GAMEOVER) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
                // RESET EVERYTHING
                player.hp = player.maxHp;
                player.isDead = false;
                player.sprite.setPosition({400.f, 300.f});
                player.sprite.setColor(sf::Color::White);
                enemies.clear();
                bullets.clear();
                killCount = 0;
                currentState = State::PLAYING;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {
                currentState = State::MENU;
            }
        }

        window.clear(sf::Color::Black);

        if (currentState == State::MENU) {
            window.setView(uiView);
            window.draw(menuText);
        }
        else if (currentState == State::PLAYING) {
            // Draw the moving world
            window.setView(worldView);
            window.draw(grid);
            for (auto& e : enemies) window.draw(e.sprite);
            for (auto& b : bullets) window.draw(b.shape);
            player.draw(window);

            // Draw the static UI
            window.setView(uiView);
            if (hasFont) {
                scoreText.setString("Kills: " + std::to_string(killCount));
                window.draw(scoreText);
            }
            
            // Draw HP Bar
            sf::RectangleShape hpBar({200.f * (player.hp / (float)player.maxHp), 20.f});
            hpBar.setFillColor(sf::Color::Red);
            hpBar.setPosition({20.f, 20.f});
            window.draw(hpBar);
        }
        else if (currentState == State::GAMEOVER) {
            window.setView(uiView);
            window.draw(gameOverText);
        }

        window.display();
    }
    return 0;
}