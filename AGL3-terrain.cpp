// ==========================================================================
// AGL3:  GL/GLFW init AGLWindow and AGLDrawable class definitions
//
// Ver.3  14.I.2020 (c) A. Łukaszewski
// ==========================================================================
// AGL3 Terrain rendering
// By   Michał Chawar
//===========================================================================
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <AGL3Window.hpp>
#include <AGL3Drawable.hpp>
#include <Config.hpp>
#include <TileManager.hpp>
#include <FrameHistory.hpp>

const float PI = M_PI;

class MySphere : public AGLDrawable {
public:
    MySphere() : AGLDrawable(0) {
        generateSphereVertices(vertices, indices, 637800.0f - 520.0f, 360, 180);
        setShaders();
        setBuffers();
    }
    void setShaders() {
        compileShadersFromFile("shaders/sphere.vs", "shaders/sphere.fs");
    }
    void setBuffers() {
        bindBuffers();

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // attribute 0, must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,//24,             // stride
            (void*)0            // array buffer offset
        );

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    }
    void draw(const glm::mat4& view, const glm::mat4& projection) {
        bindProgram();
        bindBuffers();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glm::mat4 model(1.0f);
        model = glm::translate( model, center );
        model = glm::scale    ( model, glm::vec3(scale * scaleFactor) );

        glUniformMatrix4fv(10, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(14, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(15, 1, GL_FALSE, glm::value_ptr(projection));

        glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, (void*)0);
        return;
    }
    void move(const glm::vec3& v) {
        center += v;
    }
    float getScale() const {
        return scale;
    }
    glm::vec3 getCenter() const {
        return center;
    }
private:
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    float scale = 1.0;

    GLuint EBO;

    void generateSphereVertices(std::vector<glm::vec3> &vertices, std::vector<unsigned int> &indices, float radius = 1.0f, int sectorCount = 36, int stackCount = 18) {
        float x, y, z, xy;
        
        // sektory - pionowe wycinki sfery
        float sectorStep = 2 * M_PI / sectorCount;
        // stacki  - poziome przekroje sfery
        float stackStep  =     M_PI / stackCount;
        float sectorAngle, stackAngle;

        // std::vector<glm::vec3> temp;


        // dla n stacków mamy n+1 poziomów, na których znajdują się punkty
        // Poziom 0 - 1 punkt
        stackAngle = M_PI / 2;
        xy = radius * cosf(stackAngle);      // r * cos(u)
        z  = radius * sinf(stackAngle);      // r * sin(u)

        sectorAngle = 0.0;
        x = xy * cosf(sectorAngle);      // r * cos(u) * cos(v)
        y = xy * sinf(sectorAngle);      // r * cos(u) * sin(v)

        vertices.push_back(glm::vec3(x, z, y));

        // Poziomy 1...n-1
        for (int i = 1; i < stackCount; i++) {
            stackAngle = M_PI / 2 - i * stackStep;
            xy = radius * cosf(stackAngle);      // r * cos(u)
            z  = radius * sinf(stackAngle);      // r * sin(u)

            // dla m sektorów mamy m punktów (bo ostatni sektor łączy się z pierwszym)
            for (int j = 0; j < sectorCount; j++) {
                sectorAngle = j * sectorStep;

                x = xy * cosf(sectorAngle);      // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);      // r * cos(u) * sin(v)
                vertices.push_back(glm::vec3(x, z, y));
            }
        }

        // Poziom n - jeden punkt
        stackAngle = -M_PI / 2;
        xy = radius * cosf(stackAngle);      // r * cos(u)
        z  = radius * sinf(stackAngle);      // r * sin(u)

        sectorAngle = 0.0;
        x = xy * cosf(sectorAngle);      // r * cos(u) * cos(v)
        y = xy * sinf(sectorAngle);      // r * cos(u) * sin(v)

        vertices.push_back(glm::vec3(x, z, y));


        // Generowanie indeksów dla siatki
        auto addIndices = [&](unsigned int k1, unsigned int k2, unsigned int k3) { 
            indices.push_back(k1); 
            indices.push_back(k2); 
            indices.push_back(k2); 
            indices.push_back(k3); 
        };

        unsigned int k1, k2;

        // Pierwszy stack - trójkąty
        k1 = 0; k2 = 1;
        for (unsigned int j = 0; j < sectorCount - 1; j++, k2++) {
            addIndices(k1, k2, k2 + 1);
        }
        addIndices(0, sectorCount, 1);

        // Stacki 2...n-1 - prostokąty
        for (unsigned int i = 1; i < stackCount - 1; i++) {
            k1 = (i-1) * sectorCount + 1;
            k2 = k1 + sectorCount;
            
            // iterujemy parami wierzchołków
            for (unsigned int j = 0; j < sectorCount - 1; j++, k1++, k2++) {
                addIndices(k1, k2, k2 + 1);
            }

            addIndices(k1, k2, k2 + 1 - sectorCount);
        }

        // Ostatni stack - trójkąty
        k1 = vertices.size() - 1 - sectorCount; k2 = vertices.size() - 1;
        for (unsigned int j = 0; j < sectorCount - 1; j++, k1++) {
            addIndices(k2, k1, k1 + 1);
        }
        addIndices(k2, k1, k1 + 1 - sectorCount);
    }
public:
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    float rotation = 0.0;
    float scaleFactor = 1.0;
};

class Camera {
public:
    glm::vec3 position;                         // Pozycja kamery

    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    
    bool ortho = false;

    float Yaw  = 0.0f, 
          verticalAngle    = 0.0f,
          FovChangeSpeed   = 22.5f,
          Fov,
          moveModifier     = 4.0f,
          rotationModifier = 1.0f;

    Camera(glm::vec3 const& startPosition, float hAngle = 0.0f, float vAngle = 0.0f)
        : position(startPosition), 
          Yaw( glm::radians(hAngle) ), verticalAngle( glm::radians(vAngle) ) {
            Fov = initialFov;
            calculateVectors();
        }

    glm::mat4 getViewMatrix() const {
        glm::vec3 direction = glm::normalize(front); // Zapewnia stabilność kierunku
        glm::vec3 target = position + direction * 1000.0f; // Skalujemy kierunek, aby unikać problemów z dokładnością

        return glm::lookAt(position, target, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) const {
        if (!this->ortho)
            return glm::perspective(glm::radians(Fov), aspect, 0.1f, this->drawDistance);
        
        float zoomLevel = this->Fov / this->initialFov * 10.0f;
        float halfWidth = zoomLevel * aspect / 2.0f;
        float halfHeight = zoomLevel / 2.0f;

        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, this->drawDistance);
    }

    virtual glm::vec3 getPosition() const {
        return this->position;
    }

    virtual void setPosition(glm::vec3 const& pos) {
        this->position = pos;
    }

    float getDrawDistance(float aspect = 1.0f) const {
        if (!this->ortho) return this->drawDistance;

        float zoomLevel = this->Fov / this->initialFov * 10.0f;

        return aspect > 1.0f ? zoomLevel * aspect : zoomLevel;
    }

    void setDrawDistance(float drawDistance) {
        this->drawDistance = drawDistance;
    }

    virtual glm::vec3 getMoveVector(glm::vec3 const& offset, bool useUpVector = false) const {
        return glm::vec3(
            offset.x * right +
            offset.y * (useUpVector ? up : glm::vec3(0.0f, 1.0f, 0.0f)) +
            offset.z * front
        );
    }

    virtual void move(glm::vec3 const& moveVector) {
        position += moveVector * moveModifier * (this->ortho ? (Fov / initialFov) : 1.0f);
    }

    virtual void rotate(float horizontal, float vertical) {
        Yaw += horizontal * this->rotationModifier;

        float dh = 2*PI * (Yaw > 0 ? 1 : -1);
        while (glm::abs(Yaw) > PI) Yaw -= dh;
        
        verticalAngle += vertical * this->rotationModifier;
        verticalAngle  = glm::clamp(verticalAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f);

        calculateVectors();
    }
    
    float getInitialFov() const {
        return this->initialFov;
    }

    void setZoom(float Fov) {
        this->Fov = glm::clamp( Fov, minFov, maxFov );
    }

    void increaseZoom(float deltaTime) {
        Fov = std::max(Fov - deltaTime * FovChangeSpeed, minFov);
    }

    void decreaseZoom(float deltaTime) {
        Fov = std::min(Fov + deltaTime * FovChangeSpeed, maxFov);
    }

    void resetZoom() {
        Fov = initialFov;
    }

    void setZoomLimits(float min, float max) {
        this->minFov = min;
        this->maxFov = max;
    }
protected:
    float minFov = 15.0f,
          maxFov = 75.0f,
      initialFov = 55.0f;

    float drawDistance = 100.0f;

    virtual void calculateVectors() {
        front = glm::vec3(
            cos(verticalAngle) * sin(Yaw),
            sin(verticalAngle),
            cos(verticalAngle) * cos(Yaw)
        );

        right = glm::vec3(
            sin(Yaw - PI/2.0f),
            0,
            cos(Yaw - PI/2.0f)
        );

        up = glm::cross( right, front );
    }
};

class EarthCamera : public Camera {
public:
    float  latitudeAngle = 0.0f, 
          longitudeAngle = 0.0f,
               elevation = 300.0f;
    
    float earthCenterDistance = 637800.0;
public:
    EarthCamera(float lonAngle = 0.0f, float latAngle = 0.0f, float hAngle = 0.0f, float vAngle = 0.0f, float elevation = 300.0f)    
        : Camera(glm::vec3(0.0f), hAngle, vAngle) {
            if (glm::abs(latAngle) >  90.0f) throw std::invalid_argument( "Latitude angle of camera must be in [-90, 90] range.");
            if (glm::abs(lonAngle) > 180.0f) throw std::invalid_argument("Longitude angle of camera must be in [-180, 180] range.");

            this->latitudeAngle  = glm::radians( latAngle );
            this->longitudeAngle = glm::radians( lonAngle );
            this->elevation = elevation;

            this->moveModifier = 1000.0f;

            calculateVectors();
    }

    glm::vec3 getPosition() const override {
        return glm::vec3( glm::degrees(this->longitudeAngle), glm::degrees(this->latitudeAngle), this->elevation );
    }

    void setPosition(glm::vec3 const& pos) override {
        this->longitudeAngle = glm::radians(pos.x);
        this->latitudeAngle  = glm::radians(pos.y);
        this->elevation      = pos.z;

        calculateVectors();
    }

    glm::vec3 getMoveVector(glm::vec3 const& offset, bool useUpVector = false) const override {
        glm::vec3 moveVec = offset.x * right + offset.y * front;

        return moveVec;
    }
    
    void move(glm::vec3 const& moveVector) override {
        this->position += moveVector * this->moveModifier;

        double radius = glm::length(this->position);
        this->longitudeAngle = std::atan( (double)position.z / position.x );
        this->latitudeAngle  = std::asin( (double)position.y / radius );

        float dLat = PI * (this->latitudeAngle > 0 ? 1 : -1);
        while (glm::abs(this->latitudeAngle) > PI / 2.0f) this->latitudeAngle -= dLat;

        float dLon = PI * (this->longitudeAngle > 0 ? 1 : -1);
        while (glm::abs(this->longitudeAngle) > PI) this->longitudeAngle -= dLon;

        calculateVectors();
    }
    
    void rotate(float horizontal, float vertical) override {
        Camera::rotate(-horizontal, vertical);
    }
protected:
    void calculateVectors() override {
        position = glm::vec3(
            (earthCenterDistance + elevation) * cos(latitudeAngle) * cos(longitudeAngle),
            (earthCenterDistance + elevation) * sin(latitudeAngle),                                    
            (earthCenterDistance + elevation) * cos(latitudeAngle) * sin(longitudeAngle) 
        );

        up = glm::normalize(position);
        
        glm::vec3 baseDirection = glm::vec3(
            cos(verticalAngle) * cos(Yaw),
            sin(verticalAngle),
            cos(verticalAngle) * sin(Yaw)
        );

        glm::vec3 localFront = glm::cross(up, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::length(localFront) < 0.001f) { 
            localFront = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        localFront = glm::normalize(localFront);

        glm::vec3 localRight = glm::normalize(glm::cross(localFront, up));

        front = glm::normalize(
            baseDirection.x * localFront + 
            baseDirection.y * up +
            baseDirection.z * localRight
        );

        right = glm::normalize(glm::cross(front, up));
        
        up = glm::normalize(glm::cross(right, front));
    }
};

// ==========================================================================
// Window Main Loop Inits ...................................................
// ==========================================================================
class MyGame : public AGLWindow {
public:
    MyGame() {};
    MyGame(int _wd, int _ht, const char *name, int vers, int fullscr=0)
        : AGLWindow(_wd, _ht, name, vers, fullscr) {};
    virtual void KeyCB(int key, int scancode, int action, int mods) {
        AGLWindow::KeyCB(key,scancode, action, mods); // f-key full screen switch
        
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    }
    virtual void MousePosCB(double xp, double yp) {
        AGLWindow::MousePosCB(xp, yp);
        
        if (!this->enable_mouse) return;

        glfwSetCursorPos(win(), 0, 0);


        deltaMouseHorizontal = (-1.0f) * horizontal_mouse_speed * xp;
        deltaMouseVertical   = (-1.0f) *   vertical_mouse_speed * yp;
    }
    virtual void Resize( int _wd, int _ht ) {
        AGLWindow::Resize( _wd, _ht ); // domyślne zachowanie
        
        square_size = std::min(_wd, _ht);

        x_offset = (_wd - square_size) / 2;
        y_offset = (_ht - square_size) / 2;

        ViewportOne(x_offset, y_offset, square_size, square_size);
        if (resize_mode) ViewportOne(0, 0, wd, ht);
    }
    void InitSettings(short latMin = 0, short latMax = 0, short lonMin = 0, short lonMax = 0, std::string path = "./data/") {
        min = Coordinates( latMin, lonMin );
        max = Coordinates( latMax, lonMax );
        base_path = path;

        config = Config("game.config");
        
        debug                = config.getValue("debug");
        resize_mode          = config.getValue("resize_mode");
        raw_mouse_input      = config.getValue("raw_mouse_input");
        fps_counter          = config.getValue("fps_counter");
        lower_draw_distance  = config.getValue("lower_draw_distance");
        enable_mouse         = config.getValue("enable_mouse");

        if (raw_mouse_input && glfwRawMouseMotionSupported())
            glfwSetInputMode(win(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    void SetStartPosition(float latitude, float longitude, float elevation) {
        this->position = glm::vec3( longitude, latitude, elevation );
        this->startingPositionSet = true;
    }
    void MainLoop();
private:
    // settings
    float move     = 0.25;
    float rot_step = 0.6;
    float horizontal_mouse_speed = 0.3;
    float vertical_mouse_speed   = 0.4;
    float elevation_change_speed = 250.0;
    Coordinates min, max;
    std::string base_path;

    // config
    Config config;
    bool debug, resize_mode, raw_mouse_input, fps_counter, lower_draw_distance, enable_mouse;
    
    // game state
    glm::vec3 position;
    bool startingPositionSet = false;
    bool in3DMode = false;
    
    // lod management
    unsigned short lod = 5;
    bool autoLOD = true;
    float tryToIncreaseTimeout = 1.5f, 
          tryToDecreaseTimeout = 0.5f;
    FrameHistory fh = FrameHistory(10);
    
    // controls
    double previousTime = glfwGetTime(), currentTime, deltaTime, lastSecondTime = glfwGetTime();
    float deltaHorizontal = 0.0, deltaVertical = 0.0, deltaMouseHorizontal = 0.0, deltaMouseVertical = 0.0, deltaElevation = 0.0;
    glm::vec3 deltaPosition = glm::vec3(0.0f);

    // views
    int square_size = std::min(wd, ht);
    int x_offset = (wd - square_size) / 2, y_offset = (ht - square_size) / 2;
};

// ==========================================================================
void MyGame::MainLoop() {
    Resize(wd, ht);
    bool changeProjectionFlag = false;
    
    TileManager t;

    t.setLimits(min, max);
    
    t.setPath( this->base_path );
    t.loadAllTiles();

    if (!this->startingPositionSet)
        this->position = glm::vec3( t.getCenter().longitude.toFloat(), t.getCenter().latitude.toFloat(), t.getHeight(t.getCenter()) + 200.0f );
    else
        changeProjectionFlag = true;

    Camera mainCam(
        glm::vec3(
            this->position.x * t.getXCondensation(), 
            this->position.y, 
            1.0
        ),
        180.0f
    );
    mainCam.setZoomLimits(1.5f, 750.0f);
    mainCam.ortho = true;
    mainCam.rotationModifier = 0.0f;

    EarthCamera earthCam(
        this->position.x,
        this->position.y,
        90.0f,
        0.0f,
        this->position.z
    );
    earthCam.setDrawDistance(15000.0f);

    MySphere earthSurface;
    earthSurface.center = glm::vec3(0.0f, 0.0f, 0.0f);

    unsigned int frames = 0, framesSinceLodChange = 0;

    glm::mat4 viewMatrix, projectionMatrix;
    glm::vec3 moveVector;

    bool t_pressed = false, z_pressed = false, i_pressed = false, n_pressed = false, tab_pressed = false;
    float acc = 1.0, movementSpeed = 1.0f, lastLodChange = 0.0f;
    unsigned short new_lod = this->lod;
    Camera *currentCam = &mainCam;

    do {
        // =====================================================        Time handling
        currentTime = glfwGetTime();
        deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        frames++;
        framesSinceLodChange++;
        fh.update(deltaTime * 1000);

        
        // =====================================================        Inputs

        if (changeProjectionFlag) {
            this->in3DMode = !this->in3DMode;
            
            t.set3DProjection( this->in3DMode );

            currentCam = this->in3DMode ? &earthCam : &mainCam;

            if (this->in3DMode && !this->startingPositionSet) {
                this->position.z = t.getHeight( Coordinates(position.y, position.x) ) + 150.0f;
            }

            currentCam->setPosition( this->in3DMode ?
                this->position
                : 
                glm::vec3(
                    this->position.x * t.getXCondensation(),
                    this->position.y,
                    1.0f
                ) 
            );

            if (this->in3DMode) 
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);

            changeProjectionFlag = false;
        }

        if (this->autoLOD) {
            if (t.getLod() < 9 && fh.mean() > 100.0f && framesSinceLodChange > 20 && currentTime > lastLodChange + this->tryToDecreaseTimeout) {
                
                t.setLod( t.getLod() + 1 );
                lastLodChange = currentTime;
                framesSinceLodChange = 0;
            } 
            else if (t.getLod() > 1 && fh.mean() < 30.0f && framesSinceLodChange > 20 && currentTime > lastLodChange + this->tryToIncreaseTimeout) {
                
                t.setLod( t.getLod() - 1 );
                lastLodChange = currentTime;
                framesSinceLodChange = 0;
            }
        }
        else if (new_lod != this->lod) {
            this->lod = new_lod;
            t.setLod(this->lod);

            this->autoLOD = false;
        }

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        AGLErrors("main-loopbegin");
    
        // =====================================================        Drawing

        Resize(wd, ht);

        currentCam = this->in3DMode ? &earthCam : &mainCam;

        viewMatrix       = currentCam->getViewMatrix();
        projectionMatrix = currentCam->getProjectionMatrix( (resize_mode ? 1/aspect : 1.0f) );

        t.draw(viewMatrix, projectionMatrix, this->position, currentCam->getDrawDistance( (resize_mode ? 1/aspect : 1.0f) ) * (this->lower_draw_distance ? 0.3f : 1.0f) );
        
        earthSurface.draw(viewMatrix, projectionMatrix);

        AGLErrors("main-afterdraw");

        glfwSwapBuffers(win()); // =============================   Swap buffers
        glfwPollEvents();

        // =====================================================        Steering

        // FPS counter
        if ( fps_counter && currentTime - lastSecondTime >= 1.0 ) { // If last prinf() was more than 1 sec ago
            // printf and reset timer
            printf("%4d FPS  -  %5.1f mil. triangles  -  %8.4f ms/frame  -  LOD: %d%s\n", 
                        frames, 
                        t.getTriangleCount() / 1000000.0, 
                        fh.mean(), 
                        t.getLod(),
                        this->autoLOD ? std::string(" (Automatic)").c_str() : std::string("").c_str()
                );
            frames = 0;
            lastSecondTime += 1.0;
        }

        if ( glfwGetKey( win(), GLFW_KEY_W ) == GLFW_PRESS ) {                   // W -> Przesunięcie kamery w przód
            deltaPosition.y +=  1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_S ) == GLFW_PRESS ) {                   // S -> Przesunięcie kamery w tył
            deltaPosition.y += -1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_A ) == GLFW_PRESS ) {                   // A -> Przesunięcie kamery w lewo
            deltaPosition.x += -1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_D ) == GLFW_PRESS ) {                   // D -> Przesunięcie kamery w prawo
            deltaPosition.x +=  1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_SPACE ) == GLFW_PRESS ) {               // Space -> Przesunięcie kamery w górę
            deltaElevation +=  1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_C ) == GLFW_PRESS ) {          // Shift -> Przesunięcie kamery w dół
            deltaElevation += -1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_UP ) == GLFW_PRESS ) {                  // 🡑 -> Obrót kamery w górę
            deltaVertical   +=  1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_DOWN ) == GLFW_PRESS ) {                // 🡓 -> Obrót kamery w dół
            deltaVertical   += -1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_LEFT ) == GLFW_PRESS ) {                // 🡐 -> Obrót kamery w lewo
            deltaHorizontal +=  1.0;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_RIGHT ) == GLFW_PRESS ) {               // 🡒 -> Obrót kamery w prawo
            deltaHorizontal += -1.0;
        }
        if ( glfwGetKey( win(), GLFW_KEY_KP_ADD ) == GLFW_PRESS ) {              // + -> Przybliż widok
            currentCam->increaseZoom(deltaTime * acc);
        } 
        if ( glfwGetKey( win(), GLFW_KEY_KP_SUBTRACT ) == GLFW_PRESS ) {         // - -> Oddal widok
            currentCam->decreaseZoom(deltaTime * acc);
        }
        if ( glfwGetKey( win(), GLFW_KEY_KP_MULTIPLY ) == GLFW_PRESS ) {         // * -> Resetuj przybliżenie
            currentCam->resetZoom();
        } 
        if ( glfwGetKey( win(), GLFW_KEY_APOSTROPHE ) == GLFW_PRESS ) {                   // Q -> Pokaż tylko krawędzie przeszkód
            movementSpeed *= (1.0 + deltaTime);
        } 
        if ( glfwGetKey( win(), GLFW_KEY_BACKSLASH ) == GLFW_PRESS ) {                   // Q -> Pokaż tylko krawędzie przeszkód
            movementSpeed *= (1.0 - deltaTime);
        } 
        if ( glfwGetKey( win(), GLFW_KEY_SEMICOLON ) == GLFW_PRESS ) {                   // Q -> Pokaż tylko krawędzie przeszkód
            movementSpeed = 1.0f;
        } 
        if ( glfwGetKey( win(), GLFW_KEY_Q ) == GLFW_PRESS ) {                   // Q -> Pokaż tylko krawędzie przeszkód
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } 
        if ( glfwGetKey( win(), GLFW_KEY_E ) == GLFW_PRESS ) {                   // E -> Pokaż ściany przeszkód
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } 
        if ( glfwGetKey( win(), GLFW_KEY_TAB ) == GLFW_PRESS && !tab_pressed ) { // Tab -> ??
            tab_pressed = true;
            changeProjectionFlag = true;
        } else if (glfwGetKey( win(), GLFW_KEY_TAB ) == GLFW_RELEASE && tab_pressed) tab_pressed = false;
        if ( glfwGetKey( win(), GLFW_KEY_T ) == GLFW_PRESS && !t_pressed ) {     // T -> ??
            t_pressed = true;
        } else if (glfwGetKey( win(), GLFW_KEY_T ) == GLFW_RELEASE && t_pressed) t_pressed = false;
        if ( glfwGetKey( win(), GLFW_KEY_I ) == GLFW_PRESS && !i_pressed ) {     // I -> ??
            i_pressed = true;
        } else if (glfwGetKey( win(), GLFW_KEY_I ) == GLFW_RELEASE && i_pressed) i_pressed = false;
        
        // USER LOD
        if ( glfwGetKey( win(), GLFW_KEY_1 ) == GLFW_PRESS ) {
            new_lod = 1;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_2 ) == GLFW_PRESS ) {
            new_lod = 2;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_3 ) == GLFW_PRESS ) {
            new_lod = 3;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_4 ) == GLFW_PRESS ) {
            new_lod = 4;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_5 ) == GLFW_PRESS ) {
            new_lod = 5;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_6 ) == GLFW_PRESS ) {
            new_lod = 6;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_7 ) == GLFW_PRESS ) {
            new_lod = 7;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_8 ) == GLFW_PRESS ) {
            new_lod = 8;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_9 ) == GLFW_PRESS ) {
            new_lod = 9;
            this->autoLOD = false;
        }
        if ( glfwGetKey( win(), GLFW_KEY_0 ) == GLFW_PRESS ) {
            this->autoLOD = true;
        }

        // SLOW ON CTRL
        if ( glfwGetKey( win(), GLFW_KEY_LEFT_CONTROL ) == GLFW_PRESS ) {
            acc = 0.3f;
        } else acc = 1.0f;
        // FAST ON ALT
        if ( glfwGetKey( win(), GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS ) {
            acc = 3.0f;
        } else if (acc == 3.0f) acc = 1.0f;

        // DEBUG
        if ( glfwGetKey( win(), GLFW_KEY_Z ) == GLFW_PRESS && !z_pressed ) {     // Z -> ??
            z_pressed = true;
        } else if (glfwGetKey( win(), GLFW_KEY_Z ) == GLFW_RELEASE && z_pressed) z_pressed = false;
        if ( glfwGetKey( win(), GLFW_KEY_N ) == GLFW_PRESS && !n_pressed ) {     // N -> ??
            n_pressed = true;
        } else if (glfwGetKey( win(), GLFW_KEY_N ) == GLFW_RELEASE && n_pressed) n_pressed = false;

        if (!this->in3DMode)
            deltaPosition   += glm::vec3( -deltaMouseHorizontal, deltaMouseVertical, 0.0f );
        deltaPosition   *= move     * deltaTime * acc * movementSpeed;
        deltaHorizontal *= rot_step; deltaHorizontal += deltaMouseHorizontal; deltaHorizontal *= deltaTime;
        deltaVertical   *= rot_step; deltaVertical   += deltaMouseVertical  ; deltaVertical   *= deltaTime;
        deltaElevation  *= elevation_change_speed * deltaTime * acc;
        
        if (debug && i_pressed) printf("Camera angles:  %f / %f\n", currentCam->Yaw / (2*PI) * 360.0, currentCam->verticalAngle / (2*PI) * 360.0);
        if (debug && t_pressed) printf("Lon/Lat/Elev: %f / %f / %f\n", this->position.x, this->position.y, this->position.z);

        if (debug && n_pressed) printf("Before move vector: %f / %f    Diff: %f\n", deltaPosition.x, deltaPosition.y, glm::abs(deltaPosition.x - deltaPosition.y));
        deltaPosition = currentCam->getMoveVector(deltaPosition);
        if (debug && n_pressed) printf("After  move vector: %f / %f    Diff: %f\n", deltaPosition.x, deltaPosition.y, glm::abs(deltaPosition.x - deltaPosition.y));

        if (debug && z_pressed) printf("Before position: %f / %f / %f\n", currentCam->position.x, currentCam->position.y, currentCam->position.z);
        currentCam->move( deltaPosition );
        if (debug && z_pressed) printf("After  position: %f / %f / %f\n", currentCam->position.x, currentCam->position.y, currentCam->position.z);
        currentCam->rotate( deltaHorizontal, deltaVertical );

        if (this->in3DMode && deltaElevation != 0.0f)
            currentCam->setPosition( currentCam->getPosition() + glm::vec3(0.0f, 0.0f, deltaElevation) );

        this->position.x = (this->in3DMode ? currentCam->getPosition().x : currentCam->getPosition().x / t.getXCondensation());
        this->position.y = currentCam->getPosition().y;
        this->position.z = earthCam.getPosition().z;

        // reset values
        deltaPosition = glm::vec3(0.0f);
        deltaHorizontal = 0.0f;
        deltaVertical   = 0.0f;
        deltaMouseHorizontal = 0.0f;
        deltaMouseVertical   = 0.0f;
        deltaElevation       = 0.0f;

    } while( glfwGetKey(win(), GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
             glfwWindowShouldClose(win()) == 0 );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory> [-lon <min> <max>] [-lat <min> <max>] [-start <longitude> <latitude> <elevation>]\n";
        return 0;
    }

    // Pierwszy argument to katalog
    std::string directory = argv[1];

    short latMin = 0, latMax = 0, lonMin = 0, lonMax = 0;
    bool startParameters = false;
    float latStart = 0.0f, lonStart = 0.0f, elevStart = 0.0f;

    // Przetwarzanie pozostałych argumentów
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-lon" && i + 2 < argc) {
            lonMin = std::stoi(argv[++i]);
            lonMax = std::stoi(argv[++i]);

            if (lonMin >= lonMax) {
                std::cerr << arg << ": Minimum longitude must be lower than maximum.\n";
                return 0;
            }
        } else if (arg == "-lat" && i + 2 < argc) {
            latMin = std::stoi(argv[++i]);
            latMax = std::stoi(argv[++i]);

            if (latMin >= latMax) {
                std::cerr << arg << ": Minimum latitude must be lower than maximum.\n";
                return 0;
            }
        } else if (arg == "-start" && i + 3 < argc) {
            lonStart  = std::stof(argv[++i]);
            latStart  = std::stof(argv[++i]);
            elevStart = std::stoi(argv[++i]);

            if (glm::abs(lonStart) > 180.0f) {
                std::cerr << arg << ": Longitude must be in range [-180, 180].\n";
                return 0;
            }
            if (glm::abs(latStart) > 90.0f) {
                std::cerr << arg << ": Latitude must be in range [-90, 90].\n";
                return 0;
            }

            startParameters = true;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            return 0;
        }
    }

    MyGame win;
    win.Init(1600, 900,"AGL3 Terrain",0,33);
    win.InitSettings(latMin, latMax, lonMin, lonMax, directory);
    if (startParameters) win.SetStartPosition(latStart, lonStart, elevStart);
    win.MainLoop();
    return 0;
}