Resources



### Initial steps
- Spatial grid working. Visualized on top of the grid.
- https://matthias-research.github.io/pages/tenMinutePhysics/11-hashing.pdf
- https://www.youtube.com/watch?v=sx4IIQL0x7c 
- https://www.youtube.com/watch?v=eED4bSkYCB8
![Particle Simulation](resources/1.gif)
### Testing with performance
- If i remember correctly this was mainly limited by raylib rendering, which is not instanced. ( the basic renderCircle )
- https://en.wikipedia.org/wiki/Elastic_collision
- http://www.r-5.org/files/books/computers/algo-list/realtime-3d/Ian_Millington-Game_Physics_Engine_Development-EN.pdf 
![Particle Simulation 2](resources/2.gif)
### Improving rendering performance with frustum culling 
- Also started moving to 3D in order to improve rendering even further with instanced rendering.

![Particle Simulation 3](resources/3.gif)
- Initial instanced draw test with bounds. No collisions.
![Particle Simulation 4](resources/4.gif)
### Physics update
- http://www.r-5.org/files/books/computers/algo-list/realtime-3d/Ian_Millington-Game_Physics_Engine_Development-EN.pdf chapter 7
- Mass
![Particle Simulation 5](resources/5.gif)
### Playing with adding behavior to entities
![Particle Simulation 6](resources/6.gif)
![Particle Simulation 7](resources/7.gif)
### Hot reload inital testing
![Hot Simulation 1](resources/hot1.gif)
### Hot reload and input playback
- Playing back recorded input while testing hotreloading
![Hot Playback](resources/hotPlayback.gif)
### Hello triangle with vulkan
![Vulkan Simulation 1](resources/vulkan1.gif)
### Learning basic transforms with vulkan
![Vulkan Transforms](resources/vulkanTransforms.gif)