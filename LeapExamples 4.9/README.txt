LeapExamples Project
====================

This project requires Leap Motion Controller plugin to be installed in your Engine. This is a blueprint-only project, and requires no code. It contains four demo levels, please open each by opening the Game|Maps folder in the Content Browser & double clicking the map asset.

To interact with the demos have your physical Leap Motion controller connected to your computer. Run the demos & just wave your hands above the device.


Collisions Demo
---------------

This is a basis scene that shows collision interaction of the leap hands with a physical scene. 

With your controller connected before 

Leap Motion controller got connected by a simple drag & drop of a LeapMotionControllerActor from a class list onto the scene. The default settings create a simple physical model & a crude visual hand representation. 

Visual cubes are StaticMeshActors with physics simulation enabled.


Custom Controller Demo
----------------------

This demo shows basic customization of the behavior of the Leap Motion Controller Actor. It uses LM_DemoControllerActor and LM_DemoBoneActor located in Games|Demo folder. Those blueprints derive from the corresponding C++ classes defined in the Leap Motion Controller plugin.

LM_DemoControllerActor overrides OnHandAdded/Removed events and uses them to print out simple info about the hand's id. LM_DemoBoneActor, overrides it's tick event to periodically output it's index finger's position. The LM_DemoBoneActor is then attached to the controller actor in the controller's settings pane, via the Bone Blueprint property.


Multiple Controllers Demo
-------------------------

This is a simplest demo showing use of multiple LeapMotionControllerActor object to create multiple instances of hands.


Rigged Hands Demo
-----------------

This demo uses rigged hand assets located in Game|Rigged folder. LM_RiggedHandActor derives from the LeapMotionHandActor C++ class from the Leap Motion Controller plugin. It simply contains a PoseableMeshComponent with the hand model & implements the tick event to drive it using the positions of the physical bones.

The actor contains a helper SceneComponent to help align the mesh's elbow position at the object's origin. In the tick, the script places the PoseableMeshComponent at the location of the elbow of the hand, and then only drive's each of the bone's global rotations's.

This works well enough, althogh you may notice that the rigged model's bones need to correspond in size to those of the physical hand, otherwise the two won't overlap. Once could add bone scaling to fix that, however that's not implemented here.

Disable the LeapMotionControllerActor's Show Collider & Show Mesh properties to hide the physical bone visualization.

This demo uses a single hand model, created for the right hand. The left hand is then created by mirroring the model & displaying it with a two-sided shader. The LM_RiggedHandActor has a few more comments in it's tick script.
