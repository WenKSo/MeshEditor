# MeshEditor
The mesh editor is able to render complex meshes and allow the user to select and edit vertices on these meshes. For more information about the vertices editing, please check the "Mesh Editor Report" inside the ME folder. 

## Compilation instructions
In the terminal, after make in the build directory
go to the build directory:

run the file "MeshEditor_bin"

## Scene Editor
Press "1" to create a unit cube
Press "2" to create a armadillo
Press "3" to create a bunny
Press "X" to delete a selected object 

## Object Control

Press "V" to start the view mode
Using mouse to select the object, the wireframe object will become flat shading object
Then
Press "R" for rotate mode
Press "T" for translate mode
Press "Y" for scale mode
After that you can press"WSAD" to control your selected object.
Press "SPACE" to change color

## Camera Control
Press "L" to start regular camera mode
Press "V" to start the view mode
Press "UP" and "DOWN" to move your camera up and down
Press "LEFT" and "RIGHT" to move your camera left and right 
Press "O" and "K" to move your camera along Z axis
Press "-" for perspective projection mode
Press "+" for orthographical projection mode

## Trackball
Press "P" to be trackball mode
Press "V" to start the view mode
You can press"UP" and "DOWN" to zoom in & zoom out
After that you can press"WSAD" to move camera around the trackball

## Notice

One thing I have to mention is for my laptop, my directory is the user directiory (sorry I don't know how that happens). I copy and paste my "data" folder to my user directory so I can read the .off files. If it causes seg fault, please change the string in loadmesh() to "data/cube.off" and make a copy to your user directory.
