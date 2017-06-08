
#include <SOIL.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <ctime>
#include "Utility.h"
#include "Physics.h"

using namespace std;

namespace Instancing {
  const uint GRASS_INSTANCES = 10000; // ���������� ��������

  static GL::Camera camera; // �� ������������� ��� ���������� ������. � OpenGL ������ - ��� ������ 2 �������. ��������-������� ������� � ������� ��������. // ###
                            //������ ����� ������ ������ � ��� ����� ���������� ���� � ���������� � ��������� ������������ ��� �������.
                            //�� ������ ������ ������������ ���� ������� ��� ������� ��������� ������.

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
    GLuint Shader;      // ������, �������� �����
    GLuint VAO;         // VAO ��� ����� (��� ����� VAO ��������� � �����)
    GLuint Texture;     // �������� �����
                        // ��� � �������� , ���������� �����: habrahabr.ru/post/315294/
                        // � ��� ����� ����� (��������): learnopengl.com/#!Getting-started/Textures
    string nameShader;  // ��� ��������� ���������, ������� � ���������� ������������ � ����� ������������

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
    // ������� �����
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


    // ������ �����
    // Ground: function DrawObject
    virtual void DrawObject() {
      // ���������� ������ ��� �����
      glUseProgram(Shader);                                                  CHECK_GL_ERRORS

      // ������������� ������� ��� �������. � ������ ������ ��������� ������������� ������� ������
      // ������� ������� �������� 'camera' � �������
      GLint cameraLocation = glGetUniformLocation(Shader, "camera");         CHECK_GL_ERRORS
      // ������������� ������� (��������� �� GPU ������� ��������?)                                                     // ###
      glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
      // ���������� VAO, ������� �������� ������, ����������� ��� ��������� �����
      glBindVertexArray(VAO);

      glBindTexture(GL_TEXTURE_2D, Texture);

      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);                                                       CHECK_GL_ERRORS
      // ��������� ������
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

    static GLuint Variance;    // ����� ��� �������� ��������� ��������
    static GLuint Normals;
    static GLuint PointsCount; // ���������� ������ � ������ ��������

    string nameShader;  // ��� ��������� ���������, ������� � ���������� ������������ �
                        // ����� ������������
  // ==================================================================================== //
  public:
    // constructor ��������� ������ - �����: ����� ��� ��������� ���������
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

      // ��������� ��������� ��������
      Physics physics = Physics::getInstance();
      physics.updatePhysics();

      // ������������� �������
      //physics.grassUpdateNormals();

      // ����������� �����, ���������� ��������
      glBindBuffer(GL_ARRAY_BUFFER, Variance);                                CHECK_GL_ERRORS
      // ��������� ������ � �����������
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, m_VarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // ???
      //glBindBuffer(GL_ARRAY_BUFFER, Normals);                                CHECK_GL_ERRORS
      //glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * GRASS_INSTANCES, m_NormalData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // ���������� �����
      glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS

    }
    // function GenerateGrassPositions
    // ������ ������� �������� �� �����.
    vector<VM::vec2> GenerateGrassPositions() {
      vector<VM::vec2> grassPositions(GRASS_INSTANCES);
      for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassPositions[i] = VM::vec2(rnd<float>(0.01f, 100),
                                     rnd<float>(0.01f, 100));
      }
      return grassPositions;
    }


    // ������ ��� (�������������� ������) ���, ��� ���� ��������.
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
    // ���������� ����� ��������. �� ��� ����� ����������� �������� �� �������� (���������).
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
    // ������� �����
    // Grass: function CreateObject
    void CreateObject() {
      srand(time(NULL));

      // ������ ���
      vector<VM::vec4> grassPoints = GenMesh();
      vector<VM::vec2> texturePoints = GenTexPoints();
      // ��������� ���������� ������ � ���� �����
      PointsCount = grassPoints.size();
      // ������ ������� ��� ��������
      vector<VM::vec2> grassPositions = GenerateGrassPositions();

      // ������������� �������� ��� ��������
      Physics physics = Physics::getInstance();
      physics.initPhysics(grassPoints);

      /* ����������� �������
      ��� ������� ��������� �� ���� �������� ������� 'shaderName',
      ������ ����� shaders/{shaderName}.vert - ��������� ������
      � shaders/{shaderName}.frag - ����������� ������,
      ����������� �� � �������.
      */
      Shader = GL::CompileShaderProgram(nameShader);
      // ����� ������ �����
      GLuint pointsBuffer;
      // ��� ��������� ������ ������ (� pointsBuffer �������� ������������� ������)
      glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
      // ����������� ��������������� �����
      glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
      // ��������� ����� ������� �� �������
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      // �������� VAO
      // ��������� VAO
      glGenVertexArrays(1, &VAO);                                             CHECK_GL_ERRORS
      // �������� VAO
      glBindVertexArray(VAO);                                                 CHECK_GL_ERRORS

      // ��������� ������� ��������� 'point' � �������
      GLuint pointsLocation = glGetAttribLocation(Shader, "point");           CHECK_GL_ERRORS
      // ���������� ������ ��������� � ������ �������
      glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
      // ������������� ��������� ��� ��������� ������ �� ������� (�� 4 �������� ���� float �� ���� �������)
      glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);          CHECK_GL_ERRORS
      glVertexAttribDivisor(pointsLocation, 0);                                  CHECK_GL_ERRORS


      // ������ ����� ��� ������� ��������
      GLuint positionBuffer;
      glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
      // ����� �� ����������� ����� �����, ��� ��� ������ ��� ������ ����� � ��� �� ���������� ������ glBindBuffer
      glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      GLuint positionLocation = glGetAttribLocation(Shader, "position");      CHECK_GL_ERRORS
      glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
      glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
      // ����� �� ���������, ��� ����� ����� ����� �������� �� ����� ������ ��� ������� �������� (��� ������ ��������)
      glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS

      // ������ ����� ��� �������� ��������
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
      // ����� �� ����������� ����� �����, ��� ��� ������ ��� ������ ����� � ��� �� ���������� ������ glBindBuffer
      glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);                               CHECK_GL_ERRORS
      glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * texturePoints.size(), texturePoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

      glEnableVertexAttribArray(texCoordLocation);                                 CHECK_GL_ERRORS
      glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
      glVertexAttribDivisor(texCoordLocation, 0);                                  CHECK_GL_ERRORS


      // ���������� VAO
      glBindVertexArray(0);                                                        CHECK_GL_ERRORS
      // ���������� �����
      glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
    }

    // ������ �����
    // Grass: function DrawObject
    void DrawObject() {
      // ��� �� �� �����, ��� � � ��������� �����
      glUseProgram(Shader);                                                   CHECK_GL_ERRORS
      GLint cameraLocation = glGetUniformLocation(Shader, "camera");          CHECK_GL_ERRORS
      glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
      glBindVertexArray(VAO);


      // ��������� �������� ��� �����
      UpdateGrassVariance();


      glBindTexture(GL_TEXTURE_2D, Texture);                                          CHECK_GL_ERRORS
      // ��������� �������� � ���������� GRASS_INSTANCES
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
      // �������������� ������� ���� � ������������.
      // ��������� ����� � �����: http://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox_data
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
      // �������� ��� skybox
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
    // ������ ��� ��� �����, ������ � ������ ������� GL_TEXTURE_CUBE_MAP
    // ������������ ���������� �� ���������� (������������� - GL_TRIANGLES ).
    // ��������� ����� � �����: http://learnopengl.com/code_viewer.php?code=advanced/cubemaps_skybox
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
    // �������� ������� ��� skybox.
    // ��������� ����� � �����: http://learnopengl.com/#!Advanced-OpenGL/Cubemaps
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
