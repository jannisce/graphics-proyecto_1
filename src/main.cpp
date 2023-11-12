#include <SDL2/SDL.h>
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include "face.h"
#include "color.h"
#include "fragment.h"
#include "line.h"
#include "loadOBJ.h"
#include "triangle.h"
#include "uniform.h"
#include "vertex.h"
#include "shaders.h"
#include "framebuffer.h"

float x = 3.14f / 3.0f;

Color clearColor = {0, 0, 0, 255};
Color currentColor = {255, 255, 255, 255};
Color color_a(255, 0, 0, 255); // Red color
Color color_b(0, 255, 0, 255); // Green color
Color color_c(0, 0, 255, 255); // Blue color

SDL_Window* window;
Uniform uniform;
Uniform uniform2;
Uniform uniform4;
Uniform uniform5;
Uniform uniform6;

Planeta planeta1;
Planeta planeta2;
Planeta planeta4;
Planeta planeta5;
Planeta planeta6;

struct Star {
    glm::vec3 position;
    uint8_t brightness;
};

std::array<double, WINDOW_WIDTH * WINDOW_HEIGHT> zBuffer;

glm::vec3 L = glm::vec3(0, 0, 400.0f); // Configura la dirección de la luz según tus necesidades

struct Camera {
    glm::vec3 cameraPosition;
    glm::vec3 targetPosition;
    glm::vec3 upVector;
};

void clear(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
}

Color interpolateColor(const glm::vec3& barycentricCoord, const Color& color_a, const Color& color_b, const Color& color_c) {
    float u = barycentricCoord.x;
    float v = barycentricCoord.y;
    float w = barycentricCoord.z;

    // Realiza una interpolación lineal para cada componente del color
    uint8_t r = static_cast<uint8_t>(u * color_a.r + v * color_b.r + w * color_c.r);
    uint8_t g = static_cast<uint8_t>(u * color_a.g + v * color_b.g + w * color_c.g);
    uint8_t b = static_cast<uint8_t>(u * color_a.b + v * color_b.b + w * color_c.b);
    uint8_t a = static_cast<uint8_t>(u * color_a.a + v * color_b.a + w * color_c.a);

    return Color(r, g, b, a);
}

bool isBarycentricCoord(const glm::vec3& barycentricCoord) {
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 &&
           barycentricCoord.x <= 1 && barycentricCoord.y <= 1 && barycentricCoord.z <= 1 &&
           glm::abs(1 - (barycentricCoord.x + barycentricCoord.y + barycentricCoord.z)) < 0.00001f;
}

glm::vec3 calculateBarycentricCoord(const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& P) {
    float denominator = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    float u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
    float w = 1 - u - v;
    return glm::vec3(u, v, w);
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    std::vector<Fragment> fragments;


    // Calculate the bounding box of the triangle
    int minX = static_cast<int>(std::min({a.position.x, b.position.x, c.position.x}));
    int minY = static_cast<int>(std::min({a.position.y, b.position.y, c.position.y}));
    int maxX = static_cast<int>(std::max({a.position.x, b.position.x, c.position.x}));
    int maxY = static_cast<int>(std::max({a.position.y, b.position.y, c.position.y}));

    // Iterate over each point in the bounding box
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
            glm::vec3 barycentricCoord = calculateBarycentricCoord(a.position, b.position, c.position, pixelPosition);

            if (isBarycentricCoord(barycentricCoord)) {
                Color p {0, 0, 0};
                // Interpolate attributes (color, depth, etc.) using barycentric coordinates
                Color interpolatedColor = interpolateColor(barycentricCoord, p, p, p);

                // Calculate the interpolated Z value using barycentric coordinates
                float interpolatedZ = barycentricCoord.x * a.position.z + barycentricCoord.y * b.position.z + barycentricCoord.z * c.position.z;

                // Create a fragment with the position, interpolated attributes, and Z coordinate
                Fragment fragment;
                fragment.position = glm::ivec2(x, y);
                fragment.color = interpolatedColor;
                fragment.z = interpolatedZ;

                fragments.push_back(fragment);
            }
        }
    }

    return fragments;
}

std::vector<Star> generateStars(int numStars, int minX, int maxX, int minY, int maxY, uint8_t minBrightness, uint8_t maxBrightness) {
    std::vector<Star> stars;
    for (int i = 0; i < numStars; ++i) {
        int x = rand() % (maxX - minX + 1) + minX;
        int y = rand() % (maxY - minY + 1) + minY;
        uint8_t brightness = rand() % (maxBrightness - minBrightness + 1) + minBrightness;
        stars.push_back({glm::vec3(x, y, 0), brightness});
    }
    return stars;
}

glm::mat4 createModelMatrix(glm::vec3 matrixTranslation, glm::vec3 matrixRotation, float radianSpeed) {
    static float rotationSpeed = 4.0f;  // Ajusta la velocidad de rotación aquí
    glm::mat4 translation = glm::translate(glm::mat4(1), matrixTranslation);
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.4f, 0.6f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((x += rotationSpeed) * radianSpeed), glm::vec3(0.0f, -1.0f, 0.0f)); // Solo en el eje vertical
    return translation * scale * rotation;
}

glm::mat4 createModelMatrixSol(glm::vec3 matrixTranslation, glm::vec3 matrixRotation, float radianSpeed) {
    static float rotationSpeed = 2.5f;  // Ajusta la velocidad de rotación aquí
    glm::mat4 translation = glm::translate(glm::mat4(1), matrixTranslation);
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.4f, 2.2f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((x += rotationSpeed) * radianSpeed), glm::vec3(0.0f, 1.0f, 0.0f)); // Solo en el eje vertical
    return translation * scale * rotation;
}

glm::vec3 calculatePosition(float rotation, float radius){
    float positionX = glm::cos(rotation) * radius;
    float positionZ = glm::sin(rotation) * radius;
    return glm::vec3(positionX, 0.0f, positionZ);
}


glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 1.5f, 1.5f));
    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 0.75f, 1.0f));
    return viewport;
}

glm::mat4 createViewMatrix(glm::vec3 cameraPosition, glm::vec3 targetPosition, glm::vec3 upVector){
    return glm::lookAt(
            cameraPosition,
            targetPosition,
            upVector
            );
}

void render(const std::vector<Vertex>& vertexArray,  const Uniform& uniform, int planeta) {
    std::vector<Vertex> transformedVertexArray;
    for (const auto& vertex : vertexArray) {
        auto transformedVertex = vertexShader(vertex, uniform);
        transformedVertexArray.push_back(transformedVertex);
    }

    for (size_t i = 0; i < transformedVertexArray.size(); i += 3) {
        const Vertex& a = transformedVertexArray[i];
        const Vertex& b = transformedVertexArray[i + 1];
        const Vertex& c = transformedVertexArray[i + 2];

        glm::vec3 A = a.position;
        glm::vec3 B = b.position;
        glm::vec3 C = c.position;

        glm::vec3 edge1 = B - A;
        glm::vec3 edge2 = C - A;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Bounding box para el triangulo
        int minX = static_cast<int>(std::min({A.x, B.x, C.x}));
        int minY = static_cast<int>(std::min({A.y, B.y, C.y}));
        int maxX = static_cast<int>(std::max({A.x, B.x, C.x}));
        int maxY = static_cast<int>(std::max({A.y, B.y, C.y}));

        // Iterating
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (y>0 && y<WINDOW_HEIGHT && x>0 && x<WINDOW_WIDTH) {
                    glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
                    glm::vec3 barycentricCoord = calculateBarycentricCoord(A, B, C, pixelPosition);

                    if (isBarycentricCoord(barycentricCoord)) {
                        Color barycentricColor {0, 0, 0};
                        Color interpolatedColor = interpolateColor(barycentricCoord, barycentricColor, barycentricColor, barycentricColor);

                        float depth = barycentricCoord.x * A.z + barycentricCoord.y * B.z + barycentricCoord.z * C.z;

                        glm::vec3 normal = a.normal * barycentricCoord.x + b.normal * barycentricCoord.y+ c.normal * barycentricCoord.z;
                        glm::vec3 original = a.original * barycentricCoord.x + b.original * barycentricCoord.y + c.original * barycentricCoord.z;

                        // Calculate the position 'P' of the fragment
                        glm::vec3 P = glm::vec3(glm::vec3 (0.0f,0.0f,1.0f));
                        glm::vec3 lightDirection = glm::normalize(L - P);

                        Fragment fragment;

                        // Calculate the intensity of the light using Lambertian attenuation
                        float intensity = glm::dot(normal, lightDirection);
                        fragment.intensity = intensity;

                        if (intensity <= 0){
                            continue;
                        }

                        Color finalColor = interpolatedColor * intensity;
                        fragment.position = glm::ivec2(x, y);
                        fragment.color = finalColor;
                        fragment.z = depth;
                        fragment.original = original;

                        int index = y * WINDOW_WIDTH + x;
                        if (depth < zBuffer[index]) {
                            // Apply fragment shader to calculate final color
                            Color fragmentShaderf; //= fragmentShaderJupiter(fragment);

                            switch(planeta){
                                case sol:
                                    fragmentShaderf = fragmentShaderSol(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case mercurio:
                                    fragmentShaderf = fragmentShaderRandom(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case tierra:
                                    fragmentShaderf = fragmentShaderTierra(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case jupiter:
                                    fragmentShaderf = fragmentShaderJupiter(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                                case neptuno:
                                    fragmentShaderf = fragmentShaderNeptuno(fragment);
                                    SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                                    break;
                            }

                            SDL_RenderDrawPoint(renderer, x, WINDOW_HEIGHT-y);
                            // Update the z-buffer value for this pixel
                            zBuffer[index] = depth;
                        }
                    }
                }
            }
        }
    }
}


int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Space Travel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, 0);

    // Variables para el cálculo de FPS
    int frameCount = 0;
    double totalTime = 0.0;
    int fps = 0;
    auto startTime = std::chrono::high_resolution_clock::now();


    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);


    srand(time(nullptr));

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<Face> faces;

    if (!loadOBJ("models/sphere.obj", vertices, normal, faces)) {
        std::cerr << "Error loading OBJ file." << std::endl;
        return 1;
    }

    std::vector<Planeta> planetas;

    float rotation = 0.0f;
    float rotation2 = 0.0f;
    float rotation3 = 0.0f;
    float rotation4 = 0.0f;
    float rotation5 = 0.0f;
    float xRotate = 0.0f;
    float yRotate = 0.0f;
    float moveSpeed = 0.4f;

    Camera camera;

    glm::vec3 cameraPosition(0.0f, 0.0f, 20.0f);
    glm::vec3 targetPosition(0.0f, 1.0f, 0.0f);
    glm::vec3 upVector(0.0f, 0.5f, 0.0f);

    L = cameraPosition - targetPosition;

    std::vector<Vertex> vertexArray = setupVertexArray(vertices, normal, faces);

    SDL_Event event;
    bool quit = false;

    // Genera estrellas aleatorias en un rango más grande que la cámara
    std::vector<Star> stars = generateStars(18000, -1000, 2500, -1000, 2500, 20, 250);
    glm::vec3 moveVector; // Declarar moveVector aquí

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        // Mueve la cámara hacia adelante limitando la distancia mínima
                        moveVector = moveSpeed * glm::normalize(L); // Asignar el valor a moveVector
                        if (glm::length(cameraPosition - targetPosition - moveVector) >= 1.0f) {
                            cameraPosition -= moveVector;
                            targetPosition -= moveVector;
                        }
                        break;
                    case SDLK_s:
                        // Mueve la cámara hacia atrás
                        cameraPosition += moveSpeed * glm::normalize(L);
                        targetPosition += moveSpeed * L;
                        break;
                    case SDLK_a:
                        xRotate -= 1.0f;
                        break;
                    case SDLK_d:
                        xRotate += 1.0f;
                        break;
                    case SDLK_RIGHT:
                        xRotate += 1.0f;
                        break;
                    case SDLK_LEFT:
                        xRotate -= 1.0f;
                        break;
                    case SDLK_UP:
                        yRotate += 1.0f;
                        break;
                    case SDLK_DOWN:
                        yRotate -= 1.0f;
                        break;
                }
            }
        }




        planetas.clear();
        rotation -= 0.022f;
        rotation2 += 0.018f;
        rotation3 -= 0.026f;
        rotation4 += 0.022f;
        rotation5 -= 0.028f;
        targetPosition = glm::vec3(5.0f * sin(glm::radians(xRotate)) * cos(glm::radians(yRotate)), 5.0f * sin(glm::radians(yRotate)), -5.0f * cos(glm::radians(xRotate)) * cos(glm::radians(yRotate))) + cameraPosition;


        glm::vec3 translationMatrixMercurio = calculatePosition(rotation, -1.5f);
        glm::vec3 translationMatrixTierra = calculatePosition(rotation2, 2.5f);
        glm::vec3 translationMatrixJupiter = calculatePosition(rotation4, -3.5f);
        glm::vec3 translationMatrixNeptuno = calculatePosition(rotation5, 4.5f);

        uniform.model = createModelMatrixSol(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.2f);
        uniform.view = createViewMatrix(cameraPosition, targetPosition, upVector);
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        planeta1.uniform = uniform;
        planeta1.vertex = &vertexArray;
        planeta1.shader = sol;

        uniform4.model = createModelMatrix(translationMatrixMercurio, glm::vec3(1.0f, 1.0f, 1.0f), 0.2f);
        uniform4.view = createViewMatrix(cameraPosition, targetPosition, upVector);
        uniform4.viewport = createViewportMatrix();
        uniform4.projection = createProjectionMatrix();

        planeta4.uniform = uniform4;
        planeta4.vertex = &vertexArray;
        planeta4.shader = mercurio;

        uniform2.model = createModelMatrix(translationMatrixTierra, glm::vec3(1.0f, 1.0f, 1.0f), 0.3f);
        uniform2.view = createViewMatrix(cameraPosition, targetPosition, upVector);
        uniform2.viewport = createViewportMatrix();
        uniform2.projection = createProjectionMatrix();

        planeta2.uniform = uniform2;
        planeta2.vertex = &vertexArray;
        planeta2.shader = tierra;

        uniform5.model = createModelMatrix(translationMatrixJupiter, glm::vec3(1.0f, 1.0f, 1.0f), 0.4f);
        uniform5.view = createViewMatrix(cameraPosition, targetPosition, upVector);
        uniform5.viewport = createViewportMatrix();
        uniform5.projection = createProjectionMatrix();

        planeta5.uniform= uniform5;
        planeta5.vertex = &vertexArray;
        planeta5.shader = jupiter;

        uniform6.model = createModelMatrix(translationMatrixNeptuno, glm::vec3(1.0f, 1.0f, 1.0f), 0.4f);
        uniform6.view = createViewMatrix(cameraPosition, targetPosition, upVector);
        uniform6.viewport = createViewportMatrix();
        uniform6.projection = createProjectionMatrix();

        planeta6.uniform= uniform6;
        planeta6.vertex = &vertexArray;
        planeta6.shader = neptuno;
        
        planetas.push_back(planeta1);
        planetas.push_back(planeta2);
        planetas.push_back(planeta4);
        planetas.push_back(planeta5);
        planetas.push_back(planeta6);
        
        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(renderer);

        // Clear z-buffer at the beginning of each frame
        std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<double>::max());

        // Dibuja las estrellas en el fondo
        for (const Star& star : stars) {
            SDL_SetRenderDrawColor(renderer, star.brightness, star.brightness, star.brightness, 255);
            SDL_RenderDrawPoint(renderer, star.position.x, star.position.y);
        }

        // Llamada a la función render para cada planeta
        for(const Planeta& planeta : planetas){
            render(*planeta.vertex, planeta.uniform, planeta.shader);
        }

        SDL_RenderPresent(renderer);

        // Calcula el tiempo transcurrido en este fotograma
        auto endTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(endTime - startTime).count();
        startTime = endTime;

        // Actualiza el tiempo total y el recuento de fotogramas
        totalTime += frameTime;
        frameCount++;

        // Si ha pasado un segundo, calcula los FPS y actualiza el título de la ventana
        if (totalTime >= 1.0) {
            fps = static_cast<int>(frameCount);  // Convierte el recuento de fotogramas a entero
            frameCount = 0;
            totalTime = 0.0;

            // Actualiza el título de la ventana con los FPS
            std::string windowTitle = "Space Travel - FPS: " + std::to_string(fps);
            SDL_SetWindowTitle(window, windowTitle.c_str());
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}