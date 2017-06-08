#include <iostream>
#include <vector>
#include <time.h>
#include "Utility.h"
#include <soil.h>
#include "Instancing.cpp"
using namespace std;

Instancing::MeshData ground("ground");
Instancing::MeshData *grass = new Instancing::MeshGrass("grass");
Instancing::MeshData *skybox = new Instancing::MeshSkyBox("skybox");


// Объявление статический переменных
Physics* Physics::p_instance = 0;
std::mutex Physics::mutLock;

// Размеры экрана
uint screenWidth = 800;
uint screenHeight = 600;
// Захват мышки.
bool captureMouse = true;

// Handler's functions: обработчики событий
// Эта функция вызывается для обновления экрана
void RenderLayouts() {
  // Включение буфера глубины
  glEnable(GL_DEPTH_TEST);
  // Очистка буфера глубины и цветового буфера
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Рисуем меши
  grass->DrawObject();
  ground.DrawObject();
  skybox->DrawObject();

  glutSwapBuffers();
}

// Завершение программы
void FinishProgram() {
  glutDestroyWindow(glutGetWindow());
}

// MSAA
int flagMSAA = 1;
// http://stackoverflow.com/questions/27843294/glut-opengl-multisampling-anti-aliasing-msaa
void enableMultisample()
{
  if (flagMSAA > 0)
  {
    glEnable(GL_MULTISAMPLE);
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

    // detect current settings
    GLint iMultiSample = 0;
    GLint iNumSamples = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &iMultiSample);
    glGetIntegerv(GL_SAMPLES, &iNumSamples);
    printf("MSAA on, GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d\n", iMultiSample, iNumSamples);
  }
  else
  {
    glDisable(GL_MULTISAMPLE);
    printf("MSAA off\n");
  }
  flagMSAA *= -1;
}

// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
  if (key == 27) {
    FinishProgram();
  } else if ((key == 'W') || (key == 'w')) {
    Instancing::camera.goForward();
  } else if ((key == 'S') || (key == 's')) {
    Instancing::camera.goBack();
  } else if ((key == 'M') || (key == 'm')) {
    captureMouse = !captureMouse;
    if (captureMouse) {
      glutWarpPointer(screenWidth / 2, screenHeight / 2);
      glutSetCursor(GLUT_CURSOR_NONE);
    } else {
      glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
    }
  } else if ((key == 'A') || (key == 'a')) {
    enableMultisample();
  }
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int button, int x, int y) {
  if (button == GLUT_KEY_UP) {
    Instancing::camera.rotateTop(-0.02);
  } else if (button == GLUT_KEY_LEFT) {
    Instancing::camera.rotateY(-0.02);
  } else if (button == GLUT_KEY_RIGHT) {
    Instancing::camera.rotateY(0.02);
  } else if (button == GLUT_KEY_DOWN) {
    Instancing::camera.rotateTop(0.02);
  }
}

void IdleFunc() {
  glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {
  if (captureMouse) {
    int centerX = screenWidth / 2,
      centerY = screenHeight / 2;
    if (x != centerX || y != centerY) {
      Instancing::camera.rotateY((x - centerX) / 1000.0f);
      Instancing::camera.rotateTop((y - centerY) / 1000.0f);
      glutWarpPointer(centerX, centerY);
    }
  }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
}

// Событие изменение размера окна
void windowReshapeFunc(GLint newWidth, GLint newHeight) {
  glViewport(0, 0, newWidth, newHeight);
  screenWidth = newWidth;
  screenHeight = newHeight;

  Instancing::camera.screenRatio = (float)screenWidth / screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {
  glutInit(&argc, argv);
  // #include <MSAA>
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
  glutInitContextVersion(3, 0);
  glutInitWindowPosition(-1, -1);
  glutInitWindowSize(screenWidth, screenHeight);
  glutCreateWindow("Computer Graphics 3");
  glutWarpPointer(400, 300);
  glutSetCursor(GLUT_CURSOR_NONE);

  glutDisplayFunc(RenderLayouts);
  glutKeyboardFunc(KeyboardEvents);
  glutSpecialFunc(SpecialButtons);
  glutIdleFunc(IdleFunc);
  glutPassiveMotionFunc(MouseMove);
  glutMouseFunc(MouseClick);
  glutReshapeFunc(windowReshapeFunc);
}


// Создаём камеру (Если шаблонная камера вам не нравится, то можете переделать, но я бы не стал)
void CreateCamera() {
  Instancing::camera.angle = 45.0f / 180.0f * M_PI;
  Instancing::camera.direction = VM::vec3(0, 0.3, -1);
  Instancing::camera.position = VM::vec3(0.5, 0.2, 0);
  Instancing::camera.screenRatio = (float)screenWidth / screenHeight;
  Instancing::camera.up = VM::vec3(0, 1, 0);
  Instancing::camera.zfar = 50.0f;
  Instancing::camera.znear = 0.05f;
}

void BindTextures()
{
  grass->BindTexture();
  ground.BindTexture();
  skybox->BindTexture();
}

int main(int argc, char **argv)
{
  try {
    cout << "Start" << endl;
    Physics physics = Physics::getInstance();
    cout << "Physics enabled" << endl;
    InitializeGLUT(argc, argv);
    cout << "GLUT inited" << endl;
    glewInit();
    cout << "glew inited" << endl;
    BindTextures();
    cout << "Textures binded" << endl;
    CreateCamera();
    cout << "Camera created" << endl;
    grass->CreateObject();
    cout << "Grass created" << endl;
    ground.CreateObject();
    cout << "Ground created" << endl;
    skybox->CreateObject();
    cout << "Skybox created" << endl;
    glutMainLoop();
  } catch (string s) {
    cout << s << endl;
  }
}
