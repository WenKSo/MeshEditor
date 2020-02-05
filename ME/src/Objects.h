#include "Helpers.h"
using namespace std;
using namespace glm;

enum meshType {
  CUBE,
  BUNNY,
  BUMPY_CUBE
};

class Object{
    public:
        int id;
        vec3 color;
        meshType type; 
        vec3 centroid;
        float radius;
        VertexArrayObject VAO;
        mat4 model;
        int renderMode;
        void init(meshType& t, int id);    
        
};