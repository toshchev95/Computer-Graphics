
#include <SOIL.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <ctime>
#include "Utility.h"
#include "Physics.h"

using namespace std;

namespace Instancing {
  const uint GRASS_INSTANCES = 10000; // Количество травинок

  static GL::Camera camera; // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                            //Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                            //Вы можете просто пользоваться этим классом для расчёта указанных матриц.

  template <typename T>
  T rnd(double number = 0.03f, int mod = 0) {
      if (number > 0)
        return (T) number * (rand() % mod);
      else
        return 0;
  }
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
  class MeshData {
  private:
    vector<GLfloat> vertices;
    vector<GLuint>  indices;
  public:
    GLuint Shader;      // Шейдер, рисующий траву
    GLuint VAO;         // VAO для травы (что такое VAO почитайте в доках)
    GLuint Texture;     // Текстура земли
                        // Как её наложить , рассказано здесь: habrahabr.ru/post/315294/
                        // а ещё лучше здесь (оригинал): learnopengl.com/#!Getting-started/Textures
    string nameShader;  // Имя шейдерной программы, которую в дальнейшем скомпилируем и будем использовать

  // ==================================================================================== //
  public:
    //
    // constructor MeshData
    MeshData(const string& name): nameShader(name){
      // Set up vertex data (and buffer(s)) and attribute pointers
      // learnopengl.com/#!Getting-started/Textures
      GLfloat tempVertices[] = {
          // Positions         // Texture Coords
           1.0f,  0.0f, 1.0f,  1.0f, 1.0f, // Top Right
           1.0f,  0.0f, 0.0f,  1.0f, 0.0f, // Bottom Right
           0.0f,  0.0f, 0.0f,  0.0f, 0.0f, // Bottom Left
           0.0f,  0.0f, 1.0f,  0.0f, 1.0f  // Top Left
      };

      GLuint tempIndices[] = {  // Note that we start from 0!
          0, 1, 3, // First Triangle
          1, 2, 3  // Second Triangle
      };
      vertices = vector<GLfloat>(begin(tempVertices), end(tempVertices));
      indices = vector<GLuint>(begin(tempIndices), end(tempIndices));
    }
    virtual void BindTexture() {
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);
      Texture = SOIL_load_OGL_texture("ground.png", SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, 0);
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    // Создаем землю
    // Ground: function CreateObject
    virtual void CreateObject() {

      Shader = GL::CompileShaderProgram(nameShader);

      GLuint VBO, TBO;
      glGenBuffers(1, &VBO);
      glGenBuffers(1, &TBO);

      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indices.size(), indices.data(), GL_STATIC_DRAW);

      // Position attribute
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
      glEnableVertexAttribArray(0);
      // TexCoord attribute
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
      glEnableVertexAttribArray(1);

      glBindVertexArray(0); // Unbind VAO
    }


    // Рисуем землю
    // Ground: function DrawObject
    virtual void DrawObject() {
      // Используем шейдер для земли
      glUseProgram(Shader);                                                  CHECK_GL_ERRORS

      // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
      // Находим локацию юниформа 'camera' в шейдере
      GLint cameraLocation = glGetUniformLocation(Shader, "camera");         CHECK_GL_ERRORS
      // Устанавливаем юниформ (загружаем на GPU матрицу проекции?)                                                     // ###
      glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
      // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
      glBindVertexArray(VAO);

      glBindTexture(GL_TEXTURE_2D, Texture);

      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);                                                       CHECK_GL_ERRORS
      // Отключаем шейдер
      glUseProgram(0);                                                             CHECK_GL_ERRORS
    }
  };
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

  class MeshGrass: public MeshData {
  // ==================================================================================== //
  public:
    static vector<VM::vec4> m_StartOffset;
    static vector<VM::vec4> m_VarianceData;
    static vector<VM::vec4> m_PositionData;
    static vector<VM::vec4> m_SpeedData;
    static vector<VM::vec3> m_NormalData;

    static vector<float> m_MassData;
    static vector<float> m_HardData;

    static GLuint Variance;    // Буфер для смещения координат травинок
    static GLuint Normals;
    static GLuint PointsCount; // Количество вершин у модели травинки

    string nameShader;  // Имя шейдерной программы, которую в дальнейшем скомпилируем и
                        // будем использовать
  // ==================================================================================== //
  public:
    // constructor дочернего класса - ТРАВЫ: задаём имя шейдерной программы
    MeshGrass(const string& name): MeshData("ground"){
      nameShader = name;
      m_StartOffset.resize(GRASS_INSTANCES);
      m_VarianceData.resize(GRASS_INSTANCES);
      m_PositionData.resize(GRASS_INSTANCES);
      m_SpeedData.resize(GRASS_INSTANCES);
      m_NormalData.resize(GRASS_INSTANCES);

      m_MassData.resize(GRASS_INSTANCES);
      m_HardData.resize(GRASS_INSTANCES);

    }

  // ==================================================================================== //
  private:

    // function UpdateGrassVariance
    void UpdateGrassVariance() {

      // Генерация случайных смещений
      Physics physics = Physics::getInstance();
      physics.updatePhysics();

      // Пересчитываем нормали
      //physics.grassUpdateNormals();

      // Привязываем буфер, содержащий смещения
      glBindBuffer(GL_ARRAY_BUFFER, Variance);                                CHECK_GL_ERRORS
      // Загружаем данные в видеопамять
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, m_VarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // ???
      //glBindBuffer(GL_ARRAY_BUFFER, Normals);                                CHECK_GL_ERRORS
      //glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * GRASS_INSTANCES, m_NormalData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // Отвязываем буфер
      glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS

    }
    // function GenerateGrassPositions
    // Создаём позиции травинок на карте.
    vector<VM::vec2> GenerateGrassPositions() {
      vector<VM::vec2> grassPositions(GRASS_INSTANCES);
      for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassPositions[i] = VM::vec2(rnd<float>(0.01f, 100),
                                     rnd<float>(0.01f, 100));
      }
      return grassPositions;
    }


    // Создаём меш (геометрический объект) так, как сами пожелаем.
    // function GenMesh
    vector<VM::vec4> GenMesh() {
      //int k = 3;
      VM::vec4 tempMesh[] = {
        VM::vec4(0, 0, 0,           1),
        VM::vec4(0.25, 0, 0,        1),
        VM::vec4(0.1, 0.4, 0,       1),
        VM::vec4(0.4, 0.6, 0,       1),
        VM::vec4(0.3, 0.8, 0,       1),
        VM::vec4(0.7, 1, 0,         1),
        VM::vec4(0.9, 1.1, 0,       1),

      };
      return vector<VM::vec4>(begin(tempMesh), end(tempMesh));
    }
    // Генерируем точки текстуры. По ним будем накладывать картинку на примитив (экземпляр).
    // function GenTexPoints
    vector<VM::vec2> GenTexPoints() {
      //int k = 3;
      VM::vec2 tempTex[] = {
        VM::vec2(0,   1), VM::vec2(0, 0),
        VM::vec2(1/3, 1), VM::vec2(1/3, 0),
        VM::vec2(2/3, 1), VM::vec2(2/3, 0),
        VM::vec2(1, 2/3)
      };
      return vector<VM::vec2>(begin(tempTex), end(tempTex));
    }
  // ==================================================================================== //
  public:
    // Создаем траву
    // Grass: function CreateObject
    void CreateObject() {
      srand(time(NULL));

      // Создаём меш
      vector<VM::vec4> grassPoints = GenMesh();
      vector<VM::vec2> texturePoints = GenTexPoints();
      // Сохраняем количество вершин в меше травы
      PointsCount = grassPoints.size();
      // Создаём позиции для травинок
      vector<VM::vec2> grassPositions = GenerateGrassPositions();

      // Инициализация смещений для травинок
      Physics physics = Physics::getInstance();
      physics.initPhysics(grassPoints);

      /* Компилируем шейдеры
      Эта функция принимает на вход название шейдера 'shaderName',
      читает файлы shaders/{shaderName}.vert - вершинный шейдер
      и shaders/{shaderName}.frag - фрагментный шейдер,
      компилирует их и линкует.
      */
      Shader = GL::CompileShaderProgram(nameShader);
      // Здесь создаём буфер
      GLuint pointsBuffer;
      // Это генерация одного буфера (в pointsBuffer хранится идентификатор буфера)
      glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
      // Привязываем сгенерированный буфер
      glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
      // Заполняем буфер данными из вектора
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // Создание VAO
      // Генерация VAO
      glGenVertexArrays(1, &VAO);                                             CHECK_GL_ERRORS
      // Привязка VAO
      glBindVertexArray(VAO);                                                 CHECK_GL_ERRORS

      // Получение локации параметра 'point' в шейдере
      GLuint pointsLocation = glGetAttribLocation(Shader, "point");           CHECK_GL_ERRORS
      // Подключаем массив атрибутов к данной локации
      glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
      // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
      glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);          CHECK_GL_ERRORS
      glVertexAttribDivisor(pointsLocation, 0);                                  CHECK_GL_ERRORS


      // Создаём буфер для позиций травинок
      GLuint positionBuffer;
      glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
      // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
      glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      GLuint positionLocation = glGetAttribLocation(Shader, "position");      CHECK_GL_ERRORS
      glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
      glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
      // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
      glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS

      // Создаём буфер для смещения травинок
      glGenBuffers(1, &Variance);                                            CHECK_GL_ERRORS
      glBindBuffer(GL_ARRAY_BUFFER, Variance);                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, m_VarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      GLuint varianceLocation = glGetAttribLocation(Shader, "variance");      CHECK_GL_ERRORS
      glEnableVertexAttribArray(varianceLocation);                                 CHECK_GL_ERRORS
      glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
      glVertexAttribDivisor(varianceLocation, 1);                                  CHECK_GL_ERRORS
/*

      glGenBuffers(1, &Normals);
      glBindBuffer(GL_ARRAY_BUFFER, Normals);
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * GRASS_INSTANCES, m_NormalData.data(), GL_STATIC_DRAW);

      GLuint normalLocation = glGetAttribLocation(Shader, "normal");
      glEnableVertexAttribArray(normalLocation);
      glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glVertexAttribDivisor(normalLocation, 1);

*/
      GLuint texCoordBuffer;
      glGenBuffers(1, &texCoordBuffer);
      GLuint texCoordLocation = glGetAttribLocation(Shader, "texCoord");      CHECK_GL_ERRORS
      // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
      glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * texturePoints.size(), texturePoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      glEnableVertexAttribArray(texCoordLocation);                                 CHECK_GL_ERRORS
      glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
      glVertexAttribDivisor(texCoordLocation, 0);                                  CHECK_GL_ERRORS


      // Отвязываем VAO
      glBindVertexArray(0);                                                        CHECK_GL_ERRORS
      // Отвязываем буфер
      glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
    }

    // Рисуем траву
    // Grass: function DrawObject
    void DrawObject() {
      // Тут то же самое, что и в рисовании земли
      glUseProgram(Shader);                                                   CHECK_GL_ERRORS
      GLint cameraLocation = glGetUniformLocation(Shader, "camera");          CHECK_GL_ERRORS
      glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
      glBindVertexArray(VAO);


      // Обновляем смещения для травы
      UpdateGrassVariance();


      glBindTexture(GL_TEXTURE_2D, Texture);                                          CHECK_GL_ERRORS
      // Отрисовка травинок в количестве GRASS_INSTANCES
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, PointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
      glBindVertexArray(0);                                                        CHECK_GL_ERRORS
      glUseProgram(0);                                                             CHECK_GL_ERRORS
    }

    void BindTexture() {
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);
      int width, height;
      unsigned char* image = SOIL_load_image("grass.png", &width, &height, 0, SOIL_LOAD_RGB);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
      glGenerateMipmap(GL_TEXTURE_2D);
      SOIL_free_image_data(image);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

  };
  class MeshSkyBox: public MeshData {
  public:
    GLuint Shader;
    GLuint VAO;
    GLuint Texture;

    vector<GLfloat> vertices;
    vector<GLuint>  indices;
    vector<GLchar*> faces;

    string nameShader;
  public:
    MeshSkyBox(const string& name): MeshData("ground"){
      nameShader = name;
      // Инициализируем вершины куба в пространстве.
      // Материалы взяты с сайта: http://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox_data
      GLfloat Vertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
      };
      vertices = vector<GLfloat>(begin(Vertices), end(Vertices));
      // Картинки для skybox
      faces.push_back("../Texture/right.png" );
      faces.push_back("../Texture/left.png"  );
      faces.push_back("../Texture/top.png"   );
      faces.push_back("../Texture/bottom.png");
      faces.push_back("../Texture/front.png" );
      faces.push_back("../Texture/back.png"  );
    }
  private:
  public:
    void CreateObject() {
      Shader = GL::CompileShaderProgram(nameShader);

      glGenVertexArrays(1, &VAO);                                                       CHECK_GL_ERRORS
      glBindVertexArray(VAO);                                                           CHECK_GL_ERRORS

      GLuint VBO;
      glGenBuffers(1, &VBO);                                                            CHECK_GL_ERRORS
      glBindBuffer(GL_ARRAY_BUFFER, VBO);                                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      glEnableVertexAttribArray(0);                                                           CHECK_GL_ERRORS
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);                         CHECK_GL_ERRORS
      glBindVertexArray(0);                                                                   CHECK_GL_ERRORS
      glBindBuffer(GL_ARRAY_BUFFER, 0);                                                       CHECK_GL_ERRORS
    }
    // Рисуем куб как землю, только с новыми флагами GL_TEXTURE_CUBE_MAP
    // Растеризация происходит по примитивам (треугольникам - GL_TRIANGLES ).
    // Материалы взяты с сайта: http://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox
    void DrawObject() {
      glDepthFunc(GL_LEQUAL);
      glUseProgram(Shader);                                                  CHECK_GL_ERRORS

      glActiveTexture(GL_TEXTURE0);
      glUniform1i(glGetUniformLocation(Shader, nameShader.c_str()), 0);
      glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

      GLint cameraLocation = glGetUniformLocation(Shader, "camera");         CHECK_GL_ERRORS
      glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

      glBindVertexArray(VAO);                                                CHECK_GL_ERRORS

      glDrawArrays(GL_TRIANGLES, 0, 36);
      glBindVertexArray(0);                                                        CHECK_GL_ERRORS

      glUseProgram(0);                                                             CHECK_GL_ERRORS

    }
    // Загрузка текстур для skybox.
    // Материалы взяты с сайта: http://learnopengl.com/#!Advanced-OpenGL/Cubemaps
    void BindTexture() {
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

      int width,height, channels;
      unsigned char* image;

      for(GLuint i = 0; i < faces.size(); i++) {
        image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGBA);

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image
        );
      };

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    }
  };
};
