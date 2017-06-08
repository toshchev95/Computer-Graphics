#include <SOIL.h>
#include <iostream>
#include <vector>
#include <mutex>
using namespace std;

//namespace Instancing {
  // Объявляем прототип для функций initPhysics, updatePhysics.;
  class Physics {
    Physics();
    // Функции для работы с физикой
    VM::vec4 getResultForce(float, VM::vec4, float);
    VM::vec4 getAccelerationOfGravity(VM::vec4, float);
    VM::vec4 getSpeed(VM::vec4 );
    VM::vec4 getResultVarriance(VM::vec4, VM::vec4, VM::vec4);
//    ~Physics();
  public:
    Physics(const Physics&);
    static Physics& getInstance();
    void initPhysics(vector<VM::vec4> &);
    void updatePhysics();
    void grassUpdateNormals();
  private:
    VM::vec4 G, forceWind;
    static mutex mutLock;
    static Physics* p_instance;
  };
/*
  template <typename T>
  class TemplateSTD {
  public:
    static T rnd(double number = 0.03f, double add = 0.0f);++
  };*/
//};
