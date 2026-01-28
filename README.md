# This is Francisco Barrio's first DirectX assignment.

The github is available in the following link: https://github.com/franchito55/Master-Engine, in branch assignment2

This engine is coded in C++, using DirectX12 and ImGui for the editor's GUI. At the moment, there is main window where the scene is rendered, and a 3D model of a duck loaded via GLTF, with a texture. This model is shaded using a PBR shader with a Phong BRDF.

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

## Model
In this second assignment, you can also modify some of the model's properties, such as the position, rotation, and scale. This can be done by a set of ImGui sliders in a window called "Geometry", or via a Gizmo in the scene window.

## Other ImGui parameters
Aside from being able to modify the camera, there are other features you can modify with ImGui. First off, on the bottom right of the window there is a window with some performance info. Then, there are 3 other tabs in the same window as the camera's information panel.
### Debug info:
  - Here, you can select whether to see some visual information related to debugging, such as the X-Z plane grid to get a sense of scale, the world axis at (0, 0, 0) to check the positive directions of each axis, and a small sphere in the camera target's position. You can also disable the gizmo that controls the model.
### Texture info:
  - Here, you can change 2 of the texture sampler's parameters, the Filtering mode and the Addressing mode. These changes, again, can be seen in real time.
### Geometry:
  - In this second assignment, you can also modify some of the model's properties, such as the position, rotation, and scale. This can also be done via a Gizmo in the scene window.
	
## Additional comments
  - In addition to everything else, I had to restructure the classes following more closely what was in the slides. However, I still didn't have time to properly implement everything we saw.
  - For some reason, I have gone over the PBR pixel shader many times, and I'm pretty sure I implemented the formula for the Phong BRDF we saw in class to the T. However I still don't feel like it looks how it should, so I also included a "Light intensity" slider which is just a multiplier for Li in the formula, so the lighting doesn't look as dark. Set this to 1.0 to see the default look.
### What WASN'T implemented
  - ModuleRingBuffer. Currently, the data that the shader needs is just passed via a bunch of Upload Heap buffers, and the data is passed directly each frame via a bunch of SetGraphicsRootConstantBufferView calls.
  - Deferred release: just not enough time.
