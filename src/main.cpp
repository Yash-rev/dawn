#include <SFML/Graphics.hpp>
#include <cmath> 
#include <vector>
#include <cstdlib> 
#include <ctime>
#include <string>
#include <cstdio>

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
    sf::RectangleShape shape;
    float speed;

    Enemy(sf::Vector2f startPosition) {
        shape.setSize({20.f, 20.f}); 
        shape.setFillColor(sf::Color::Magenta);
        shape.setOrigin({10.f, 10.f});
        shape.setPosition(startPosition);
        speed = 150.f; 
    }

    void update(float dt, sf::Vector2f playerPosition) {
        float dx = playerPosition.x - shape.getPosition().x;
        float dy = playerPosition.y - shape.getPosition().y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 0) {
            sf::Vector2f direction({dx / distance, dy / distance});
            shape.move(direction * speed * dt);
        }
    }
};

class Player {
public:
    sf::CircleShape body;
    sf::RectangleShape gun;
    float speed;
    float fireRate;
    float fireCooldown;
    
    int hp;
    int maxHp; 
    bool isDead;
    float iFrameDuration; 
    float iFrameTimer;    

    Player(float startX, float startY) {
        body.setRadius(20.f);
        body.setFillColor(sf::Color::Blue);
        body.setOrigin({20.f, 20.f}); 
        
        gun.setSize({30.f, 8.f});
        gun.setFillColor(sf::Color::Red);
        gun.setOrigin({0.f, 4.f}); 

        setPosition({startX, startY});
        speed = 300.f; 
        fireRate = 0.15f; 
        fireCooldown = 0.f;
        
        maxHp = 3;
        hp = maxHp;             
        isDead = false;
        iFrameDuration = 1.0f; 
        iFrameTimer = 0.f;
    }

    void setPosition(sf::Vector2f pos) {
        body.setPosition(pos);
        gun.setPosition(pos);
    }

    // --- NEW: We now pass the worldView in so we can calculate mouse coordinates correctly ---
    void update(float dt, const sf::RenderWindow& window, const sf::View& worldView, std::vector<Bullet>& activeBullets) {
        if (isDead) return;

        if (iFrameTimer > 0.f) {
            iFrameTimer -= dt;
            body.setFillColor(sf::Color::Cyan); 
        } else {
            body.setFillColor(sf::Color::Blue);
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

        body.move(movement * speed * dt);
        gun.move(movement * speed * dt);

        // --- NEW: Map mouse pixels to the moving Camera coordinates ---
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixelPos, worldView);

        float dx = mousePos.x - body.getPosition().x;
        float dy = mousePos.y - body.getPosition().y;

        const float PI = 3.14159265f;
        float angle = std::atan2(dy, dx) * 180.f / PI;
        gun.setRotation(sf::degrees(angle));

        fireCooldown -= dt; 
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && fireCooldown <= 0.f) {
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance != 0) { 
                sf::Vector2f fireDirection({dx / distance, dy / distance});
                activeBullets.emplace_back(body.getPosition(), fireDirection);
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
                body.setFillColor(sf::Color(100, 100, 100)); 
                gun.setFillColor(sf::Color(50, 50, 50));
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(body);
        window.draw(gun);
    }
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Dusk");
    Player player(400.f, 300.f);

    bool hasFont = false;
    
    // --- NEW: CREATE THE CAMERAS ---
    // The worldView is a camera that tracks the player
    sf::View worldView(sf::Vector2f(400.f, 300.f), sf::Vector2f(800.f, 600.f));
    // The uiView is a static camera that never moves, so text stays on screen
    sf::View uiView = window.getDefaultView();

   // --- BULLETPROOF BACKGROUND GRID FOR SFML 3 ---
    sf::VertexArray grid(sf::PrimitiveType::Lines);
    sf::Color gridColor(30, 30, 30);

    for (int x = -5000; x <= 5000; x += 100) {
        sf::Vertex v1;
        v1.position = { static_cast<float>(x), -5000.f };
        v1.color = gridColor;
        grid.append(v1);

        sf::Vertex v2;
        v2.position = { static_cast<float>(x), 5000.f };
        v2.color = gridColor;
        grid.append(v2);
    }

    for (int y = -5000; y <= 5000; y += 100) {
        sf::Vertex v1;
        v1.position = { -5000.f, static_cast<float>(y) };
        v1.color = gridColor;
        grid.append(v1);

        sf::Vertex v2;
        v2.position = { 5000.f, static_cast<float>(y) };
        v2.color = gridColor;
        grid.append(v2);
    }

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies; 
    
    float spawnTimer = 0.f;
    float spawnRate = 0.5f; 
    
    int killCount = 0;

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        std::printf("ERROR: Could not find arial.ttf!\n");
    }

    sf::Text scoreText(font);
    scoreText.setCharacterSize(30);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({20.f, 60.f});

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // --- NEW: UPDATE CAMERA POSITION ---
        // Center the camera squarely on the player every single frame
        worldView.setCenter(player.body.getPosition());

        player.update(dt, window, worldView, bullets);

        if (!player.isDead) {
            spawnTimer -= dt;
            if (spawnTimer <= 0.f) {
                float randomAngle = static_cast<float>(std::rand() % 360);
                const float PI = 3.14159265f;
                float angleRadians = randomAngle * PI / 180.f;
                
                // Spawn radius is 600px from the PLAYER, not from 0,0
                float spawnRadius = 600.f;
                sf::Vector2f spawnPos = player.body.getPosition() + 
                    sf::Vector2f(std::cos(angleRadians) * spawnRadius, std::sin(angleRadians) * spawnRadius);
                
                enemies.emplace_back(spawnPos);
                spawnTimer = spawnRate; 
            }
        }

        // We use a safe margin of 1000 pixels instead of fixed screen coords to delete bullets
        for (int i = bullets.size() - 1; i >= 0; i--) {
            bullets[i].update(dt);
            
            float distFromPlayerX = std::abs(bullets[i].shape.getPosition().x - player.body.getPosition().x);
            float distFromPlayerY = std::abs(bullets[i].shape.getPosition().y - player.body.getPosition().y);
            
            if (distFromPlayerX > 1000.f || distFromPlayerY > 1000.f) {
                bullets.erase(bullets.begin() + i);
            }
        }

        for (auto& enemy : enemies) {
            enemy.update(dt, player.body.getPosition());
        }

        for (int i = bullets.size() - 1; i >= 0; i--) {
            bool bulletHitSomething = false;
            for (int j = enemies.size() - 1; j >= 0; j--) {
                float dx = bullets[i].shape.getPosition().x - enemies[j].shape.getPosition().x;
                float dy = bullets[i].shape.getPosition().y - enemies[j].shape.getPosition().y;
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance < 15.f) { 
                    enemies.erase(enemies.begin() + j); 
                    bulletHitSomething = true;
                    killCount++;
                    break; 
                }
            }
            if (bulletHitSomething) {
                bullets.erase(bullets.begin() + i);
            }
        }

        if (!player.isDead) {
            for (const auto& enemy : enemies) {
                float dx = player.body.getPosition().x - enemy.shape.getPosition().x;
                float dy = player.body.getPosition().y - enemy.shape.getPosition().y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < 30.f) {
                    player.takeDamage();
                }
            }
        }

        if (hasFont) {
            scoreText.setString("Kills: " + std::to_string(killCount));
        }

        window.clear(sf::Color::Black);
        
        // --- THE TWO-STEP RENDER PROCESS ---
        
        // STEP 1: Look through the World Camera
        window.setView(worldView);
        window.draw(grid); // Draw the floor first
        player.draw(window);
        for (const auto& bullet : bullets) window.draw(bullet.shape);
        for (const auto& enemy : enemies) window.draw(enemy.shape);
        
        // STEP 2: Switch to the static UI Camera
        window.setView(uiView);
        
        sf::RectangleShape hpBackground({200.f, 20.f});
        hpBackground.setPosition({20.f, 20.f});
        hpBackground.setFillColor(sf::Color(100, 0, 0)); 
        window.draw(hpBackground);

        int displayHp = std::max(0, player.hp); 
        float hpPercent = static_cast<float>(displayHp) / static_cast<float>(player.maxHp);
        sf::RectangleShape hpForeground({200.f * hpPercent, 20.f});
        hpForeground.setPosition({20.f, 20.f});
        hpForeground.setFillColor(sf::Color::Red); 
        window.draw(hpForeground);

        if (hasFont) {
            window.draw(scoreText);
        }

        window.display();
    }

    return 0;
}