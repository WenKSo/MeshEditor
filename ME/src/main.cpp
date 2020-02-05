#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include "Helpers.h"
#include "Objects.h"
#include <unistd.h>
#include <glm/gtx/string_cast.hpp>
#include <unordered_set>
#include <set>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#else
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#endif

#include <math.h>

// Shortcut to avoid Eigen:: and std:: everywhere, DO NOT USE IN .h
using namespace std;
using namespace glm;

int x = 0;
Object ob;
vector<int> indices_cube;
vector<float> vertices_cube;
vector<int> indices_bunny;
vector<float> vertices_bunny;
vector<int> indices_bumpy;
vector<float> vertices_bumpy;
vector<float> vertices1;
vector<float> vertices2;
vector<float> vertices3;
VertexBufferObject VBO_CUBE;
VertexBufferObject VBO_BUNNY;
VertexBufferObject VBO_BUMPY;
VertexBufferObject EBO_CUBE;
VertexBufferObject EBO_BUNNY;
VertexBufferObject EBO_BUMPY;

vec3 cameraPos   = vec3(0.0f, 0.0f,  3.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp    = vec3(0.0f, 1.0f,  0.0f);
vec3 cameraZFront = vec3(0.0f,0.0f,-1.0f);
vec3 cameraXRight = vec3(1.0f, 0.0f,  0.0f);
vec3 origin = vec3(0.0f,0.0f,0.0f);
vec3 cameraDirection = normalize(origin-cameraPos);
vec3 up = vec3(0.0f, 1.0f, 0.0f);
vec3 lightPos(0.0f, 0.0f, 3.0f);
vec3 cameraRight = normalize(cross(up, cameraDirection));
vec3 baryCenterBunny;
vec3 baryCenterBumpy;
vector< vector<int> > surrEdges;
unordered_set<int> surrVertices;

// Three main matrices
mat4 view = mat4(1.0f);
mat4 projection = mat4(1.0f);

// Switches
int projectionMode = 0;

int camMode = 0;
int currentMode=0;
int currentVertex=-1;
int mergeVertexOne=-1;
int mergeVertexTwo=-1;
int clickCount=0;
float rx=0;
float ry=0;
float rz=0;

float camSpeed = 5.0f;
float camFrontSpeed = 0.05f;
float rotateSpeed = 2.0f;
float transSpeed = 0.01f;
float scaleSpeed = 0.05f;

float cursorPosX;
float cursorPosY;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {glViewport(0, 0, width, height);};
void loadMesh(vector<float>& vertices, vector<int>& indices, string filepath);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
vector<vec3> getVertexNorms(vector<float>& vertices,vector<int>& indices);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void updateSurroundingVertex(vector<int>& indices);
void mergeVertices(int a, int b, vector<float>& vertices, vector<int>& indices);
void updateNormals(vector<float>& vertices, vector<int>& indices);

int main(void){
    GLFWwindow* window;
    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, "Mesh Editor", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    glEnable(GL_DEPTH_TEST);
    const GLchar* vertex_shader =
            "#version 150 core\n"
            "in vec3 position;"
            "in vec3 normal;"

            "out vec3 FragPos;"
            "out vec3 Normal;"
            "uniform mat4 view;"
            "uniform mat4 projection;"
            "uniform mat4 model;"
            "void main()"
            "{"
            "    FragPos = vec3(model * vec4(position, 1.0));"
            "    Normal = mat3(transpose(inverse(model))) * normal;"
            "    gl_Position = projection * view * vec4(FragPos, 1.0);"
            "}";
    const GLchar* fragment_shader =
            "#version 150 core\n"
            "in vec3 FragPos;"
            "in vec3 Normal;"
            "out vec4 FragColor;"

            "uniform vec3 lightPos;"
            "uniform vec3 lightColor;"
            "uniform vec3 objectColor;"

            "void main()"
            "{"
            "float ambientStrength = 1.0;"
            "vec3 ambient = ambientStrength * lightColor;"
            "vec3 norm = normalize(Normal);"
            "vec3 lightDir = normalize(lightPos - FragPos);"
            "float diff = max(dot(norm, lightDir), 0.0);"
            "vec3 diffuse = diff * lightColor;"

            "vec3 result = (ambient+diffuse) * objectColor;"
            "FragColor = vec4(result, 1.0);"
            "} ";
    Program program;
    program.init(vertex_shader,fragment_shader,"FragColor");
    // VAO FOR CUBE-----------------------------------------
    VertexArrayObject VAO_CUBE;
    VAO_CUBE.init();
    loadMesh(vertices_cube,indices_cube,"../data/cube.off");
    for(int i=0;i<vertices_cube.size();i+=3){
        if(i<12) vertices_cube[i+2]= 0.5;
        else vertices_cube[i+2]= -0.5;
    }
    vector<vec3> normals_cube = getVertexNorms(vertices_cube,indices_cube);
    for(vec3 x: normals_cube) cout << to_string(x).c_str() << endl;
    for(int i=0;i<vertices_cube.size();i+=3){
        vertices1.push_back(vertices_cube[i]);
        vertices1.push_back(vertices_cube[i+1]);
        vertices1.push_back(vertices_cube[i+2]);
        vertices1.push_back(normals_cube[i/3].x);
        vertices1.push_back(normals_cube[i/3].y);
        vertices1.push_back(normals_cube[i/3].z);
    }
    VAO_CUBE.bind();
    VBO_CUBE.init();
    VBO_CUBE.updateV(vertices1);
    // position attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    // normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    EBO_CUBE.init();
    EBO_CUBE.updateE(indices_cube);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    // ------------------------------------------------------

    // VAO FOR BUNNY-----------------------------------------
    VertexArrayObject VAO_BUNNY;
    VAO_BUNNY.init();
    VAO_BUNNY.bind();
    loadMesh(vertices_bunny,indices_bunny,"../data/Armadillo.off");
    for(int i=0;i<vertices_bunny.size();i++) vertices_bunny[i] /= 90;
    for(int i=0;i<vertices_bunny.size()/3;i++){
        rx+=vertices_bunny[i*3];
        ry+=vertices_bunny[i*3+1];
        rz+=vertices_bunny[i*3+2];
    }
    baryCenterBunny = vec3(rx/vertices_bunny.size()*3,ry/vertices_bunny.size()*3,rz/vertices_bunny.size()*3);
    for(int i=0;i<vertices_bunny.size()/3;i++) {
        vertices_bunny[i*3]-=baryCenterBunny.x;
        vertices_bunny[i*3+1]-=baryCenterBunny.y;
        vertices_bunny[i*3+2]-=baryCenterBunny.z;
    }
    vector<vec3> normals_bunny = getVertexNorms(vertices_bunny,indices_bunny);
    for(int i=0;i<vertices_bunny.size();i+=3){
        vertices2.push_back(vertices_bunny[i]);
        vertices2.push_back(vertices_bunny[i+1]);
        vertices2.push_back(vertices_bunny[i+2]);
        vertices2.push_back(normals_bunny[i/3].x);
        vertices2.push_back(normals_bunny[i/3].y);
        vertices2.push_back(normals_bunny[i/3].z);
    }
    VBO_BUNNY.init();
    VBO_BUNNY.updateV(vertices2);
    // position attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    EBO_BUNNY.init();
    EBO_BUNNY.updateE(indices_bunny);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    // ------------------------------------------------------

    // VAO FOR BUMPY_CUBE-----------------------------------------
    VertexArrayObject VAO_BUMPY;
    VAO_BUMPY.init();
    loadMesh(vertices_bumpy,indices_bumpy,"../data/happy_vrip.off");
    for(int i=0;i<vertices_bumpy.size();i++) vertices_bumpy[i] *= 10;
    rx=0;ry=0;rz=0;
    for(int i=0;i<vertices_bumpy.size()/3;i++){
        rx+=vertices_bumpy[i*3];
        ry+=vertices_bumpy[i*3+1];
        rz+=vertices_bumpy[i*3+2];
    }
    baryCenterBumpy = vec3(rx/vertices_bumpy.size()*3,ry/vertices_bumpy.size()*3,rz/vertices_bumpy.size()*3);
    for(int i=0;i<vertices_bumpy.size()/3;i++) {
        vertices_bumpy[i*3]-=baryCenterBumpy.x;
        vertices_bumpy[i*3+1]-=baryCenterBumpy.y;
        vertices_bumpy[i*3+2]-=baryCenterBumpy.z;
    }
    vector<vec3> normals_bumpy = getVertexNorms(vertices_bumpy,indices_bumpy);
    for(int i=0;i<vertices_bumpy.size();i+=3){
        vertices3.push_back(vertices_bumpy[i]);
        vertices3.push_back(vertices_bumpy[i+1]);
        vertices3.push_back(vertices_bumpy[i+2]);
        vertices3.push_back(normals_bumpy[i/3].x);
        vertices3.push_back(normals_bumpy[i/3].y);
        vertices3.push_back(normals_bumpy[i/3].z);
    }
    VBO_BUMPY.init();
    VAO_BUMPY.bind();
    for(int i=0;i<vertices_bumpy.size();i++) vertices_bumpy[i] *= 0.15;
    VBO_BUMPY.updateV(vertices3);
    // position attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    EBO_BUMPY.init();
    EBO_BUMPY.updateE(indices_bumpy);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    // ------------------------------------------------------

    program.bind();
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glUniform3f(program.uniform("lightPos"), lightPos.x,lightPos.y,lightPos.z);
    glUniform3f(program.uniform("lightColor"), 1.0f, 1.0f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        // Clear the framebuffer
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        program.bind();
        int triangleCount=0;
        switch(ob.type){
            case CUBE:{
                VAO_CUBE.bind();
                triangleCount = indices_cube.size()/3;
                break;
            }
            case BUNNY:{
                VAO_BUNNY.bind();
                triangleCount = indices_bunny.size()/3;
                break;
                }
            case BUMPY_CUBE:{
                VAO_BUMPY.bind();
                triangleCount = indices_bumpy.size()/3;
                break;
                }
            default:
                break;
        }

        // Construct Model, View, Projection Matrix

        if(projectionMode == 0) projection = perspective(radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        else projection  = ortho(-1.0f*width/height, 1.0f*width/height, -1.0f, 1.0f, 0.1f, 100.0f);
        //glUniform3f(program.uniform("viewPos"), cameraPos.x,cameraPos.y,cameraPos.z);
        glUniformMatrix4fv(program.uniform("projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, &(ob.model)[0][0]);
        if(camMode==0) view = lookAt(cameraPos, origin, cross(cameraDirection, cameraRight));
        else view = lookAt(cameraPos, origin, cameraUp);
        glUniformMatrix4fv(program.uniform("view"), 1, GL_FALSE, &view[0][0]);
        cameraDirection = normalize(origin-cameraPos);
        cameraRight = normalize(cross(up, cameraDirection));
        cameraFront = normalize(cameraDirection);
        if(currentMode!=4) clickCount=0;
        glUniform3f(program.uniform("objectColor"), 1,1,1);
        for(int i=0;i<triangleCount; i++){
            //glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)(i * 3 * sizeof(unsigned int)));
            glDrawElements(GL_LINE_LOOP, 3, GL_UNSIGNED_INT, (void *)(i * 3 * sizeof(unsigned int)));
        }
        glUniform3f(program.uniform("objectColor"), ob.color.x,ob.color.y,ob.color.z);
        for(int i=0;i<triangleCount; i++){
            //glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)(i * 3 * sizeof(unsigned int)));
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)(i * 3 * sizeof(unsigned int)));
        }
        glBindVertexArray(0);

        // Swap front and back buffers
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
    }

    // Deallocate opengl memory
    program.free();
    VAO_BUNNY.free();
    VAO_BUMPY.free();
    VBO_CUBE.free();
    VBO_BUNNY.free();
    VBO_BUMPY.free();
    EBO_CUBE.free();
    EBO_BUNNY.free();
    EBO_BUMPY.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}

void print(std::unordered_set<int> const &s)
{
    cout << "Surr Ver: ";
    for (auto const& i: s) {
        std::cout << i << " ";
    }
}

void selectVertex(vec3 ray, vec3 origin){
    if(ob.type==CUBE){
        cout << "Vertices Size: " << vertices1.size()/6 << endl;
        for (int o=0;o<vertices1.size()/6;o++){
            float b = dot((origin-vec3(vertices1[o*6],vertices1[o*6+1],vertices1[o*6+2])),ray);
            float c = dot(origin-vec3(vertices1[o*6],vertices1[o*6+1],vertices1[o*6+2]),origin-vec3(vertices1[o*6],vertices1[o*6+1],vertices1[o*6+2]))-0.1*0.1;
            float x = b*b-c;
            if((b*b-c)>0){
                cout << "Click Count: " << clickCount << endl;
                cout << "Get Vertex: " << o << endl;
                currentVertex = o;
                if(currentMode == 4) clickCount++;
                if(currentMode == 5) {
                    cout << "Start Udpate" << endl;
                    updateSurroundingVertex(indices_cube);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    cout << "surEdge Size: " << surrEdges.size() << endl;
                    cout << "Done update" << endl;
                    print(surrVertices);
                    cout << "m2: " << (int)*surrVertices.begin() << endl;
                    mergeVertices(currentVertex,(int)*surrVertices.begin(),vertices1,indices_cube);
                    cout << "vertices size: " << vertices1.size()/6 << endl;
                    cout << "indices size: " << indices_cube.size()/3 << endl;
                    VBO_CUBE.updateV(vertices1);
                    EBO_CUBE.updateE(indices_cube);
                    return;
                }

                if(clickCount==1) {
                    mergeVertexOne = o;
                    updateSurroundingVertex(indices_cube);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    print(surrVertices);
                } else if (clickCount==2){
                    if(surrVertices.find(o)==surrVertices.end()) {
                        cout << "Not in the surrounding vertices set!" << endl;
                        clickCount = 1;
                    }
                    else {
                        mergeVertexTwo = o;
                        mergeVertices(mergeVertexOne,mergeVertexTwo,vertices1,indices_cube);
                        cout << "indices Size: " << indices_cube.size() << endl;
                        VBO_CUBE.updateV(vertices1);
                        EBO_CUBE.updateE(indices_cube);
                        cout << "Merged!" << endl;
                        clickCount = 0;
                    }
                }
                return;
            }
        }
    }
    else if(ob.type==BUNNY){
        for (int o=0;o<vertices2.size()/6;o++){
            float b = dot((origin-vec3(vertices2[o*6],vertices2[o*6+1],vertices2[o*6+2])),ray);
            float c = dot(origin-vec3(vertices2[o*6],vertices2[o*6+1],vertices2[o*6+2]),origin-vec3(vertices2[o*6],vertices2[o*6+1],vertices2[o*6+2]))-0.001*0.001;
            float x = b*b-c;
            if((b*b-c)>0){
                currentVertex = o;
                cout << "Click Count: " << clickCount << endl;
                cout << "Get Vertex: " << o << endl;
                if(currentMode == 4) clickCount++;
                if(currentMode == 5) {
                    cout << "Start Udpate" << endl;
                    updateSurroundingVertex(indices_bunny);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    cout << "surEdge Size: " << surrEdges.size() << endl;
                    cout << "Done update" << endl;
                    print(surrVertices);
                    cout << "m2: " << (int)*surrVertices.begin() << endl;
                    mergeVertices(currentVertex,(int)*surrVertices.begin(),vertices2,indices_bunny);
                    cout << "Done Merge X" << endl;
                    VBO_BUNNY.updateV(vertices2);
                    EBO_BUNNY.updateE(indices_bunny);
                    return;
                }
                if(clickCount==1) {
                    mergeVertexOne = o;
                    updateSurroundingVertex(indices_bunny);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    print(surrVertices);
                }else if(clickCount==2){
                    if(surrVertices.find(o)==surrVertices.end()) {
                        cout << "Not in the surrounding vertices set!" << endl;
                        clickCount = 1;
                    }
                    else {
                        mergeVertexTwo = o;
                        mergeVertices(mergeVertexOne,mergeVertexTwo,vertices2,indices_bunny);
                        VBO_BUNNY.updateV(vertices2);
                        EBO_BUNNY.updateE(indices_bunny);
                        cout << "Merged!" << endl;
                        clickCount = 0;
                    }
                }
                return;
            }
        }
    }
    else if(ob.type==BUMPY_CUBE){
        for (int o=0;o<vertices3.size()/6;o++){
            float b = dot((origin-vec3(vertices3[o*6],vertices3[o*6+1],vertices3[o*6+2])),ray);
            float c = dot(origin-vec3(vertices3[o*6],vertices3[o*6+1],vertices3[o*6+2]),origin-vec3(vertices3[o*6],vertices3[o*6+1],vertices3[o*6+2]))-0.001*0.001;
            float x = b*b-c;
            if((b*b-c)>0){
                cout << "Click Count: " << clickCount << endl;
                cout << "Get Vertex: " << o << endl;
                currentVertex = o;
                if(currentMode == 4) clickCount++;
                if(currentMode == 5) {
                    cout << "Start Udpate" << endl;
                    updateSurroundingVertex(indices_bumpy);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    cout << "surEdge Size: " << surrEdges.size() << endl;
                    cout << "Done update" << endl;
                    print(surrVertices);
                    cout << "m2: " << (int)*surrVertices.begin() << endl;
                    mergeVertices(currentVertex,(int)*surrVertices.begin(),vertices3,indices_bumpy);
                    cout << "Done Merge X" << endl;
                    VBO_BUMPY.updateV(vertices3);
                    EBO_BUMPY.updateE(indices_bumpy);
                    return;
                }
                if(clickCount==1) {
                    mergeVertexOne = o;
                    updateSurroundingVertex(indices_bumpy);
                    cout << "surVer Size: " << surrVertices.size() << endl;
                    print(surrVertices);
                }else if(clickCount==2){
                    if(surrVertices.find(o)==surrVertices.end()) {
                        cout << "Not in the surrounding vertices set!" << endl;
                        clickCount = 1;
                    }
                    else {
                        mergeVertexTwo = o;
                        mergeVertices(mergeVertexOne,mergeVertexTwo,vertices3,indices_bumpy);
                        VBO_BUMPY.updateV(vertices3);
                        EBO_BUMPY.updateE(indices_bumpy);
                        cout << "Merged!" << endl;
                        clickCount = 0;
                    }
                }
                return;
            }
        }
    }

}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float xworld = ((xpos/float(width))*2)-1;
    float yworld = (((height-1-ypos)/float(height))*2)-1;
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS && currentMode==6 && currentVertex!=-1){
        float xDisp = xworld - cursorPosX;
        float yDisp = yworld - cursorPosY;
        if(ob.type==CUBE){
            vertices1[currentVertex*6]-=(xDisp*cameraRight.x+yDisp*normalize(cross(cameraRight,cameraDirection)).x);
            vertices1[currentVertex*6+1]-=(xDisp*cameraRight.y+yDisp*normalize(cross(cameraRight,cameraDirection)).y);
            vertices1[currentVertex*6+2]-=(xDisp*cameraRight.z+yDisp*normalize(cross(cameraRight,cameraDirection)).z);
            vector<float> vect{vertices1[currentVertex*6],vertices1[currentVertex*6+1]};
            VBO_CUBE.updateSubV(vect,currentVertex);

        }else if(ob.type==BUNNY){
            vertices2[currentVertex*6]-=(xDisp*cameraRight.x+yDisp*normalize(cross(cameraRight,cameraDirection)).x);
            vertices2[currentVertex*6+1]-=(xDisp*cameraRight.y+yDisp*normalize(cross(cameraRight,cameraDirection)).y);
            vertices2[currentVertex*6+2]-=(xDisp*cameraRight.z+yDisp*normalize(cross(cameraRight,cameraDirection)).z);
            vector<float> vect{vertices2[currentVertex*6],vertices2[currentVertex*6+1]};
            VBO_BUNNY.updateSubV(vect,currentVertex);
        }else if(ob.type==BUMPY_CUBE){
            vertices3[currentVertex*6]-=(xDisp*cameraRight.x+yDisp*normalize(cross(cameraRight,cameraDirection)).x);
            vertices3[currentVertex*6+1]-=(xDisp*cameraRight.y+yDisp*normalize(cross(cameraRight,cameraDirection)).y);
            vertices3[currentVertex*6+2]-=(xDisp*cameraRight.z+yDisp*normalize(cross(cameraRight,cameraDirection)).z);
            vector<float> vect{vertices3[currentVertex*6],vertices3[currentVertex*6+1]};
            VBO_BUMPY.updateSubV(vect,currentVertex);
        }
        cursorPosX = xworld;
        cursorPosY = yworld;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if (action == GLFW_RELEASE) return;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Get the size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Convert screen position to world coordinates
    float x = ((xpos/float(width))*2.0f)-1.0f;
    float y = 1.0f - (2.0f * ypos) / float(height); // NOTE: y axis is flipped in glfw
    cursorPosX = x;
    cursorPosY = y;
    float z = 3.0f;
    vec3 ray_nds = vec3(x, y, z);
    vec4 ray_clip = vec4(x, y, -1.0, 1.0);
    vec4 ray_eye = inverse(projection) * ray_clip;
    ray_eye = vec4(ray_eye.x,ray_eye.y, -1.0, 0.0);
    vec4 ray_tmp = inverse(view) * ray_eye;
    vec3 ray_wor = vec3(ray_tmp.x,ray_tmp.y,ray_tmp.z);
    ray_wor = normalize(ray_wor);
    selectVertex(ray_wor, cameraPos);
}

vector<float> parserFloat(string str){
    vector<float> v;
    string delimiter = " ";
    size_t pos = 0;
    string token;
    while ((pos = str.find(delimiter)) != string::npos) {
        token = str.substr(0, pos);
        v.push_back(stof(token));
        str.erase(0, pos + delimiter.length());
    }
    return v;
}

vector<int> parserInt(string str){
    vector<int> v;
    string delimiter = " ";
    size_t pos = 0;
    string token;
    int count =0;
    while ((pos = str.find(delimiter)) != string::npos) {
        if(count==0) {
            count++;
            str.erase(0, pos + delimiter.length());
            continue;
        }
        token = str.substr(0, pos);
        v.push_back(stoi(token));
        str.erase(0, pos + delimiter.length());
    }
    v.push_back(stoi(str));
    return v;
}

vector<int> parserIntOne(string str){
    vector<int> v;
    string delimiter = " ";
    size_t pos = 0;
    string token;
    while ((pos = str.find(delimiter)) != string::npos) {
        token = str.substr(0, pos);
        v.push_back(stoi(token));
        str.erase(0, pos + delimiter.length());
    }
    v.push_back(stoi(str));
    return v;
}

void loadMesh(vector<float>& vertices, vector<int>& indices, string filepath){
    cout << "New Mesh ! ! ! " << endl;
    ifstream in(filepath);
    if(!in){
        cout << "Failed: The target file cannot be opened." << endl;
    }
    string str;
    int count = 0;
    int numV=0;
    int numF=0;
    vector<float> tempV;
    vector<int> vf;
    vector<int> tempF;
    while (getline(in, str)) {
        if(count == 0){
            count++;
            continue;
        }
        if(count == 1){
            vf = parserIntOne(str);
            numV = vf[0];
            numF = vf[1];
        }
        else if(count < numV+2){
            tempV = parserFloat(str);
            vertices.push_back(tempV[0]);
            vertices.push_back(tempV[1]);
            vertices.push_back(tempV[2]);
            //cout << tempV[0]  << " " << tempV[1] << " "<< tempV[2] << endl;
        }
        else{
            tempF = parserInt(str);
            indices.push_back(tempF[0]);
            indices.push_back(tempF[1]);
            indices.push_back(tempF[2]);
            //cout << tempF[0]  << " " << tempF[1] << " "<< tempF[2] << endl;
        }
        count++;
    }
    cout << "Done!" << endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return;
    float temp;
    switch (key)
    {
        case GLFW_KEY_UP:
            if(camMode==0) cameraPos+=cameraFront*camFrontSpeed;
            else cameraPos+=cameraUp*camFrontSpeed;
            break;
        case GLFW_KEY_DOWN:
            if(camMode==0) cameraPos-=cameraFront*camFrontSpeed;
            else cameraPos-=cameraUp*camFrontSpeed;
            break;
        case GLFW_KEY_LEFT:
            cameraPos-=cameraXRight*camFrontSpeed;
            break;
        case GLFW_KEY_RIGHT:
            cameraPos+=vec3(cameraXRight)*camFrontSpeed;
            break;
        case GLFW_KEY_W:
            if(currentMode == 0){
                temp=cameraPos.y;
                cameraPos.y=cos(radians(-camSpeed))*cameraPos.y-sin(radians(-camSpeed))*cameraPos.z;
                cameraPos.z=sin(radians(-camSpeed))*temp+cos(radians(-camSpeed))*cameraPos.z;
            }else if(currentMode == 1){
                mat4 a = mat4(1.0f);
                mat4 b = mat4(1.0f);
                mat4 c = mat4(1.0f);
                a = translate(a,-ob.centroid);
                b = rotate(b, radians(rotateSpeed), vec3(1,0,0));
                c = translate(c,ob.centroid);
                ob.model = c*b*a*ob.model;
            }else if(currentMode == 2){
                ob.model[3][1]+=transSpeed;
                ob.centroid.y+=transSpeed;
            }else if(currentMode == 3){
                ob.model = scale(ob.model,vec3(1+scaleSpeed,1+scaleSpeed,1+scaleSpeed));
                ob.radius*=(1+scaleSpeed);
            }
            break;
        case GLFW_KEY_S:
            if(currentMode == 0){
            temp=cameraPos.y;
            cameraPos.y=cos(radians(camSpeed))*cameraPos.y-sin(radians(camSpeed))*cameraPos.z;
            cameraPos.z=sin(radians(camSpeed))*temp+cos(radians(camSpeed))*cameraPos.z;
            }else if(currentMode == 1){
                mat4 a = mat4(1.0f);
                mat4 b = mat4(1.0f);
                mat4 c = mat4(1.0f);
                a = translate(a,-ob.centroid);
                b = rotate(b, radians(-rotateSpeed), vec3(1,0,0));
                c = translate(c,ob.centroid);
                ob.model = c*b*a*ob.model;
            }else if(currentMode == 2){
                ob.model[3][1]-=transSpeed;
                ob.centroid.y-=transSpeed;
            }else if(currentMode == 3){
                ob.model = scale(ob.model,vec3(1/(1+scaleSpeed),1/(1+scaleSpeed),1/(1+scaleSpeed)));
                ob.radius*=(1/(1+scaleSpeed));
            }
            break;
        case GLFW_KEY_A:
            if(currentMode == 0){
            temp=cameraPos.x;
            cameraPos.x=cos(radians(camSpeed))*cameraPos.x-sin(radians(camSpeed))*cameraPos.z;
            cameraPos.z=sin(radians(camSpeed))*temp+cos(radians(camSpeed))*cameraPos.z;
            }else if(currentMode == 1){
                mat4 a = mat4(1.0f);
                mat4 b = mat4(1.0f);
                mat4 c = mat4(1.0f);
                a = translate(a,-ob.centroid);
                b = rotate(b, radians(-rotateSpeed), vec3(0,1,0));
                c = translate(c,ob.centroid);
                ob.model = c*b*a*ob.model;
            }else if(currentMode == 2){
                ob.model[3][0]-=transSpeed;
                ob.centroid.x-=transSpeed;
            }
            break;
        case GLFW_KEY_D:
            if(currentMode == 0){
                temp=cameraPos.x;
                cameraPos.x=cos(radians(-camSpeed))*cameraPos.x-sin(radians(-camSpeed))*cameraPos.z;
                cameraPos.z=sin(radians(-camSpeed))*temp+cos(radians(-camSpeed))*cameraPos.z;
            }else if(currentMode == 1){
                mat4 a = mat4(1.0f);
                mat4 b = mat4(1.0f);
                mat4 c = mat4(1.0f);
                a = translate(a,-ob.centroid);
                b = rotate(b, radians(rotateSpeed), vec3(0,1,0));
                c = translate(c,ob.centroid);
                ob.model = c*b*a*ob.model;
            }else if(currentMode == 2){
                ob.model[3][0]+=transSpeed;
                ob.centroid.x+=transSpeed;
            }
            break;
        case GLFW_KEY_1:{
            Object o;
            meshType t = CUBE;
            o.init(t,0);
            ob = o;
            break;
        }
        case GLFW_KEY_2:{
            Object o;
            meshType t = BUNNY;
            o.init(t,0);
            mat4 model = mat4(1.0f);
            ob = o;
            break;
            }
        case GLFW_KEY_3:{
            Object o;
            meshType t = BUMPY_CUBE;
            o.init(t,0);
            ob = o;
            break;
            }
        case GLFW_KEY_V:
            currentMode = 0;
            break;
        case GLFW_KEY_R:
            currentMode = 1;
            break;
        case GLFW_KEY_T:
            currentMode = 2;
            break;
        case GLFW_KEY_Y:
            currentMode = 3;
            break;
        case GLFW_KEY_MINUS:
            projectionMode = 0;
            break;
        case GLFW_KEY_EQUAL:
            projectionMode = 1;
            break;
        case GLFW_KEY_P:
            camMode = 0;
            break;
        case GLFW_KEY_L:
            camMode = 1;
            break;
        case GLFW_KEY_O:
            cameraPos+=cameraZFront*camFrontSpeed;
            break;
        case GLFW_KEY_K:
            cameraPos-=cameraZFront*camFrontSpeed;
            break;
        case GLFW_KEY_SPACE:
            ob.color.z+=0.05;
            break;
        case GLFW_KEY_M:
            currentMode = 4;
            break;
        case GLFW_KEY_X:
            //EBO_CUBE.updateE(indices_bumpy);
            currentMode = 5;
            break;
        case GLFW_KEY_Z:
            currentVertex=-1;
            currentMode = 6;
            break;
        case GLFW_KEY_U:
            if(ob.type==CUBE)updateNormals(vertices1,indices_cube);
            else if(ob.type==BUNNY)updateNormals(vertices2,indices_bunny);
            else if(ob.type==BUMPY_CUBE)updateNormals(vertices3,indices_bumpy);
            break;
        default:
            break;
    }
}

vec3 calTriangleNorm(vec3 a, vec3 b, vec3 c){
    vec3 n = normalize(cross((b-a),(c-a)));
    return n;
}

vector<vec3> getVertexNorms(vector<float>& vertices,vector<int>& indices){
    vector<vec3> vertexNormals(vertices.size()/3,vec3(0.0,0.0,0.0));
    for(int i=0;i<indices.size();i+=3){

        vec3 n = calTriangleNorm(vec3(vertices[indices[i]*3],vertices[indices[i]*3+1],vertices[indices[i]*3+2]),
                                 vec3(vertices[indices[i+1]*3],vertices[indices[i+1]*3+1],vertices[indices[i+1]*3+2]),
                                 vec3(vertices[indices[i+2]*3],vertices[indices[i+2]*3+1],vertices[indices[i+2]*3+2]));
        vertexNormals[indices[i]]+=n;
        vertexNormals[indices[i+1]]+=n;
        vertexNormals[indices[i+2]]+=n;
        //cout << "index " << i/3 << " " << to_string(n).c_str() << endl;
        //faceNormals.push_back(n);
    };
    for(int i=0;i<vertexNormals.size();i++){
        vertexNormals[i] = normalize(vertexNormals[i]);
    }
    return vertexNormals;
}
bool checkIfInsideSurE(int a, int b){
    for(vector<int>& e: surrEdges){
        if((e[0]==a && e[1]==b) || (e[0]==b && e[1]==a)) return true;
    }
    return false;
}
void updateSurroundingVertex(vector<int>& indices){
    vector< vector<int> > surrE;
    unordered_set<int> surrV;
    for(int i=0;i<indices.size();i++){
        if(currentVertex==indices[i]){
            if(i%3==0) {
                if(checkIfInsideSurE(indices[i+1],indices[i+2])) continue;
                surrE.push_back(vector<int>{indices[i+1],indices[i+2]});
                surrV.insert(indices[i+1]);
                surrV.insert(indices[i+2]);
            }
            else if(i%3==1) {
                if(checkIfInsideSurE(indices[i-1],indices[i+1])) continue;
                surrE.push_back(vector<int>{indices[i-1],indices[i+1]});
                surrV.insert(indices[i-1]);
                surrV.insert(indices[i+1]);
            }
            else if(i%3==2) {
                if(checkIfInsideSurE(indices[i-2],indices[i-1])) continue;
                surrE.push_back(vector<int>{indices[i-2],indices[i-1]});
                surrV.insert(indices[i-2]);
                surrV.insert(indices[i-1]);
            }
        }
    }
    surrEdges = surrE;
    surrVertices = surrV;
}

bool checkIfInsideIndices(vector<int>& indices, int a, int b, int c){
    for(int i=0;i<indices.size()/3;i++){
        set<int> s1{indices[i],indices[i+1],indices[i+2]};
        set<int> s2{a,b,c};
        if(s1==s2) return true;
    }
    return false;
}

void mergeVertices(int a, int b, vector<float>& vertices, vector<int>& indices){
    for(vector<int> edge: surrEdges){
//        cout << "edge: " << edge[0] << " " << edge[1] << endl;
        if(b == edge[0] || b == edge[1]) continue;
        if(checkIfInsideIndices(indices, edge[0],edge[1],b)) continue;
        indices.push_back(edge[0]);
        indices.push_back(edge[1]);
        indices.push_back(b);
    }
    cout << "Added New Faces." << endl;
    for(int i=0;i<indices.size();i++){
        //cout << "indices i: " << i << endl;
        if(a==indices[i]){
            indices.erase(indices.begin()+i/3*3,indices.begin()+i/3*3+3);
            if(i%3==0) i--;
            else if(i%3==1) i-=2;
            else i-=3;
        }
    }
    //for(int i=0;i<indices.size()/3;i++){cout << "Face " << i << ": " << indices[i*3] << " " << indices[i*3+1] << " " << indices[i*3+2] << endl;}
    cout << "Erased old Faces." << endl;
    vertices.erase(vertices.begin()+a*6, vertices.begin()+a*6+6);
    cout << "vertices size: " << vertices.size()/6 << endl;
    //for(int i=0;i<vertices.size()/6;i++) cout << "vertex " << i << ": " << vertices[i*6] << " " << vertices[i*6+1] << " " << vertices[i*6+2] << endl;
    cout << "Erased Vertex: " << a << endl;
    for(int i=0;i<indices.size();i++) if(indices[i]>=a) indices[i]--;
    updateNormals(vertices,indices);
    //for(int x: indices) cout << x << endl;
    cout << "Done Merge" << endl;
}

void updateNormals(vector<float>& vertices, vector<int>& indices){
    vector<vec3> vertexNormals(vertices.size()/6,vec3(0.0,0.0,0.0));
    for(int i=0;i<indices.size();i+=3){

        vec3 n = calTriangleNorm(vec3(vertices[indices[i]*6+3],vertices[indices[i]*6+4],vertices[indices[i]*6+5]),
                                 vec3(vertices[indices[i+1]*6+3],vertices[indices[i+1]*6+4],vertices[indices[i+1]*6+5]),
                                 vec3(vertices[indices[i+2]*6+3],vertices[indices[i+2]*6+4],vertices[indices[i+2]*6+5]));
        vertexNormals[indices[i]]+=n;
        vertexNormals[indices[i+1]]+=n;
        vertexNormals[indices[i+2]]+=n;
        //cout << "index " << i/3 << " " << to_string(n).c_str() << endl;
    };
    for(int i=0;i<vertexNormals.size();i++){
        vertexNormals[i] = normalize(vertexNormals[i]);
        vertices[6*i+3] = vertexNormals[i].x;
        vertices[6*i+4] = vertexNormals[i].y;
        vertices[6*i+5] = vertexNormals[i].z;
    }
}
