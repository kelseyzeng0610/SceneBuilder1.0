#include "MyGLCanvas.h"
#include <glm/gtc/type_ptr.hpp>

int Shape::m_segmentsX;
int Shape::m_segmentsY;
int MyGLCanvas::nextObjectId = 0;
int MyGLCanvas::selectedObjId = -1;



int Shape::lastIndex;
std::vector<std::array<float, 3>> Shape::m_points;
std::unordered_map<int, glm::vec3> Shape::m_normals;

// Struct for storing each object in the scenegraph described by an xml file
std::vector<SceneObject> sceneObjects;

MyGLCanvas::MyGLCanvas(int x, int y, int w, int h, const char *l) : Fl_Gl_Window(x, y, w, h, l)
{
	mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_DOUBLE);

	eyePosition = glm::vec3(0.0f, 0.0f, 5.0f);
	lookatPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	rotVec = glm::vec3(0.0f, 0.0f, 0.0f);
  nextObjectId = 0;
  selectedObjId = -1;

	pixelWidth = w;
	pixelHeight = h;

	wireframe = 0;
	viewAngle = 60;
	clipNear = 0.01f;
	clipFar = 10.0f;

	castRay = false;
	drag = false;
	mouseX = 0;
	mouseY = 0;
	spherePosition = glm::vec3(0, 0, 0);

	

	camera.setViewAngle(viewAngle);
	camera.setNearPlane(clipNear);
	camera.setFarPlane(clipFar);
	// Set the mode so we are modifying our objects.
	// camera.orientLookVec(eyePosition, glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
  camera.orientLookVec(glm::vec3(0.0f, 0.0f, 3.0f), 
                         glm::vec3(0.0f, 0.0f, 0.0f), 
                         glm::vec3(0.0f, 1.0f, 0.0f));
	isectOnly = 1;
	segmentsX = segmentsY = 10;
	

  //  addObject(SHAPE_CUBE);
}

void MyGLCanvas::setupCamera() {
    
    glm::mat4 view = glm::lookAt(eyePosition, lookatPoint, glm::vec3(0.0f, 1.0f, 0.0f));

    
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));
}


MyGLCanvas::~MyGLCanvas()
{
  delete cube;
  delete cylinder;
  delete cone;
  delete sphere;
  objectList.clear();
  

}

// loop over objectList and select the one that is hit by the ray
// returns the closest object and its index
// std::pair<ObjectNode, int> MyGLCanvas::closestObject(glm::vec3 rayOriginPoint, int mouseX, int mouseY){
// 	std::pair<ObjectNode, int> closest_obj_info;

// 	glm::vec3 intersection_obj = glm::vec3(0);
// 	glm::vec3 intersection = glm::vec3(0);
// 	glm::vec3 normal = glm::vec3(0);

// 	ObjectNode* closest_obj = nullptr;
// 	double closest_t = INFINITY;

// 	ObjectNode small;

 
// 	return closest_obj_info;
// }

glm::vec3 MyGLCanvas::generateRay(int pixelX, int pixelY)
{
	glm::vec3 eyePos = camera.getEyePoint();

	glm::vec3 farPoint = getEyePoint(pixelX, pixelY, pixelWidth, pixelHeight);
	return glm::normalize(farPoint - eyePos);
}

glm::vec3 MyGLCanvas::getEyePoint(int pixelX, int pixelY, int screenWidth, int screenHeight)
{
	// Convert pixel coordinates to normalized device coordinates (-1 to 1)
	float ndcX = (2.0f * pixelX / screenWidth) - 1.0f;
	float ndcY = 1.0f - (2.0f * pixelY / screenHeight);

	// Scale based on camera's view angle and aspect ratio
	float aspectRatio = (float)screenWidth / screenHeight;
	float filmPlanDepth = camera.getFilmPlanDepth();

	// Create point in camera space (using film plane depth from camera)
	glm::vec4 cameraPoint(ndcX * aspectRatio, ndcY, -filmPlanDepth, 1.0f);

	// Transform from camera space to world space using inverse view matrix
	glm::mat4 viewToWorld = camera.getInverseModelViewMatrix();
	glm::vec4 worldPoint = viewToWorld * cameraPoint;

	return glm::vec3(worldPoint);
}

glm::vec3 MyGLCanvas::getIsectPointWorldCoord(glm::vec3 eye, glm::vec3 ray, float t)
{
	return eye + ray * t;
}

double MyGLCanvas::intersect(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix)
{
	// transform eye point and ray into object space
	glm::vec4 eyePointPO = transformMatrix * glm::vec4(eyePointP, 0.0f);
	glm::vec4 rayVO = transformMatrix * glm::vec4(rayV, 0.0f);

	// transform sphere center
	glm::vec4 sphereCenterO = transformMatrix * glm::vec4(spherePosition, 0.0f);

	// adjust eye point relative to sphere center
	glm::vec4 relativeEyePoint = eyePointPO - sphereCenterO;

	double A = glm::dot(rayVO, rayVO);
	double B = 2.0 * glm::dot(rayVO, relativeEyePoint);
	double C = glm::dot(relativeEyePoint, relativeEyePoint) - 0.25;

	double discriminant = pow(B, 2) - 4.0 * A * C;

	// no intersection
	if (discriminant < 0)
		return -1.0;

	double t1 = (-B - sqrt(discriminant)) / (2.0 * A);
	double t2 = (-B + sqrt(discriminant)) / (2.0 * A);

	// return nearest intersection
	if (t1 > 0 && t2 > 0)
	{
		return std::min(t1, t2);
	}
	else if (t1 > 0)
	{
		return t1;
	}
	else if (t2 > 0)
	{
		return t2;
	}

	return -1.0;
}

void MyGLCanvas::draw()
{
  
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	if (!valid())
	{ // this is called when the GL canvas is set up for the first time or when it is resized...
		printf("establishing GL context\n");

	

		glViewport(0, 0, w(), h());
		updateCamera(w(), h());

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// glShadeModel(GL_SMOOTH);
		glShadeModel(GL_FLAT);

		GLfloat light_pos0[] = {eyePosition.x, eyePosition.y, eyePosition.z, 0.0f};
		GLfloat ambient[] = {0.7f, 0.7f, 0.7f, 1.0f};
		GLfloat diffuse[] = {0.5f, 0.5f, 0.5f, 1.0f};

		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos0);

		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		/****************************************/
		/*          Enable z-buferring          */
		/****************************************/

		glEnable(GL_DEPTH_TEST);
		glPolygonOffset(1, 1);
	}

	// Clear the buffer of colors in each bit plane.
	// bit plane - A set of bits that are on or off (Think of a black and white image)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  glMatrixMode(GL_MODELVIEW);
	// Se
	camera.orientLookVec(eyePosition, glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	glLoadMatrixf(glm::value_ptr(camera.getModelViewMatrix()));
	drawObjects();
  drawAxis();

  glFlush();
    
	
}



void MyGLCanvas::drawObjects(){
   

  for(ObjectNode obj: objectList){
    // printf("Drawing Object ID %d\n", obj.id);

    glPushMatrix();
    glScalef(obj.scale.x, obj.scale.y, obj.scale.z);
    glRotatef(obj.rotation.x, 1.0f, 0.0f, 0.0f);
    glRotatef(obj.rotation.y, 0.0f, 1.0f, 0.0f);
    glRotatef(obj.rotation.z, 0.0f, 0.0f, 1.0f);
    glTranslatef(obj.translate.x, obj.translate.y, obj.translate.z);
    glColor3f(1.0f, 1.0f, 1.0f); 
 if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor3f(1.0f, 1.0f, 1.0f);
        }

     if (obj.primitive != nullptr) {
    obj.primitive->draw();
} else {
    printf("Object ID %d has a null primitive\n", obj.id);
}
    
     glPopMatrix();
    
  }
}


int MyGLCanvas::selectObject(int mouseX, int mouseY){

  printf("Selecting Object\n");
  glm::vec3 eyePoint = getEyePoint(mouseX, mouseY, pixelWidth, pixelHeight);
  glm::vec3 ray = generateRay(mouseX, mouseY);

  ObjectNode* closest_obj = nullptr;

  int selectedObjId = -1;
  float closest_t = INFINITY;
  std::vector<double> results;
  for(ObjectNode obj: objectList){
    results.clear();
    glm::vec3 objScale = obj.scale;
    glm::vec3 objTranslate = obj.translate;
    glm::vec3 objRotation = obj.rotation;

    glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), objTranslate) * 
                                glm::rotate(glm::mat4(1.0f), glm::radians(objRotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(objRotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(objRotation.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
                                glm::scale(glm::mat4(1.0f), objScale);
    glm::mat4 invTransformMatrix = glm::inverse(transformMatrix); 

    glm::vec3 invRay = glm::vec3(invTransformMatrix * glm::vec4(ray, 0.0f));
    glm::vec3 invEyePoint = glm::vec3(invTransformMatrix * glm::vec4(eyePoint, 1.0f));
    glm::vec3 p = glm::vec3(invEyePoint);
    glm::vec3 d = glm::normalize(glm::vec3(invRay));


    switch(obj.primitive->getType()){
      case SHAPE_CUBE:
        results = intersectWithCube(invEyePoint, invRay, invTransformMatrix);
        break;
      case SHAPE_SPHERE:
        results = intersectWithSphere(invEyePoint, invRay, invTransformMatrix);
        break;
      case SHAPE_CYLINDER:
       results = intersectWithCylinder(invEyePoint, invRay, invTransformMatrix);
        break;
      case SHAPE_CONE:
         results= intersectWithCone(invEyePoint, invRay, invTransformMatrix);
        break;
      default:
        obj.primitive = nullptr;
        break;
    }
    for (double t : results) {
                      if (t > 0) {
                          glm::vec3 obj_intersection = p + float(t) * d;
                          glm::vec4 world_intersection = transformMatrix * glm::vec4(obj_intersection, 1.0f);
                          double dist = glm::length(glm::vec3(world_intersection) - eyePoint);

                          if (dist < closest_t) {
                              closest_t = dist;
                              closest_obj = &obj;
                          }
                      }
                  }


  }
  if (closest_obj != nullptr) {
    selectedObjId = closest_obj->id;
    return selectedObjId;
    printf("Selected Object ID: %d\n", selectedObjId);
  } else {
    printf("No object selected\n");
    return -1;
  }




}



void MyGLCanvas::drawObject() {
	//add object properties

	//cast ray onto object
	glMatrixMode(GL_MODELVIEW);
	// Set the mode so we are modifying our objects.
	camera.orientLookVec(eyePosition, glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	glLoadMatrixf(glm::value_ptr(camera.getModelViewMatrix()));

	if (castRay == true)
	{
	
		glm::vec3 eyePointP = getEyePoint(mouseX, mouseY, pixelWidth, pixelHeight);
		
		glm::vec3 rayV = generateRay(mouseX, mouseY);	
		glm::vec3 sphereTransV(spherePosition[0], spherePosition[1], spherePosition[2]);

		float t = intersect(eyePointP, rayV, glm::translate(glm::mat4(1.0), sphereTransV));
		glm::vec3 isectPointWorldCoord = getIsectPointWorldCoord(eyePointP, rayV, t);

		if (t > 0)
		{
			glColor3f(1, 0, 0);
			glPushMatrix();
			glTranslated(spherePosition[0], spherePosition[1], spherePosition[2]);
			glutWireCube(1.0f);
			glPopMatrix();
			glPushMatrix();
			glTranslatef(isectPointWorldCoord[0], isectPointWorldCoord[1], isectPointWorldCoord[2]);
			glutSolidSphere(0.05f, 10, 10);
			glPopMatrix();
			printf("hit!\n");
		}
		else
		{
			printf("miss!\n");
		}
	}

	glPushMatrix();

	// move the sphere to the designated position
	glTranslated(spherePosition[0], spherePosition[1], spherePosition[2]);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glColor3f(1.0, 1.0, 1.0);
	if (wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	glPushMatrix();
	glRotatef(90, 0, 1, 0);
	
	glPopMatrix();

	glPopMatrix();

}





void MyGLCanvas::updateCamera(int width, int height)
{
	float xy_aspect;
	xy_aspect = (float)width / (float)height;

	camera.setScreenSize(width, height);

	
	glMatrixMode(GL_PROJECTION);

	
	glLoadIdentity();
	glm::mat4 projection = camera.getProjectionMatrix();
	glLoadMatrixf(glm::value_ptr(projection));
}

int MyGLCanvas::handle(int e)
{
  printf("Event happened: %d\n", e);
	switch (e)
	{
	case FL_DRAG:
    mouseX = (int)Fl::event_x();
    mouseY = (int)Fl::event_y();

    if (drag == true) {
        
        glm::vec3 eyePoint = getEyePoint(mouseX, mouseY, pixelWidth, pixelHeight);
        glm::vec3 rayDir = generateRay(mouseX, mouseY);
        
        
        glm::vec3 new_intersection = eyePoint + (rayDir * oldT);
        
        
        glm::vec3 offset = oldIsectPoint - oldCenter;
        spherePosition = new_intersection - offset; 


    }
    return (1);
	case FL_MOVE:
		Fl::belowmouse(this);
		
		mouseX = (int)Fl::event_x();
		mouseY = (int)Fl::event_y();

		break;
	case FL_PUSH:
		printf("mouse push\n");
	
		if ((Fl::event_button() == FL_LEFT_MOUSE) && (castRay == false))
		{ 
			castRay = true;
      int selected = selectObject(mouseX, mouseY);
      printf("Selected Object ID: %d\n", selected);
      return 1;
		}
		else if ((Fl::event_button() == FL_RIGHT_MOUSE) && (drag == false))
		{ 
      
      // move the selected object 
			glm::vec3 eyePointP = getEyePoint(mouseX, mouseY, pixelWidth, pixelHeight);
			glm::vec3 rayV = generateRay(mouseX, mouseY);
			glm::vec3 sphereTransV(spherePosition[0], spherePosition[1], spherePosition[2]);
			float t = intersect(eyePointP, rayV, glm::translate(glm::mat4(1.0), sphereTransV));
			glm::vec3 isectPointWorldCoord = getIsectPointWorldCoord(eyePointP, rayV, t);

			// this only activates if the mouse intersects with the sphere to enable dragging
			if (t > 0)
			{
				drag = true;
				printf("drag is true\n");
				oldCenter = spherePosition;
				oldIsectPoint = isectPointWorldCoord;
				oldT = t;
			}
		}
		return (1);
	case FL_RELEASE:
		printf("mouse release\n");
		if (Fl::event_button() == FL_LEFT_MOUSE)
		{
			castRay = false;
		}
		else if (Fl::event_button() == FL_RIGHT_MOUSE)
		{
			drag = false;
		}
		return (1);
	case FL_KEYUP:
		printf("keyboard event: key pressed: %c\n", Fl::event_key());
		switch (Fl::event_key())
		{
		case 'w':
			eyePosition.y += 0.05f;
			break;
		case 'a':
			eyePosition.x += 0.05f;
			break;
		case 's':
			eyePosition.y -= 0.05f;
			break;
		case 'd':
			eyePosition.x -= 0.05f;
			break;
		}
		updateCamera(w(), h());
		break;
	case FL_MOUSEWHEEL:
		printf("mousewheel: dx: %d, dy: %d\n", Fl::event_dx(), Fl::event_dy());
		eyePosition.z += Fl::event_dy() * -0.05f;
		updateCamera(w(), h());
		break;
	}

	  return Fl_Gl_Window::handle(e);
}

void MyGLCanvas::resize(int x, int y, int w, int h)
{
	Fl_Gl_Window::resize(x, y, w, h);
	puts("resize called");
}

void MyGLCanvas::drawAxis()
{
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(1.0, 0, 0);
	glColor3f(0.0, 1.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.0, 1.0, 0);
	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 1.0);
	glEnd();
	glEnable(GL_LIGHTING);
}

void MyGLCanvas::resetScene() {
	//TODO: reset scene to default 
    objectList.clear();
    nextObjectId = 0;
    selectedObjId = -1;
    redraw();
}

void MyGLCanvas::setShape(OBJ_TYPE type) {
    objType = type;
    addObject(type);
    printf("set shape to: %d\n", type);
    redraw();
}

void MyGLCanvas::setSegments() {
    shape->setSegments(segmentsX, segmentsY);
}

// Logic derived from lab04
std::vector<double> MyGLCanvas::intersectWithSphere(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix)
{
  // Transform ray and eye point to object space
  glm::vec4 eyePointPO = transformMatrix * glm::vec4(eyePointP, 1.0f);
  glm::vec4 rayVO = transformMatrix * glm::vec4(rayV, 0.0f);

  glm::vec3 p = glm::vec3(eyePointPO);
  glm::vec3 d = glm::normalize(glm::vec3(rayVO));

  double A = glm::dot(glm::vec3(d), glm::vec3(d));
  double B = 2.0f * glm::dot(glm::vec3(d), glm::vec3(p));
  double C = glm::dot(glm::vec3(p), glm::vec3(p)) - 0.25;

  double discriminant = (B * B) - (4.0 * A * C);

  std::vector<double> results;

  if (discriminant < 0)
  {
    return results;
  }

  // Calculate both intersection points
  double t1 = (-B - sqrt(discriminant)) / (2.0 * A);
  double t2 = (-B + sqrt(discriminant)) / (2.0 * A);

  if (t1 > 0) results.push_back(t1);
  if (t2 > 0) results.push_back(t2);

  return results;
}

std::vector<double> MyGLCanvas::intersectWithCube(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix)
{
  
  glm::vec4 eyePointPO = transformMatrix * glm::vec4(eyePointP, 1.0f);
  glm::vec4 rayVO = transformMatrix * glm::vec4(rayV, 0.0f);

  glm::vec3 p = glm::vec3(eyePointPO);
  glm::vec3 d = glm::normalize(glm::vec3(rayVO));

  double t_min = INFINITY;
  glm::vec3 intersection;

  std::vector<double> results;

  // Right face of cube, x = 0.5
  if (d.x != 0)
  {
    double t_right = (0.5 - p.x) / d.x;
    intersection = p + float(t_right) * d;
    if (t_right > 0 &&
        intersection.y >= -0.5 && intersection.y <= 0.5 &&
        intersection.z >= -0.5 && intersection.z <= 0.5)
    {
      results.push_back(t_right);
    }
  }

  // Left face of cube, x = -0.5
  if (d.x != 0)
  {
    double t_left = (-0.5 - p.x) / d.x;
    intersection = p + float(t_left) * d;
    if (t_left > 0 &&
        intersection.y >= -0.5 && intersection.y <= 0.5 &&
        intersection.z >= -0.5 && intersection.z <= 0.5)
    {
      results.push_back(t_left);
    }
  }

  // Top face of cube, y = 0.5
  if (d.y != 0)
  {
    double t_top = (0.5 - p.y) / d.y;
    intersection = p + float(t_top) * d;
    if (t_top > 0 &&
        intersection.x >= -0.5 && intersection.x <= 0.5 &&
        intersection.z >= -0.5 && intersection.z <= 0.5)
    {
      results.push_back(t_top);
    }
  }

  // Bottom face of cube, y = -0.5
  if (d.y != 0)
  {
    double t_bottom = (-0.5 - p.y) / d.y;
    intersection = p + float(t_bottom) * d;
    if (t_bottom > 0 &&
        intersection.x >= -0.5 && intersection.x <= 0.5 &&
        intersection.z >= -0.5 && intersection.z <= 0.5)
    {
      results.push_back(t_bottom);
    }
  }

  // Front face of cube, z = 0.5
  if (d.z != 0)
  {
    double t_front = (0.5 - p.z) / d.z;
    intersection = p + float(t_front) * d;
    if (t_front > 0 &&
        intersection.x >= -0.5 && intersection.x <= 0.5 &&
        intersection.y >= -0.5 && intersection.y <= 0.5)
    {
      results.push_back(t_front);
    }
  }

  // Back face of cube, z = -0.5
  if (d.z != 0)
  {
    double t_back = (-0.5 - p.z) / d.z;
    intersection = p + float(t_back) * d;
    if (t_back > 0 &&
        intersection.x >= -0.5 && intersection.x <= 0.5 &&
        intersection.y >= -0.5 && intersection.y <= 0.5)
    {
      results.push_back(t_back);
    }
  }

  // Return smallest positive t here
  return results;
}


void MyGLCanvas::addObject(OBJ_TYPE type) {
  ObjectNode node;
 
  node.id = nextObjectId++;
  node.translate = glm::vec3(0.0f, 0.0f, 0.0f);
  node.scale = glm::vec3(1.0f, 1.0f, 1.0f);
  node.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
  

    switch (type) {
        case SHAPE_SPHERE:
            node.primitive = new Sphere();
            break;
        case SHAPE_CUBE:
            node.primitive = new Cube();
            break;
        case SHAPE_CYLINDER:
            node.primitive = new Cylinder();
            break;
        case SHAPE_CONE:
            node.primitive = new Cone();
            break;
        default:
            node.primitive = new Sphere();
            break;
    }
    if (node.primitive != nullptr) {
      node.primitive->setSegments(segmentsX, segmentsY);
       objectList.push_back(node);
    }else{
      printf("Object ID %d has a null primitive\n", node.id);
    }
   
    


}

std::vector<double> MyGLCanvas::intersectWithCylinder(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix)
{
  // Transform ray and eye point to object space
  glm::vec4 eyePointPO = transformMatrix * glm::vec4(eyePointP, 1.0f);
  glm::vec4 rayVO = transformMatrix * glm::vec4(rayV, 0.0f);

  glm::vec3 p = glm::vec3(eyePointPO);
  glm::vec3 d = glm::normalize(glm::vec3(rayVO));

  double t_min = INFINITY;

  // Expand (px + tdx)^2 + (pz + tdz)^2 = 0.25 to find A, B and C
  double A = d.x * d.x + d.z * d.z;
  double B = 2.0f * (p.x * d.x + p.z * d.z);
  double C = p.x * p.x + p.z * p.z - 0.25;

  double discriminant = (B * B) - (4.0f * A * C);

  std::vector<double> results;

  if (discriminant >= 0 && A != 0)
  {
    double t1 = (-B - sqrt(discriminant)) / (2.0 * A);
    double t2 = (-B + sqrt(discriminant)) / (2.0 * A);

    // Check if intersection y value is between (-0.5, 0.5) before potentially setting new t_min
    if (t1 > 0)
    {
      glm::vec3 intersection = p + float(t1) * d;
      if (intersection.y >= -0.5f && intersection.y <= 0.5f)
      {
        results.push_back(t1);
      }
    }
    if (t2 > 0)
    {
      glm::vec3 intersection = p + float(t2) * d;
      if (intersection.y >= -0.5f && intersection.y <= 0.5f)
      {
        results.push_back(t2);
      }
    }
  }

  // Check intersection of the two caps
  if (d.y != 0)
  {
    double t_top = (0.5f - p.y) / d.y;
    double t_bottom = (-0.5f - p.y) / d.y;

    if (t_top > 0)
    {
      glm::vec3 intersection = p + float(t_top) * d;
      if (intersection.x * intersection.x + intersection.z * intersection.z <= 0.25)
      {
        results.push_back(t_top);
      }
    }

    if (t_bottom > 0)
    {
      glm::vec3 intersection = p + float(t_bottom) * d;
      if (intersection.x * intersection.x + intersection.z * intersection.z <= 0.25)
      {
        results.push_back(t_bottom);
      }
    }
  }

  return results;
}

std::vector<double> MyGLCanvas::intersectWithCone(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix)
{
  glm::vec4 eyePointPO = transformMatrix * glm::vec4(eyePointP, 1.0f);
  glm::vec4 rayVO = transformMatrix * glm::vec4(rayV, 0.0f);

  glm::vec3 p = glm::vec3(eyePointPO);
  glm::vec3 d = glm::normalize(glm::vec3(rayVO));

  double t_min = INFINITY;

  double A = d.x * d.x + d.z * d.z - 0.25 * d.y * d.y;
  double B = 2.0f * (p.x * d.x + p.z * d.z) + (0.25 - (0.5 * p.y)) * d.y;
  double C = p.x * p.x + p.z * p.z - 0.25 * (0.5 - p.y) * (0.5 - p.y);

  double discriminant = (B * B) - (4.0f * A * C);

  std::vector<double> results;

  if (discriminant >= 0 && A != 0)
  {
    double t1 = (-B - sqrt(discriminant)) / (2.0 * A);
    double t2 = (-B + sqrt(discriminant)) / (2.0 * A);

    if (t1 > 0)
    {
      glm::vec3 intersection = p + float(t1) * d;
      if (intersection.y >= -0.5f && intersection.y <= 0.5f)
      {
        results.push_back(t1);
      }
    }
    if (t2 > 0)
    {
      glm::vec3 intersection = p + float(t2) * d;
      if (intersection.y >= -0.5f && intersection.y <= 0.5f)
      {
        results.push_back(t2);
      }
    }
  }

  if (d.y != 0)
  {
    double t_cap = (-0.5f - p.y) / d.y;

    if (t_cap > 0)
    {
      glm::vec3 intersection = p + float(t_cap) * d;
      if (intersection.x * intersection.x + intersection.z * intersection.z <= 0.25)
      {
        results.push_back(t_cap);
      }
    }
  }

  return results;
}
