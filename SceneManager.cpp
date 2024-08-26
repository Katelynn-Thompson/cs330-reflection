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

// declaration of global variables and defines
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
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
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.ambientStrength = 0.5f;
	plasticMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plasticMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.shininess =60.0;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL clothMaterial;
	clothMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	clothMaterial.ambientStrength = 0.7f;
	clothMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	clothMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Cloth is less shiny
	clothMaterial.shininess = 10.0;
	clothMaterial.tag = "cloth";

	m_objectMaterials.push_back(clothMaterial);

	OBJECT_MATERIAL aluminumMaterial;
	aluminumMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	aluminumMaterial.ambientStrength = 0.5f;
	aluminumMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	aluminumMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	aluminumMaterial.shininess = 90.0;
	aluminumMaterial.tag = "aluminum";

	m_objectMaterials.push_back(aluminumMaterial);

}
/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Glass.png", "Frog");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Keyboard.jpg", "Base");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Body.jpg", "Body");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Screen.png", "Screen");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Wood.jpg", "Desk");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Redbull.png", "Can");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/mouse.jpg", "Mouse");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/headphone.jpg", "Headphone");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Cushion.jpg", "Cushion");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Buttons.jpg", "Buttons");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/cantop.jpg", "Top");
	bReturn = CreateGLTexture(
		"C:/Users/katel/Downloads/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Debug/Texture/Wheel.jpg", "Wheel");

	
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
    // Enable custom lighting
    m_pShaderManager->setBoolValue(g_UseLightingName, true);

		// Sunlight from the right window
		m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 15.0f, -5.0f); // Positioned high and to the right
		m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.6f, 0.55f, 0.5f); // Slightly warm ambient light
		m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.95f, 0.85f); // Warm and bright diffuse light
		m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 0.9f); 
		m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
		m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.7f); // High specular intensity

		// Overhead light (Overhead light)
		m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 10.0f, 0.0f); // Positioned directly above
		m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.5f, 0.5f, 0.5f); // Neutral ambient light
		m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.7f, 0.7f, 0.8f); 
		m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.6f, 0.6f, 0.7f);
		m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
		m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f); 

		// General ambient light for overall brightness
		m_pShaderManager->setVec3Value("ambientLight.color", 0.5f, 0.5f, 0.55f); // Slightly cool ambient light
		m_pShaderManager->setFloatValue("ambientLight.intensity", 1.0f); // Moderate intensity 
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();


	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// Desk Surface
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.5f, 0.4f, 1.0f); // Dark forest green/teal color for the desk surface

	SetShaderTexture("Desk");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");
	
	// Draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/******************************************************************/
	// Desk Side L
	scaleXYZ = glm::vec3(1.3f, 4.5f, 16.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-20.7f, 2.0f, -2.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.5f, 0.4f, 1.0f); // Dark forest green/teal color for the desk surface

	SetShaderTexture("Desk");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/******************************************************************/
	// Desk Side R
	scaleXYZ = glm::vec3(1.3f, 4.5f, 16.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.7f, 2.0f, -2.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.5f, 0.4f, 1.0f); // Dark forest green/teal color for the desk surface

	SetShaderTexture("Desk");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/******************************************************************/
	// Desk Side BackBoard
	scaleXYZ = glm::vec3(1.3f, 4.5f, 42.6f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.1f, 2.0f, -10.7f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.5f, 0.4f, 1.0f); // Dark forest green/teal color for the desk surface

	SetShaderTexture("Desk");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/****************************************************************/
	// Base of the Laptop
	scaleXYZ = glm::vec3(12.0f, 1.0f, 6.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 1.1f, 0.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for the laptop base

	SetShaderTexture("Body");
	SetTextureUVScale(1.0, 1.0);

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/****************************************************************/
	// Keyboard base
	scaleXYZ = glm::vec3(5.8f, 1.3f, 2.9f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 1.69f, 0.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(2.0f, 2.75f, 0.8f, 2.0f); // Pink color for the laptop base
	SetShaderTexture("Base");
	SetTextureUVScale(1.0, 1.0);
	// Draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	// Screen of the Laptop (base)
	scaleXYZ = glm::vec3(12.0f, 8.0f, 0.1f); // Width, height, depth

	// Set the XYZ rotation for the mesh
	XrotationDegrees = -20.0f; // Tilted back
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 4.5f, -4.2f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for the laptop screen

	SetShaderTexture("Body");
	SetTextureUVScale(1.0, 1.0);

	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/****************************************************************/
	// Desktop Screen
	scaleXYZ = glm::vec3(10.8f, 6.5f, 0.1f); // Width, height, depth

	// Set the XYZ rotation for the mesh
	XrotationDegrees = -20.0f; // Tilted back
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 5.0f, -4.1f); // Positioned at the back edge of the base

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for the laptop screen
	SetShaderTexture("Screen");
	SetTextureUVScale(1.0, 1.0);
	// Draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	// Frog planter body
	scaleXYZ = glm::vec3(2.3f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.5f, 0.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.2745f, 1.0f, 0.4392f, 1.0f); // Green color for frog body
	SetShaderTexture("Frog");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	// Left Eye
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.5f, 2.0f, -2.42f); 

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//ShaderColor(0.2745f, 1.0f, 0.4392f, 1.0f); // Same green color for continuity


	SetShaderTexture("Frog");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// Right Eye
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 2.0f, -2.26f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.2745f, 1.0f, 0.4392f, 1.0f); // Same green color for continuity

	SetShaderTexture("Frog");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// Left Eye Pupil
	scaleXYZ = glm::vec3(0.15f, 0.15f, -0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.4f, 2.0f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black (for pupils)

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// Right Eye Pupil
	scaleXYZ = glm::vec3(0.15f, 0.15f, -0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 2.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); //Black(for pupils)

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// Left Blush
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.0f, 1.4f, -2.45f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1.0f, 0.8f, 0.8f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	// Right Blush
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.5f, 1.4f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(1.0f, 0.8f, 0.8f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Red Bull Can
	scaleXYZ = glm::vec3(1.0f, 4.0f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 0.0f, 0.0f); // Positioned as needed

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color to match the Red Bull can
	//SetShaderColor(0.65f, 0.16f, 0.16f, 1.0f); // Red color for the can
	SetShaderTexture("Can");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("aluminum");
	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	//Red bull top
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// Set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 4.0f, 0.0f); // Positioned as needed

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color to match the Red Bull can (or adjust as needed)
	//SetShaderColor(0.65f, 0.16f, 0.16f, 1.0f); // Red color for the can
	SetShaderTexture("Top");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("aluminum");
	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/****************************************************************/
	// Headband
	scaleXYZ = glm::vec3(5.0f, 5.0f, 4.5f); // Adjust scale as needed

	// set the XYZ rotation for the mesh
	XrotationDegrees = 345.0f; // Rotate to form a headband shape
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.5f, 4.5f, -7.5f); // Position above the desk

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for headband
	SetShaderTexture("Headphone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the torus mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
	/****************************************************************/
	/****************************************************************/
	// Cat Ear (Left)
	scaleXYZ = glm::vec3(2.2f, 3.0f, 0.75); // Adjust scale as needed

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // Rotate to form a headband shape
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.5f, 8.25f, -8.5f); // Position above the desk

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for headband
	SetShaderTexture("Headphone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the torus mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	/****************************************************************/
	// Headband cup (Left)
	scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // Rotated to form a headband shape
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-17.5f, 2.8f, -7.2f); // Position above the desk

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for headband
	SetShaderTexture("Headphone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the torus mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Cushion (Left)
	scaleXYZ = glm::vec3(2.3f, 1.3f, 2.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.9f, 2.5f, -7.2f); // Position above the desk

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(3.0f, 2.75f, 0.8f, 3.0f); // Pink color for headband
	SetShaderTexture("Cushion");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cloth");
	// draw the torus mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Cat Ear (Right Side)
	scaleXYZ = glm::vec3(2.2f, 3.0f, 0.75);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f; // Adjusted for right side

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.5f, 8.5f, -8.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for ear
	SetShaderTexture("Headphone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the cone mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	/****************************************************************/
	// Headband Cup (Right Side)
	scaleXYZ = glm::vec3(2.5f, 2.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -100.0f; // Adjusted for right side

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.5f, 2.9f, -7.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 0.75f, 0.8f, 1.0f); // Pink color for cup
	SetShaderTexture("Headphone");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the half sphere mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Cushion (Right Side)
	scaleXYZ = glm::vec3(2.3f, 1.3f, 2.3f); // Same scale as left side

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -100.0f; // Adjusted for right side

	// set the XYZ position for the mesh (Mirrored X position)
	positionXYZ = glm::vec3(-10.2f, 2.8f, -7.2f); // Mirrored position

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(3.0f, 2.75f, 0.8f, 3.0f); // Adjusted color for cushion
	SetShaderTexture("Cushion");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cloth");
	// draw the sphere mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Mousepad Surface
	scaleXYZ = glm::vec3(5.3f, 0.2f, 5.0f); // Large, flat surface

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 0.2f, 4.5f); // Positioned on the desk

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark color for the mousepad
	SetShaderMaterial("cloth");
	// draw the box mesh for the mousepad surface
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Wrist Rest
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 0.5f, 7.5f); // Positioned at the bottom of the mousepad

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Same dark color as the mousepad
	SetShaderMaterial("cloth");
	// draw the cylinder mesh for the wrist rest
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Mouse Body
	scaleXYZ = glm::vec3(2.0f, 1.5f, 3.0f); 

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 50.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(13.0f, 0.2f, 2.8f); // Positioned on top of the mousepad

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color values into the shader
	//SetShaderColor(3.0f, 2.75f, 0.8f, 3.0f); // Slightly lighter black for the mouse
	SetShaderTexture("Mouse");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the half-sphere mesh for the mouse body
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	/****************************************************************/
	// Mouse Buttons
	scaleXYZ = glm::vec3(2.0f, 0.5f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 135.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.95f, 1.1f, 4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark color for the buttons
	SetShaderTexture("Buttons");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	// draw the box mesh for the mouse buttons
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/****************************************************************/
	// Mouse Scroll Wheel
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 340.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.15f, 1.55f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark color for the scroll wheel
	SetShaderTexture("Wheel");
	SetTextureUVScale(2.0, 2.0);
	SetShaderMaterial("plastic");
	// draw the cylinder mesh for the scroll wheel
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/	
}
