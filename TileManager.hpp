// ==========================================================================
// TileManager: class definitions
//
// Michał Chawar
// ==========================================================================
// Hemisphere
// Coordinate
// Coordinates
// 
// Tile
// TileManager
//===========================================================================

#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>

#include <string>
#include <cmath>

#include <vector>
#include <unordered_map>

#include <stdexcept>
#include <cctype>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <AGL3Drawable.hpp>


// ----------------------------------------
//  
//      COORDINATES utilities
//  
// ----------------------------------------

enum class Hemisphere {
    North,
    South,
    East,
    West
};

class Coordinate {
private:
    uint16_t degrees, minutes, seconds;
    Hemisphere hem;
    std::string partial_tile_string;
public:
    Coordinate() {}
    Coordinate(unsigned short degrees, unsigned short minutes, unsigned short seconds, Hemisphere hem) : 
        degrees(degrees), minutes(minutes), seconds(seconds), hem(hem) 
    {
        calculatePartialTileString();
    }
    Coordinate(unsigned short degrees, Hemisphere hem) : Coordinate(degrees, 0, 0, hem) {}
    Coordinate(float val, Hemisphere hem) : Coordinate(
        (short) val, 
        (short)((val - (short)val) * 60.0f),
        (short)((val - (short)val) * 3600.0f) % 60,
        hem
    ) {}
public:
    void offset(short degrees, short minutes, short seconds) {
        this->degrees += degrees;
        this->minutes += minutes;
        this->seconds += seconds;

        calculatePartialTileString();
    }

    float toFloat() const {
        float result = degrees + minutes / 60.0f + seconds / 3600.0f;

        if (hem == Hemisphere::South || hem == Hemisphere::West) result = -result;

        return result;
    }
             short getDegreesSigned() const { return (hem == Hemisphere::South || hem == Hemisphere::West ? -degrees : degrees); }
    unsigned short getDegrees() const { return degrees; }
    unsigned short getMinutes() const { return minutes; }
    unsigned short getSeconds() const { return seconds; }
    
    void setDegrees(short degrees) {
        if (degrees == this->getDegreesSigned()) {
            return;
        }

         if ((this->hem == Hemisphere::North || this->hem == Hemisphere::South) && abs(degrees) > 90)
            throw std::invalid_argument("Degrees for latitude have to be in range (-90, 90)");
         if ((this->hem == Hemisphere::West  || this->hem == Hemisphere::East) && abs(degrees)  > 180)
            throw std::invalid_argument("Degrees for longitude have to be in range (-180, 180)");

        this->degrees = abs(degrees);
        this->hem = (this->hem == Hemisphere::North || this->hem == Hemisphere::South) ? 
                    (degrees >= 0) ? Hemisphere::North : Hemisphere::South             :
                    (degrees >= 0) ? Hemisphere::East  : Hemisphere::West;
        
        calculatePartialTileString();
    }
    void setMinutes(unsigned short minutes) { 
        if (minutes > 60)
            throw std::invalid_argument("Minutes for coordinate have to be in range [0, 60]");

        this->minutes = minutes;
    }
    void setSeconds(unsigned short seconds) { 
        if (seconds > 60)
            throw std::invalid_argument("Seconds for coordinate have to be in range [0, 60]");

        this->seconds = seconds;
    }
    
    Hemisphere  getHemisphere() const { return hem; }

    std::string getPartialTileString() const {
        return this->partial_tile_string;
    }
    std::string toString() const {
        std::string result = "";

        switch (hem) {
            case Hemisphere::North:
                result += "N";
                break;
            case Hemisphere::South:
                result += "S";
                break;
            case Hemisphere::East:
                result += "E";
                break;
            case Hemisphere::West:
                result += "W";
                break;
        }

        result += " " + std::to_string(degrees) + "° " + std::to_string(minutes) + "' " + std::to_string(seconds) + "\"";
        return result;
    }
private:
    void calculatePartialTileString() {
        std::string result = "";
        unsigned short digits;
        
        short tile_origin_degrees = degrees;

        // if on inverted hemisphere and not on border then make correction
        if ( (hem == Hemisphere::South || hem == Hemisphere::West) && !(minutes == 0 && seconds == 0) )
            tile_origin_degrees++;

        std::string deg = std::to_string(tile_origin_degrees);

        switch (hem) {
            case Hemisphere::North:
                result += "N";
                digits = 2;
                break;
            case Hemisphere::South:
                result += "S";
                digits = 2;
                break;
            case Hemisphere::East:
                result += "E";
                digits = 3;
                break;
            case Hemisphere::West:
                result += "W";
                digits = 3;
                break;
        }

        deg.insert(0, digits - deg.length(), '0');
        result += deg;

        this->partial_tile_string = result;
    }
};

class Coordinates {
public:
    Coordinate latitude, longitude;
public:
    Coordinates() {}
    Coordinates(Coordinate latitude, Coordinate longitude) {
        if (!(latitude.getHemisphere() == Hemisphere::North || latitude.getHemisphere() == Hemisphere::South))
            throw std::invalid_argument("Invalid hemisphere for latitude");

        if (!(longitude.getHemisphere() == Hemisphere::East || longitude.getHemisphere() == Hemisphere::West))
            throw std::invalid_argument("Invalid hemisphere for longitude");
        
        this->latitude = latitude;
        this->longitude = longitude;
    }
    Coordinates(unsigned short degrees_lat, Hemisphere hem_lat, unsigned short degrees_lon, Hemisphere hem_lon) 
        : Coordinates( Coordinate(degrees_lat, hem_lat), Coordinate(degrees_lon, hem_lon) ) {}
    Coordinates(short degrees_lat, short degrees_lon) 
        : Coordinates( 
            std::abs( degrees_lat ), 
            degrees_lat >= 0 ? Hemisphere::North : Hemisphere::South,
            std::abs( degrees_lon ), 
            degrees_lon >= 0 ? Hemisphere::East : Hemisphere::West
        ) {}
    Coordinates(float lat, float lon) 
        : Coordinates(
            Coordinate( 
                std::abs( lat ), 
                lat >= 0 ? Hemisphere::North : Hemisphere::South
            ),
            Coordinate(
                std::abs( lon ),
                lon >= 0 ? Hemisphere::East : Hemisphere::West
            )
        ) {}
public:
    std::string getTileString() const {
        return latitude.getPartialTileString() + longitude.getPartialTileString();
    }
    std::string toString() const {
        return latitude.toString() + " / " + longitude.toString();
    }
};


// ----------------------------------------
//  
//      TILE class
//  
// ----------------------------------------

class Tile : public AGLDrawable {
public:
    Tile() : AGLDrawable(0) {
        // Inicjalizuj wysokości brakującymi danymi (-1000, wartość dowolna zgodna z aplikacją)
        for (int i = 0; i < 1201; ++i) {
            for (int j = 0; j < 1201; ++j) {
                height_map[i][j] = NO_DATA; // Brak danych domyślnie
            }
        }
    }
    Tile(Coordinates tile_origin, std::vector<glm::vec2> &vert, GLuint v2d, GLuint v3d, GLuint f, GLuint ebo) : Tile() {
        this->vertices = &vert;
        this->load(tile_origin);

        this->v2d = v2d;
        this->v3d = v3d;
        this->f   = f;
        this->EBO = ebo;

        setShaders();
        setBuffers();
    }
public:
    void setShaders() {
        pId = glCreateProgram();
        glAttachShader(pId, this->is3D ? v3d : v2d);
        glAttachShader(pId, f);
        CompileLink(pId, "Linking",  3);
    }
    void set3DProjection(bool isProjection3D) {
        if (this->is3D == isProjection3D) return;

        glDetachShader(pId, this->is3D ? v3d : v2d);
        this->is3D = isProjection3D;
        glAttachShader(pId, this->is3D ? v3d : v2d);

        CompileLink(pId, "Linking",  3);
    }
    void setBuffers() {
        bindBuffers();
        
        glBufferData(GL_ARRAY_BUFFER, 1201 * 1201 * sizeof(float), height_map, GL_STATIC_DRAW );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // attribute 0, must match the layout in the shader.
            1,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,//24,             // stride
            (void*)0            // array buffer offset
        );
    }
    void draw(glm::mat4 const& view, glm::mat4 const& projection, unsigned int indices_size, unsigned int offset) {
        bindProgram();
        bindBuffers();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        if (!this->is3D) glUniform1f(4, this->x_condensation);
        glUniform1i(5, this->origin.latitude .getDegreesSigned());
        glUniform1i(6, this->origin.longitude.getDegreesSigned());

        glUniformMatrix4fv(14, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(15, 1, GL_FALSE, glm::value_ptr(projection));

        glDrawElements(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, (void*)(offset * sizeof(unsigned int)));
    }
public:
    short getHeight(int i, int j) const {
        if (i < 0 || i >= 1201 || j < 0 || j >= 1201) {
            throw std::out_of_range("Indeks poza zakresem mapy wysokości.");
        }

        return height_map[i][j];
    }
    short getHeight(Coordinates const& coords) {
        if (coords.latitude.getMinutes() < 0 || coords.latitude.getMinutes() > 60 || (coords.latitude.getMinutes() == 60 && coords.latitude.getSeconds() != 0) || coords.latitude.getSeconds() < 0 || coords.latitude.getSeconds() > 59) {
            throw std::out_of_range("Latitude out of bounds for minutes/seconds: " + std::to_string(coords.latitude.getMinutes()) + " / " + std::to_string(coords.latitude.getSeconds()) + ".");
        }
        if (coords.longitude.getMinutes() < 0 || coords.longitude.getMinutes() > 60 || (coords.longitude.getMinutes() == 60 && coords.longitude.getSeconds() != 0) || coords.longitude.getSeconds() < 0 || coords.longitude.getSeconds() > 59) {
            throw std::out_of_range("Longitude out of bounds for minutes/seconds: " + std::to_string(coords.longitude.getMinutes()) + " / " + std::to_string(coords.longitude.getSeconds()) + ".");
        }

        int x = coords.longitude.getMinutes() * 20 + coords.longitude.getSeconds() / 3;
        int y = coords.latitude .getMinutes() * 20 + coords.latitude .getSeconds() / 3;
        
        if (coords.longitude.getHemisphere() == Hemisphere::South)
            y = 1200 - y;
        if (coords.latitude .getHemisphere() == Hemisphere::West)
            x = 1200 - x;

        return height_map[y][x];
    }
    void  setXCondensation(float x_condensation) {
        this->x_condensation = x_condensation;
    }
    static Coordinates decodeTileNameString(std::string tileName) {
        if (tileName.length() != 7) throw std::invalid_argument("Tile name must consist of exactly 7 characters: " + tileName);
        
        unsigned short deg_lat, deg_lon;
        Hemisphere h_lat, h_lon;

        if      (tileName[0] == 'N') h_lat = Hemisphere::North;
        else if (tileName[0] == 'S') h_lat = Hemisphere::South;
        else throw std::invalid_argument("Invalid hemisphere for latitude in tile name: " + tileName);
        
        if      (tileName[3] == 'E') h_lon = Hemisphere::East;
        else if (tileName[3] == 'W') h_lon = Hemisphere::West;
        else throw std::invalid_argument("Invalid hemisphere for longitude in tile name: " + tileName);

        if ( !(std::isdigit(tileName[1]) && std::isdigit(tileName[2]) && std::isdigit(tileName[4]) && std::isdigit(tileName[5]) && std::isdigit(tileName[6])) )
            throw std::invalid_argument("Invalid digits in tile name: " + tileName);
        
        deg_lat = (tileName[1] - '0') * 10  + (tileName[2] - '0');
        deg_lon = (tileName[4] - '0') * 100 + (tileName[5] - '0') * 10 + (tileName[6] - '0');

        return Coordinates(deg_lat, h_lat, deg_lon, h_lon);
    }
public:
    const static short NO_DATA = -1000;
    static std::string path;
    Coordinates origin;
private:
    float height_map[1201][1201];

    GLuint v2d, v3d, f;
    bool is3D = false;
    
    GLuint VBO, EBO;
    std::vector<glm::vec2>* vertices;
    int height = 1201, width = 1201;
    float x_condensation = 1.0f;

    void load(Coordinates const& origin) {
        std::string file_name = path + origin.getTileString() + ".hgt";

        // Otwieranie pliku w trybie binarnym
        std::ifstream file(file_name, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Nie można otworzyć pliku: " + file_name);
        }

        this->origin = Coordinates( origin.latitude.getDegreesSigned(), origin.longitude.getDegreesSigned() );

        // Ładowanie danych do height_map
        for (short i = 0; i < 1201; ++i) {
            for (short j = 0; j < 1201; ++j) {
                unsigned char buffer[2];
                if (!file.read(reinterpret_cast<char*>(buffer), 2)) {
                    throw std::runtime_error("Błąd odczytu danych z pliku: " + file_name);
                }

                // Zamiana kolejności bajtów
                short value = static_cast<short>((buffer[0] << 8) | buffer[1]);

                // Sprawdzenie zakresu wartości
                if (value < -500 || value > 9000) {
                    height_map[1200 - i][j] = NO_DATA; // Brak danych
                } else {
                    height_map[1200 - i][j] = value;
                }
            }
        }

        file.close();
        // std::cout << "Pomyślnie załadowano dane z pliku: " << file_name << "\n";
    }
};


// ----------------------------------------
//  
//      TILE MANAGER class
//  
// ----------------------------------------

class TileManager : public AGLDrawable {
private:
    std::unordered_map<std::string, std::unique_ptr<Tile>> tiles;
    std::vector<std::string> loaded_keys;
    
    std::vector<glm::vec2> vertices;
    std::vector<unsigned int> indices[10];
    unsigned int ind_offsets[10];
    unsigned short user_lod = 5;

    Coordinates limit_sw = Coordinates((short)0, 0),
                limit_ne = Coordinates((short)0, 0);
    bool  unbounded_lat = true,
          unbounded_lon = true;
    
    GLuint v2d, v3d, f;
    GLuint EBO;
    bool is3D = false;

    unsigned int tilesRendered = 0;
    
    const static std::string NOT_LOADED;
    const float earthRadius = 637800.0;
public:
    TileManager() {
        // Generowanie wierzchołków
        int width = 1201, height = 1201;
        int i0, i1, i2, i3;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                vertices.emplace_back((float)i / (height - 1), (float)j / (width - 1));
            }
        }

        // Generowanie indeksów
        for (int i = 0; i < height - 1; i++) {
            for (int j = 0; j < width - 1; j++) {
                for (int lod = 1; lod <= 9; lod++) {
                    if (i % lod || j % lod) continue;
                    
                    i0 = i * width + j;
                    i1 = std::min(i0 + lod,         (i + 1) * width - 1);
                    i2 = std::min(i0 + width * lod, width * (height - 1) + j);
                    i3 = std::min(i2 + lod,         ((i2 / width) + 1) * width - 1);

                    indices[lod].push_back(i0);
                    indices[lod].push_back(i1);
                    indices[lod].push_back(i2);

                    indices[lod].push_back(i1);
                    indices[lod].push_back(i3);
                    indices[lod].push_back(i2);
                }
            }
        }

        setShaders();
        setBuffers();
    }

    void setShaders() {
        v2d = glCreateShader(GL_VERTEX_SHADER);
        v3d = glCreateShader(GL_VERTEX_SHADER);
        f   = glCreateShader(GL_FRAGMENT_SHADER);

        getShaderSource(v2d, "shaders/tile.vs");
        getShaderSource(v3d, "shaders/tile3d.vs");
        getShaderSource(f,   "shaders/tile.fs");

        GLint Result = GL_FALSE;
        if ( Result = CompileLink(v2d, "VS-2D") ) {
            if ( Result = CompileLink(v3d, "VS-3D") ) {
                CompileLink(f, "FS");
            }
        }
    }

    void setBuffers() {
        glGenBuffers(1, &EBO);

        std::vector<unsigned int> ind;
        unsigned int offset = 0;

        for (int i = 1; i <= 9; i++) {
            this->ind_offsets[i] = offset;

            for (int j = 0; j < this->indices[i].size(); j++) {
                ind.push_back(this->indices[i][j]);
            }

            offset += this->indices[i].size();
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ind.size() * sizeof(unsigned int), ind.data(), GL_DYNAMIC_DRAW);
    }

    // Funkcja ładowania kafli
    std::string loadTile(Coordinates coords) {
        std::string loaded = loadTileInternal(coords);

        if (loaded == this->NOT_LOADED && coords.latitude.getMinutes() == 0 && coords.latitude.getSeconds() == 0) {
            coords.latitude.offset(-1, 60, 0);
            loaded = loadTileInternal(coords);

            if (loaded == this->NOT_LOADED)
                coords.latitude.offset(1, -60, 0);
        }

        if (loaded == this->NOT_LOADED && coords.longitude.getMinutes() == 0 && coords.longitude.getSeconds() == 0) {
            coords.longitude.offset(-1, 60, 0);
            loaded = loadTileInternal(coords);

            if (loaded == this->NOT_LOADED)
                coords.longitude.offset(1, -60, 0);
        }

        if (loaded == this->NOT_LOADED && coords.latitude.getMinutes() == 0 && coords.latitude.getSeconds() == 0 && coords.longitude.getMinutes() == 0 && coords.longitude.getSeconds() == 0) {
            coords.latitude .offset(-1, 60, 0);
            coords.longitude.offset(-1, 60, 0);
            loaded = loadTileInternal(coords);

            if (loaded == this->NOT_LOADED) {
                coords.latitude .offset(1, -60, 0);
                coords.longitude.offset(1, -60, 0);
            }
        }

        return loaded;
    }
    void loadAllTiles() {
        if (this->unbounded_lat || this->unbounded_lon) {
            int toLoad = std::distance(std::filesystem::directory_iterator(Tile::path), std::filesystem::directory_iterator{});
            int loaded = 0;

            for (const auto & entry : std::filesystem::directory_iterator(Tile::path)) {
                try {
                    Coordinates orig = Tile::decodeTileNameString( entry.path().stem().string() );
                    
                    if (
                        (this->unbounded_lat || (orig.latitude .getDegreesSigned() >= this->limit_sw.latitude .getDegreesSigned() 
                                              && orig.latitude .getDegreesSigned() <  this->limit_ne.latitude .getDegreesSigned()))
                     && (this->unbounded_lon || (orig.longitude.getDegreesSigned() >= this->limit_sw.longitude.getDegreesSigned() 
                                              && orig.longitude.getDegreesSigned() <  this->limit_ne.longitude.getDegreesSigned()))
                        ) 
                    {
                        this->loadTileInternal( orig );
                        printf("Ładowanie....                    %d / %d\n", loaded++, toLoad);
                    }
                }
                catch (const std::exception& e) {
                    continue;
                }
            }
        } else {
            int toLoad = (this->limit_ne.latitude.getDegrees() - this->limit_sw.latitude.getDegrees()) * (this->limit_ne.longitude.getDegrees() - this->limit_sw.longitude.getDegrees());
            int loaded = 0;

            for (short lat = this->limit_sw.latitude.getDegreesSigned(); lat < this->limit_ne.latitude.getDegreesSigned(); lat++) {
                for (short lon = this->limit_sw.longitude.getDegreesSigned(); lon < this->limit_ne.longitude.getDegreesSigned(); lon++) {
                    this->loadTileInternal( Coordinates( lat, lon ) );

                    printf("Ładowanie....                    %d / %d\n", loaded++, toLoad);
                }
            }
        }

        this->propagateXCondensation();
    }

    // Funkcja ładowania koordynatów
    short getHeight(Coordinates const& coords) {
        std::string key = this->loadTile(coords);

        return key != this->NOT_LOADED ? 
            tiles[ key ]->getHeight(coords) :
            Tile::NO_DATA;
    }
    uint64_t getTriangleCount() {
        return (uint64_t)this->tilesRendered * this->indices[ this->user_lod ].size();
    }
    void draw(glm::mat4 const& view, glm::mat4 const& projection, glm::vec3 const& position, float drawDistance = 10000.0f) {
        drawDistance *= 1.2f;

        double radius = glm::length(position);
        double posLon = position.x;
        double posLat = position.y;

        glm::vec3 worldPos = this->transformToWorldPosition3D( position );

        glm::vec2 targetCords;
        Coordinates target;

        this->tilesRendered = 0;

        for (int i = 0; i < this->loaded_keys.size(); i++) {
            target = this->tiles[ this->loaded_keys[i] ]->origin;

            targetCords.x = glm::clamp( posLon, target.longitude.getDegreesSigned() + 0.0, target.longitude.getDegreesSigned() + 1.0 );
            targetCords.y = glm::clamp( posLat, target.latitude .getDegreesSigned() + 0.0, target.latitude .getDegreesSigned() + 1.0 );

            if ((this->is3D && 
                 glm::length( this->transformToWorldPosition3D(glm::vec3(targetCords, 0.0f)) - worldPos ) <= drawDistance )
             || (!this->is3D &&
                 glm::length( targetCords - glm::vec2(position.x, position.y) ) <= drawDistance )) 
            {
                this->tiles[ this->loaded_keys[i] ]->draw(view, projection, indices[ this->user_lod ].size(), this->ind_offsets[ this->user_lod ]);
                this->tilesRendered++;
            }
        }
    }
public:
    float getXCondensation() const {
        return glm::cos( this->getCenter().latitude.getDegrees() / 90.0f );
    }
    void setLod(unsigned short lod) {
        if (lod < 1 || lod > 9) return;
        if (lod == this->user_lod) return;

        this->user_lod = lod;
    }
    unsigned short getLod() const {
        return this->user_lod;
    }
    void setPath(std::string path) {
        Tile::path = path;
    }
    void setLimits(Coordinates south_west, Coordinates north_east) {
        this->limit_sw = south_west;
        this->limit_ne = north_east;

        this->unbounded_lon = this->limit_ne.longitude.getDegreesSigned() <= this->limit_sw.longitude.getDegreesSigned();
        this->unbounded_lat = this->limit_ne.latitude .getDegreesSigned() <= this->limit_sw.latitude .getDegreesSigned();
    }
    void set3DProjection(bool isProjection3D) {
        if (this->is3D == isProjection3D) return;

        this->is3D = isProjection3D;

        for (int i = 0; i < loaded_keys.size(); i++) {
            tiles[ loaded_keys[i] ]->set3DProjection( this->is3D );
        }
    }
    Coordinates getCenter() const {
        float lat = this->limit_sw.latitude .getDegreesSigned() + (this->limit_ne.latitude .getDegreesSigned() - this->limit_sw.latitude .getDegreesSigned()) / 2.0f,
              lon = this->limit_sw.longitude.getDegreesSigned() + (this->limit_ne.longitude.getDegreesSigned() - this->limit_sw.longitude.getDegreesSigned()) / 2.0f;

        return Coordinates( lat, lon );
    }
private:
    std::string loadTileInternal(Coordinates const& origin) {
        std::string key = origin.getTileString();

        if (tiles.find(key) == tiles.end()) {
            // Kafel nie jest załadowany, ładowanie z pliku
            try {
                auto tile = std::make_unique<Tile>(origin, vertices, v2d, v3d, f, EBO);
                tiles[key] = std::move( tile );
                
                this->loaded_keys.push_back(key);

                if (this->unbounded_lat || this->unbounded_lon) {
                    // Pierwszy załadowany, ustaw wirtualne granice
                    if (this->loaded_keys.size() == 1) {
                        short min_lat = origin.latitude .getDegreesSigned(),
                              min_lon = origin.longitude.getDegreesSigned(),
                              max_lat = origin.latitude. getDegreesSigned() + 1,
                              max_lon = origin.longitude.getDegreesSigned() + 1;

                        if (!this->unbounded_lat) {
                            min_lat = this->limit_sw.latitude .getDegreesSigned();
                            max_lat = this->limit_ne.latitude .getDegreesSigned();
                        }
                        if (!this->unbounded_lon) {
                            min_lon = this->limit_sw.longitude.getDegreesSigned();
                            max_lon = this->limit_ne.longitude.getDegreesSigned();
                        }

                        this->limit_sw = Coordinates( min_lat, min_lon );
                        this->limit_ne = Coordinates( max_lat, max_lon );
                    }
                    // Kolejny załadowany, zaktualizuj wirtualne granice
                    else {
                        short min_lat = std::min( this->limit_sw.latitude .getDegreesSigned(),         origin.latitude .getDegreesSigned()      ),
                              min_lon = std::min( this->limit_sw.longitude.getDegreesSigned(),         origin.longitude.getDegreesSigned()      ),
                              max_lat = std::max( this->limit_ne.latitude .getDegreesSigned(), (short)(origin.latitude .getDegreesSigned() + 1) ),
                              max_lon = std::max( this->limit_ne.longitude.getDegreesSigned(), (short)(origin.longitude.getDegreesSigned() + 1) );

                        if (!this->unbounded_lat) {
                            min_lat = this->limit_sw.latitude .getDegreesSigned();
                            max_lat = this->limit_ne.latitude .getDegreesSigned();
                        }
                        if (!this->unbounded_lon) {
                            min_lon = this->limit_sw.longitude.getDegreesSigned();
                            max_lon = this->limit_ne.longitude.getDegreesSigned();
                        }

                        this->limit_sw.latitude .setDegrees( min_lat );
                        this->limit_sw.longitude.setDegrees( min_lon );
                        this->limit_ne.latitude .setDegrees( max_lat );
                        this->limit_ne.longitude.setDegrees( max_lon );
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Błąd wczytywania kafla (" + key + "): " + e.what() << "\n";
                
                return this->NOT_LOADED;
            }
        }

        return key;
    }

    void propagateXCondensation() {
        float x_cond = this->getXCondensation();

        for (int i = 0; i < this->loaded_keys.size(); i++) {
            this->tiles[ this->loaded_keys[i] ]->setXCondensation( x_cond );
        }
    }
    glm::vec3 transformToWorldPosition3D( glm::vec3 const& pos ) {
        double latitude  = glm::radians(pos.y);
        double longitude = glm::radians(pos.x);

        return glm::vec3(
            (earthRadius + pos.z) * cos(latitude) * cos(longitude),
            (earthRadius + pos.z) * sin(latitude),
            (earthRadius + pos.z) * cos(latitude) * sin(longitude)
        );
    }
};

std::string Tile::path = "./data/";
const std::string TileManager::NOT_LOADED = "tile_not_loaded";