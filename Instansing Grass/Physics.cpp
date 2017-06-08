#include "Instancing.cpp"
using namespace Instancing;

  const float dT = 0.001;
  // change
  GLuint MeshGrass::Variance;
  GLuint MeshGrass::Normals;
  GLuint MeshGrass::PointsCount;
  vector<VM::vec4> MeshGrass::m_StartOffset;
  vector<VM::vec4> MeshGrass::m_VarianceData;
  vector<VM::vec4> MeshGrass::m_PositionData;
  vector<VM::vec4> MeshGrass::m_SpeedData;

  vector<VM::vec3> MeshGrass::m_NormalData;

  vector<float> MeshGrass::m_MassData;
  vector<float> MeshGrass::m_HardData;
  //namespace Instancing {
  // function random
  //template <typename T>
//  class TemplateSTD;
// };
  template <typename T>
  T rndom(double number = 0.03f, double add = 0.0f) {
      if (number > 0)
        return (T) rand() * number / RAND_MAX + add;
      else
        return 0;
  }
  // change +++
  /*
  void Physics::grassUpdateNormals() {
    VM::vec4 point1 = {0.0f, 0.0f, 0.0f, 1.0f},
             point2 = {0.33f, 0.0f, 0.0f, 1.0f},
             point3 = {1.0f, 1.0f, 0.0f, 1.0f};

    MeshGrass::m_NormalData.clear();
    float tempValueRotation, tempScaleCoefficient;
    cout<<"|";
    for (int i = 0; i < GRASS_INSTANCES; i++) {
      cout<<i<<"[";
      tempValueRotation = rndom<float>(1);
      VM::mat4 rotationMatrix(1.0f);
      cout<<01;
      rotationMatrix[0][0] = cos(tempValueRotation);
      rotationMatrix[0][2] = sin(tempValueRotation);
      rotationMatrix[2][0] = -sin(tempValueRotation);
      rotationMatrix[2][2] = cos(tempValueRotation);

      tempScaleCoefficient = rndom<GLfloat>(5);
      VM::vec4 newPoint1 = point1;
      VM::vec4 newPoint2 = rotationMatrix * tempScaleCoefficient * point2;
      VM::vec4 newPoint3 = rotationMatrix * tempScaleCoefficient * point3 + MeshGrass::m_VarianceData[i] * point3.y;
      //VM::vec4 newPoint3 = MeshGrass::m_VarianceData[i];

      VM::vec3 edge1 = newPoint2.xyz() - newPoint1.xyz();
      VM::vec3 edge2 = newPoint3.xyz() - newPoint1.xyz();
      VM::vec3 normal = VM::cross(edge1, edge2);
      cout<<02;
      cout<<"("<< normal <<","<< VM::normalize(normal)<<")";
      normal = VM::normalize(normal);
      cout<<03;
      normal = (normal.z >= 0) ? (normal) : (normal * -1);
      cout<<04;
      MeshGrass::m_NormalData.push_back(normal);
      cout<<05;
      cout<<"]";
    }
}
*/

  // Инициализация смещений для травинок
  // Физика: начальное состояние всех травинок находится в равновесии
  // Физика: начальное смещение, смещение, позоция травинки, скорость травинки (по умолчанию равна 0).  // Инициализация смещений для травинок
  void Physics::initPhysics(vector<VM::vec4> & grassPoints){
    // Сохраняем количество вершин в меше травы
    VM::vec4 null(0, 0, 0, 0);
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
      MeshGrass::m_StartOffset[i] = VM::vec4(rndom<double>(), 0, rndom<double>(), 0);
      MeshGrass::m_VarianceData[i] = MeshGrass::m_StartOffset[i];
      MeshGrass::m_PositionData[i] = MeshGrass::m_VarianceData[i]
                                     + grassPoints[MeshGrass::PointsCount - 1];
      MeshGrass::m_SpeedData[i] = null;
      // change coefficients
      MeshGrass::m_HardData[i] = rndom<float>(2, 1);
      MeshGrass::m_MassData[i] = rndom<float>(0.02, 0.005);
    }
    G = VM::vec4(0.0, -9.8, 0.0, 0.0);
    forceWind = VM::vec4(2.1, 0.0, 2.0, 1.0);
    // Инициализируем нормали для травы
    // grassUpdateNormals();
  }
  // Функции для работы с физикой
  VM::vec4 Physics::getResultForce(float m, VM::vec4 position, float hardness){
    return G*m + forceWind - position * hardness;
  }
  VM::vec4 Physics::getAccelerationOfGravity(VM::vec4 force, float mass){
    return force / mass;
  }
  VM::vec4 Physics::getSpeed(VM::vec4 accelerationOfGravity){
    float dT = 0.001;
    return accelerationOfGravity * dT;
  }
  VM::vec4 Physics::getResultVarriance(VM::vec4 startOffset,
                                       VM::vec4 speed,
                                       VM::vec4 accelerationOfGravity) {
    float dT = 0.001;
    return startOffset + speed * dT + accelerationOfGravity * dT * dT / 2;
  }
  // Обновление смещений
  // Физика: Находим результирующую силу по 2 - ому закону Ньютона, затем ускорение.
  // Физика: В итоге, скорость и координаты перемещения (смещение), учитывая начальное смещение.
  void Physics::updatePhysics() {
    float mass, hardness;
    for (uint i = 0; i < GRASS_INSTANCES; i++) {
      mass = MeshGrass::m_MassData[i];
      hardness = MeshGrass::m_HardData[i];
      VM::vec4 F = getResultForce(mass, MeshGrass::m_PositionData[i], hardness);
      VM::vec4 a = getAccelerationOfGravity(F, mass);
      MeshGrass::m_SpeedData[i] += getSpeed(a);
      MeshGrass::m_VarianceData[i] = getResultVarriance(MeshGrass::m_StartOffset[i], MeshGrass::m_SpeedData[i], a);
      MeshGrass::m_PositionData[i] += MeshGrass::m_VarianceData[i];
    }
  }
  // Функция для получения единственного объекта для работы с физикой
  Physics& Physics::getInstance() {
    if(!p_instance)
    {
      std::lock_guard<std::mutex> lock(mutLock);
      if (!p_instance)
        p_instance = new Physics();
    }
    return *p_instance;
  }
  Physics::Physics(){}
  Physics::Physics(const Physics& other){
    if(!p_instance)
    {
      std::lock_guard<std::mutex> lock(mutLock);
      if (!p_instance)
        p_instance = new Physics();
    }
    this->p_instance = other.p_instance;
  }
