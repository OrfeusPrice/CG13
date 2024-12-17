#include <SFML/Graphics.hpp>
#include <GL/glew.h>

#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

#include "shader.h"
#include "camera.h"
#include "model.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 50.0f, 200.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


void checkOpenGLerror() {
    
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << error << std::endl;

        switch (error) {
        case GL_INVALID_OPERATION:
            std::cout << "GL_INVALID_OPERATION: Неверная операция." << std::endl;
            break;
        case GL_INVALID_VALUE:
            std::cout << "GL_INVALID_VALUE: Неверное значение." << std::endl;
            break;
        case GL_INVALID_ENUM:
            std::cout << "GL_INVALID_ENUM: Неверный перечисляемый тип." << std::endl;
            break;
        case GL_OUT_OF_MEMORY:
            std::cout << "GL_OUT_OF_MEMORY: Недостаточно памяти." << std::endl;
            break;
        default:
            std::cout << "Неизвестный код ошибки: " << error << std::endl;
            break;
        }
    }
    
}

void Init()
{
    glEnable(GL_DEPTH_TEST);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(sf::Window& window)
{
    // Проверка закрытия окна при нажатии ESC
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
        window.close();

    // Управление камерой с помощью клавиш WASD
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        camera.ProcessKeyboard(UP, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        camera.ProcessKeyboard(ROTATE_LEFT, deltaTime);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        camera.ProcessKeyboard(ROTATE_RIGHT, deltaTime);



    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
    float xpos = static_cast<float>(mousePosition.x);
    float ypos = static_cast<float>(mousePosition.y);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // Инвертируем для работы с координатами камеры
    lastX = xpos;
    lastY = ypos;

    // Обрабатываем движение мыши через камеру
    camera.ProcessMouseMovement(xoffset, yoffset);
}

int main()
{
    setlocale(LC_ALL, "ru");

    sf::Window window(sf::VideoMode(800, 800), "3D figures", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);

    window.setMouseCursorVisible(false);

    glewInit();

    Init();

    Shader asteroidShader("asteroids.vs", "asteroids.fs");
    Shader planetShader("10.3.planet.vs", "10.3.planet.fs");

    // load models
    // -----------
    //Model rock("resources/rock/rock.obj");
    //Model planet("resources/planet/planet.obj");
    Model planet("resources/Cat_v1_L3.123cb1b1943a-2f48-4e44-8f71-6bbe19a3ab64/12221_Cat_v1_l3.obj");
    //Model planet("resources/luffy/obj/luffy.obj");
    Model rock("resources/Bird_v1_L2.123ca5dbb1bc-8ef6-44e4-b558-3e6e2bbc7dd7/12248_Bird_v1_L2.obj");


    // generate a large list of semi-random model transformation matrices
    // ------------------------------------------------------------------
    unsigned int amount = 7;
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(static_cast<unsigned int>(time(NULL))); // initialize random seed
    float radius = 150.0;
    float offset = 25.0f;

    std::vector<glm::vec3> initialPositions(amount);

    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));
        initialPositions[i] = glm::vec3(x, y, z);
        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }

    // configure instanced array
    // -------------------------
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STREAM_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < rock.meshes.size(); i++)
    {
        unsigned int VAO = rock.meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    // GenerateCircleVertexes();

    sf::Clock clock;


    std::vector<glm::vec3> rotationAxes(amount);
    std::vector<float> rotationSpeeds(amount);
    for (unsigned int i = 0; i < amount; i++) {
        // Случайная ось вращения
        rotationAxes[i] = glm::normalize(glm::vec3(
            static_cast<float>(rand() % 100) / 100.0f,
            static_cast<float>(rand() % 100) / 100.0f,
            static_cast<float>(rand() % 100) / 100.0f
        ));

        // Случайная скорость вращения (градусы в секунду)
        rotationSpeeds[i] = static_cast<float>(rand() % 100) / 10.0f; // от 0 до 10
    }
    std::vector<float> orbitAngles(amount, 0.0f);
    std::vector<float> selfRotationAngles(amount, 0.0f);

    // Основной цикл
    while (window.isOpen())
    {
        // Управление временными логиками
        float currentFrame = clock.getElapsedTime().asSeconds();
        deltaTime = (currentFrame - lastFrame) * 10;
        lastFrame = currentFrame;

        // Обработка ввода
        processInput(window);

        const float orbitSpeed = 0.1f; // Скорость вращения орбиты
        for (unsigned int i = 0; i < amount; i++)
        {
            // 1. Создаём начальную единичную матрицу
            glm::mat4 model = glm::mat4(1.0f);

            // 2. Вращение метеорита вокруг орбиты (вокруг "солнца")
            orbitAngles[i] += orbitSpeed * deltaTime; // `orbitSpeed` в градусах в секунду
            if (orbitAngles[i] > 360.0f)
                orbitAngles[i] -= 360.0f; // Сбрасываем угол
            float orbitAngleRad = glm::radians(orbitAngles[i]);

            // Рассчитываем новую позицию объекта на орбите
            glm::mat4 orbitRotation = glm::rotate(glm::mat4(1.0f), orbitAngleRad, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 position = initialPositions[i]; // Начальная позиция
            glm::vec4 newPosition = orbitRotation * glm::vec4(position, 1.0f);

            // Перемещение объекта на новую орбитальную позицию
            model = glm::translate(model, glm::vec3(newPosition));

            // 3. Вращение объекта вокруг своей оси
            selfRotationAngles[i] += rotationSpeeds[i] * deltaTime; // `rotationSpeeds[i]` в градусах в секунду
            if (selfRotationAngles[i] > 360.0f) {
                selfRotationAngles[i] -= 360.0f; // Сбрасываем угол
            }
            float selfRotationAngleRad = glm::radians(selfRotationAngles[i]);

            // Добавляем вращение вокруг собственной оси
            model = glm::rotate(model, selfRotationAngleRad, rotationAxes[i]);

            // 4. Сохраняем итоговую матрицу трансформации
            modelMatrices[i] = model;
        }



        // Обновляем буфер
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, amount * sizeof(glm::mat4), &modelMatrices[0]);

        // Очистка экрана
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        // Настройка матриц трансформации
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix(); // Предполагаем, что у вас есть объект камеры
        asteroidShader.use();
        asteroidShader.setMat4("projection", projection);
        asteroidShader.setMat4("view", view);
        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);

        // Рисуем планету
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        float angle = glm::radians(270.0f);
        glm::vec3 axis = glm::vec3(1.0f, 0.0f, 0.0f);
        model = glm::rotate(model, angle, axis);
        planetShader.setMat4("model", model);
        planet.Draw(planetShader);

        // Рисуем метеориты
        asteroidShader.use();
        asteroidShader.setInt("texture_diffuse1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rock.textures_loaded[0].id);
        for (unsigned int i = 0; i < rock.meshes.size(); i++) {
            glBindVertexArray(rock.meshes[i].VAO);
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(rock.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }

        // ОбменBuffers
        window.display();

        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
    }
    

    return 0;
}
