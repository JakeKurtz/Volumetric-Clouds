DESCRIPTION:
CPSC 591/691 Final project Fall 2020

RUN INSTRUCTIONS:
Open the .sln file with visual studio and change from x86 to x64. Then run the program

CONTROLS:

	- WASD + Space + Shift: move camera
	- Mouse and Left_MSB : look around
	
OPTIONS:

	Resolution Scale:
		Down scales the resolution, to increase performance, if needed.
	Maximum Raymarch Steps:
		Limits the number of steps per ray. Can increase performance, if needed. 
		But doesn't look good at low values.
	Animation Speed:
		Speed of the clouds.
	
Lighting Settings:
	
	Light Color:
		Main light color.
	Ambient Color:
		Ambient light color.
	
	Sky Top Color:
		The top color of the sky.
	Sky Bottom Color:
		The bottom color of the sky.
	
	Ambient Blend:
		The intensity of the ambient blending. The region of the clouds 
		that are not being illuminated by the main light source have there opacancy turned down to 
		blend them with the background color.
		
	Light Intensity:
		Initensity of the main light source.
	
	g: 
		This term is related to the Henyey-Greenstein phase function. 
		-1: back scattering
		 0: isotropic scattering
		 1: forward scattering
	
	xpos:
	ypos:
	zpos:
		Light position
	
Cloud Settings:

	Density:
		Cloud density
	Top Density:
		Controls the density transition towards the top of the clouds.
		0.01: Maximum blend intensity
		1: Uniform density 
	Bottom Density:
		Controls the density transition towards the bottom of the clouds.
		0.01: Uniform density
		1: Maximum blend intensity
	
	Thickness:
		Cloud thickness
	Top Roundness:
		Controls the curvature transition towards the top of the clouds.
		0: Completely round
		1: Flat top
	Bottom Roundness:
		Controls the curvature transition towards the bottom of the clouds.
		0: Flat bottom
		1: Completely round
	
	Coverage Noise Scale:
		Size of the coverage noise texture.
	Shape Noise Scale:
		Size of the shape noise texture.
		A large value for this will slow things down.
	Detail Noise Scale:
		Size of the detail noise texture.
	
	Shape Noise Intensity:
	Detail Noise Intensity:
	
Presets:
	Just some preset settings to give an overview of the different types of clouds.