# This is Francisco Barrio's first DirectX assignment.

The github is available in the following link: https://github.com/franchito55/Master-Engine

This engine is coded in C++, using DirectX12 and ImGui for the editor's GUI. At the moment, there is only a main window with the editor, and a quad with a texture.

## Camera
The controls are those specified in the assignment, but for anyone who hasn't read them:
	- While holding RMB, you can move the camera using WASD to move forwads, backwards, left and right, and Space/Control to go up and down. You can also move the mouse to rotate the camera around itself in this mode. In this mode, holding Left Shift will double the speed at which you move.
	- While holding Alt + LMB, you can orbit the camera around the target by using the mouse.
	- The mouse wheel controls the zoom (a.k.a. the distance to the camera's target). Scroll up to zoom in, scroll down to zoom out.
	- The F key places your camera's target at (looks at) the quad's position.
 
You can also control some aspects of the camera using the "Camera" ImGui tab on the main debugging window (left).
- Vectors: this contains information about the:
  - Position
  - Forward
  - Up
  - Target
  - And also the distance to the target
- Parameters: this contains other parameters related to the camera and camera movement, such as the FOV, or different movement speeds.
 
All these values can be modified in real time, although it is not recommended to modify the forward and up vectors of the camera manually. Although, if you do and can't go back, you can just hit F and the camera will focus on the quad and these vectors will be recalculated.

Also, keep in mind that there is no limit to the rotation or orbiting of the camera in the Y values. This is a deliberate design decision as I wanted to give the freedom to do it to the user if they want to.

## Other ImGui parameters
Aside from being able to modify the camera, there are other features you can modify with ImGui. First off, on the right side of the window there is a window with some performance info. Then, there are 3 other tabs in the same window as the camera's information panel.
### Debug info:
  - Here, you can select whether to see some visual information related to debugging, such as the X-Z plane grid to get a sense of scale, the world axis at (0, 0, 0) to check the positive directions of each axis, and a small sphere in the camera target's position.
### Texture info:
  - Here, you can change 2 of the texture sampler's parameters, the Filtering mode and the Addressing mode. These changes, again, can be seen in real time.
### Geometry:
  - Here you can control the position of the quad in the world.
	
## Additional comments
  - I decided to not include a ModuleEditor class, since it didn't really make sense to me to have a module to effectively execute only 1 line on the render method, especially since any class can create an ImGui window as its methods are static. As such, the rendering of ImGui is done in the ModuleD3D12 class, which makes the fact that it should always be rendered last, easier to control.
  - There are some small details I probably spent too much time on, for instance, making the camera ImGui parameters be bidirectionally editable, or make the zoom not be able to trespass the target if you scroll the mouse wheel too hard, which probably wouldn't have mattered for this assignment.
  - Lastly, for some reason, in my personal computer, Alt + Enter enters fullscreen mode, even though it is apparently not implemented (and in fact it doesn't work on the uni's PCs). This is probably something automatically implemented in my GPU and will most likely not work on other PCs until I implement it.
  - The shaders are called "Exercise2VS" and "Exercise2PS", ignore these names as I just didn't bother changing them since the exercise 2.
