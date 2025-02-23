///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/

void SceneManager::LoadSceneTextures() {
	// load the texture images and convert to OpenGL texture data
	bool bReturn = false;

	// load the textures for the 3D scene
	bReturn = CreateGLTexture("Textures/cloud.jpg", "cloud");
	bReturn = CreateGLTexture("Textures/fire.jpg", "fire");
	bReturn = CreateGLTexture("Textures/metal.jpg", "metal");
	bReturn = CreateGLTexture("Textures/mud.jpg", "mud");
	bReturn = CreateGLTexture("Textures/seashells.jpg", "seashells");
	bReturn = CreateGLTexture("Textures/soil.jpg", "soil");
	bReturn = CreateGLTexture("Textures/stainedglass.jpg", "stainedglass");
	bReturn = CreateGLTexture("Textures/stone.jpg", "stone");
	bReturn = CreateGLTexture("Textures/treebark.jpg", "treebark");
	bReturn = CreateGLTexture("Textures/wood.jpg", "wood");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture images and convert to OpenGL texture data
	LoadSceneTextures();

	// Define the object materials for the 3D scene
	DefineObjectMaterials();

	// Set up scene lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh(); // Load the plane mesh
	m_basicMeshes->LoadTaperedCylinderMesh(); // Load the tapered cylinder mesh
	m_basicMeshes->LoadTorusMesh(); // Load the torus mesh
	m_basicMeshes->LoadBoxMesh(); // Load the box mesh
	m_basicMeshes->LoadCylinderMesh(); // Load the cylinder mesh
	m_basicMeshes->LoadConeMesh(); // Load the cone mesh
	m_basicMeshes->LoadPrismMesh(); // Load the prism mesh
	m_basicMeshes->LoadPyramid4Mesh(); // Load the pyramid mesh
	m_basicMeshes->LoadSphereMesh(); // Load the sphere mesh
}

void SceneManager::DefineObjectMaterials() {
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);
}

void SceneManager::SetupSceneLights() {
	// Enable lighting in shaders
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light 1 - White Key Light (Main Light Source)
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.02f, 0.05f, 0.05f); // Slightly cool ambient
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.3f, 0.2f); 
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.4f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f);

	// Light 2 - White Fill Light (Softens Shadows)
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 10.0f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.02f, 0.02f, 0.02f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 25.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.3f);

	// Light 3 - Warm Colored Light (Adds Warmth and Color)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.03f, 0.02f, 0.01f); // Slightly warm ambient
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.9f, 0.6f, 0.2f);  // Orange-yellow tone
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.4f, 0.3f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 18.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.6f);

	// Light 4 - Cool Blue Back Light (Adds Depth)
	m_pShaderManager->setVec3Value("lightSources[3].position", -4.0f, 8.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.01f, 0.01f, 0.03f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.2f, 0.4f, 1.0f); // Cool blue light
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.3f, 0.4f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.7f);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 model;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.3, 0.2, 1.0); // Updated floor color to brown to resemble a table
	SetShaderTexture("wood"); // Set the texture for the floor
	SetShaderMaterial("wood"); // Set the material for the floor

	SetTextureUVScale(5.0f, 5.0f); // repeat the texture 5 times in the U and V directions to create a tiled floor

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	
	// Create a coffee cup using tapered cylinder (Parent Object)
	glm::mat4 cupTransform = glm::mat4(1.0f); // cup transformation matrix
	cupTransform = glm::translate(cupTransform, glm::vec3(6.0f, 1.12f, 7.0f)); // cup base position to be on the table	
	cupTransform = glm::rotate(cupTransform, glm::radians(160.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // cup rotation to make it stand upright
	cupTransform = glm::scale(cupTransform, glm::vec3(0.4f, 1.1f, 0.4f)); // cup scale to make it taller

	m_pShaderManager->setMat4Value("model", cupTransform); // set the cup transformation matrix
	//SetShaderColor(1.0, 0.0, 0.0, 1.0); // cup color to red
	SetShaderTexture("stainedglass"); // Set the texture for the cup
	SetShaderMaterial("glass"); // Set the material for the cup
	m_basicMeshes->DrawTaperedCylinderMesh(); // Draw the tapered cylinder mesh

	// Create a torus for the handle of the cup (Child Object)
	glm::mat4 handleTransform = cupTransform; // handle transformation matrix
	handleTransform = glm::translate(handleTransform, glm::vec3(1.0f, 0.5f, 0.0f)); // handle position to be on the cup
	handleTransform = glm::rotate(handleTransform, glm::radians(90.0f), glm::vec3(0.0f, 10.0f, 90.0f)); // handle rotation to make it stand upright
	handleTransform = glm::scale(handleTransform, glm::vec3(0.2f, 0.2f, 0.1f)); // handle scale to make it smaller

	m_pShaderManager->setMat4Value("model", handleTransform); // set the handle transformation matrix
	//SetShaderColor(0.0, 0.0, 1.0, 1.0); // handle color to blue
	SetShaderTexture("stainedglass"); // Set the texture for the handle
	m_basicMeshes->DrawTorusMesh(); // Draw the torus mesh

	// Add Computer Monitor
	AddComputerMonitor(glm::vec3(0.5f,1.5f, 2.0f));

	// Add Pencil
	AddPencil(glm::vec3(-5.0f, 0.1f, 7.0f));

	// Add Stack of Books
	AddStackOfBooks(glm::vec3(-9.0f, 1.3f, 7.2f));
}

void SceneManager::AddComputerMonitor(glm::vec3 position) {
	// Monitor body (Box)
	SetTransformations(glm::vec3(8.0f, 3.0f, 0.1f), 0.0f, 0.0f, 0.0f, position);
	SetShaderTexture("cloud"); // Cloud texture for monitor frame
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::AddPencil(glm::vec3 position) {
	// Pencil body (Cylinder)
	SetTransformations(glm::vec3(0.05f, 1.5f, 0.05f), 0.0f, 0.0f, 90.0f, position);
	SetShaderTexture("wood"); // Wooden texture for pencil body
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();

	// Metal band near eraser (Small Cylinder)
	SetTransformations(glm::vec3(0.05f, 0.05f, 0.05f), 0.0f, 0.0f, 90.0f, position + glm::vec3(0.05f, 0.0f, 0.0f));
	SetShaderTexture("metal"); // Metal texture for band
	SetShaderMaterial("gold");
	m_basicMeshes->DrawCylinderMesh();

	// Eraser (Small Cylinder)
	SetTransformations(glm::vec3(0.05f, 0.2f, 0.05f), 0.0f, 0.0f, 90.0f, position + glm::vec3(0.25f, 0.0f, 0.0f));
	SetShaderTexture("fire"); // Red texture for eraser
	SetShaderMaterial("clay");
	m_basicMeshes->DrawCylinderMesh();
}

void SceneManager::AddStackOfBooks(glm::vec3 position) {
		SetTransformations(glm::vec3(1.5f, 0.3f, 1.0f), 0.0f, 0.0f, 0.0f, position + glm::vec3(0.1f, -1.1f, 0.0f));
		SetShaderTexture("fire"); 
		SetShaderMaterial("clay");
		m_basicMeshes->DrawBoxMesh();

		SetTransformations(glm::vec3(1.5f, 0.3f, 1.0f), 0.0f, 0.0f, 0.0f, position + glm::vec3(0.0f, -0.8f, 0.0f));
		SetShaderTexture("metal");
		SetShaderMaterial("clay");
		m_basicMeshes->DrawBoxMesh();

		SetTransformations(glm::vec3(1.5f, 0.3f, 1.0f), 0.0f, 0.0f, 0.0f, position + glm::vec3(0.0f, -0.5f, 0.1f));
		SetShaderTexture("seashells");
		SetShaderMaterial("clay");
		m_basicMeshes->DrawBoxMesh();
}
