#include "Helpers.h"
#include "Objects.h"

void Object::init(meshType& t, int i){
    centroid = vec3(0.0f, 0.0f, 0.0f);
    type = t;
    color = vec3(1.2f, 0.4f, 0.2f);
    if(t == CUBE) radius = 1.0f;
    if(t == BUNNY) radius = 0.55f;
    if(t == BUMPY_CUBE) radius = 0.7f;
    model = mat4(1.0f);
    renderMode = 0;
    id = i;
};