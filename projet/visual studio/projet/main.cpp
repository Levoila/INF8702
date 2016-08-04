#define GLM_FORCE_RADIANS

#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vec3.hpp>
#include <geometric.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include "ShaderProg.h"
#include <fstream>
#include <sstream>
#include "GPUBuffer.h"
#include "GravitySimulation.h"

using namespace std;

int elapsed = 0;
int elapsedBase = 0;
int frameCount = 0;

glm::mat4x4 MVP;
glm::mat4x4 P;
glm::mat4x4 M;
glm::mat4x4 V;

int winId;

float dist = 10.0f;
float rotY = 0.0f;
float rotX = glm::pi<float>() / 4.0;

GravitySimulation* simulation = nullptr;

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
		glutDestroyWindow(winId);
		exit(EXIT_SUCCESS);
		break;
	case '+':
		dist += 1.0f;
		break;
	case '-':
		dist -= 1.0f;
		break;
	case ' ':
		simulation->playPause();
		break;
	case 'r':
		simulation->reset();
		break;
	case 'b':
		simulation->benchmark();
		break;
	case 'g':
		simulation->setOnGPU(!simulation->isOnGPU());
		break;
	}
}

void specialKeyboard(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		rotY += 0.1;
		break;
	case GLUT_KEY_RIGHT:
		rotY -= 0.1;
		break;
	case GLUT_KEY_UP:
		rotX += 0.1;
		break;
	case GLUT_KEY_DOWN:
		rotX -= 0.1;
		break;
	}
}

void dessiner()
{
	simulation->tick();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	M = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, -dist));
	M = glm::rotate(M, rotX, glm::vec3(1.0, 0.0, 0.0f));
	M = glm::rotate(M, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

	MVP = P * V * M;

	simulation->render();

	glutSwapBuffers();
	glutPostRedisplay();

	++frameCount;
	elapsed = glutGet(GLUT_ELAPSED_TIME);
	if (elapsed - elapsedBase >= 1000) {
		elapsedBase = elapsed;
		cout << "FPS : " << frameCount;
		cout << "   Particles : " << simulation->getParticleCount();
		cout << "   Group size : " << simulation->getGroupSize();
		cout << "   Status : " << (simulation->isOnGPU() ? "running on GPU" : "running on CPU") << std::endl;
		frameCount = 0;
	}
}

void resize(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
}

void processMainMenu(int option)
{
	switch (option) {
	case 0:
		simulation->playPause();
		break;
	case 1:
		simulation->reset();
		break;
	case 2:
		simulation->setOnGPU(false);
		break;
	case 3:
		simulation->setOnGPU(true);
		break;
	case 4:
		simulation->benchmark();
		break;
	}
}

void processParticlesMenu(int option)
{
	if (option == 0) {
		simulation->loadDataset("datasets/dubinski.tab.gz");
	}
	else {
		simulation->generateRandomUniform(option, 10.0f, 14.0f, 14.0f, 14.0f);
	}
}

void processDtMenu(int option)
{
	switch (option) {
	case 0:
		simulation->setDt(1.0f);
		break;
	case 1:
		simulation->setDt(0.1f);
		break;
	case 2:
		simulation->setDt(0.01f);
		break;
	case 3:
		simulation->setDt(0.001f);
		break;
	case 4:
		simulation->setDt(0.0001f);
		break;
	case 5:
		simulation->setDt(0.00001f);
		break;
	}
}

void processGMenu(int option)
{
	switch (option) {
	case 0:
		simulation->setG(1.0f);
		break;
	case 1:
		simulation->setG(0.1f);
		break;
	case 2:
		simulation->setG(0.01f);
		break;
	case 3:
		simulation->setG(0.001f);
		break;
	case 4:
		simulation->setG(0.0001f);
		break;
	case 5:
		simulation->setG(0.00001f);
		break;
	}
}

void processEps2Menu(int option)
{
	switch (option) {
	case 0:
		simulation->setEps2(100.0f);
		break;
	case 1:
		simulation->setEps2(10.0f);
		break;
	case 2:
		simulation->setEps2(1.0f);
		break;
	case 3:
		simulation->setEps2(0.1f);
		break;
	case 4:
		simulation->setEps2(0.01f);
		break;
	case 5:
		simulation->setEps2(0.001f);
		break;
	case 6:
		simulation->setEps2(0.0001f);
		break;
	case 7:
		simulation->setEps2(0.00001f);
		break;
	}
}

void processOptiMenu(int option)
{
	simulation->setOptimizationLevel(option);
}

void processWorkSizeMenu(int option)
{
	simulation->setGroupSize(option);
}

void processOpacityMenu(int option)
{
	switch (option) {
	case 0:
		simulation->setOpacity(1.0f);
		break;
	case 1:
		simulation->setOpacity(0.9f);
		break;
	case 2:
		simulation->setOpacity(0.8f);
		break;
	case 3:
		simulation->setOpacity(0.7f);
		break;
	case 4:
		simulation->setOpacity(0.6f);
		break;
	case 5:
		simulation->setOpacity(0.5f);
		break;
	case 6:
		simulation->setOpacity(0.4f);
		break;
	case 7:
		simulation->setOpacity(0.3f);
		break;
	case 8:
		simulation->setOpacity(0.2f);
		break;
	case 9:
		simulation->setOpacity(0.1f);
		break;
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(800, 600);
	winId = glutCreateWindow("Projet Felix Prevost - N-body Simulation");
	glewInit();
	glutDisplayFunc(dessiner);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutReshapeFunc(resize);

	int optiMenu = glutCreateMenu(processOptiMenu);
	glutAddMenuEntry("Naive approach", 0);
	glutAddMenuEntry("Memory optimized", 1);
	glutAddMenuEntry("Memory optimized + loop unrolling", 2);

	int particlesMenu = glutCreateMenu(processParticlesMenu);
	glutAddMenuEntry("128", 128);
	glutAddMenuEntry("256", 256);
	glutAddMenuEntry("512", 512);
	glutAddMenuEntry("1024", 1024);
	glutAddMenuEntry("2048", 2048);
	glutAddMenuEntry("4096", 4096);
	glutAddMenuEntry("8192", 8192);
	glutAddMenuEntry("16384", 16384);
	glutAddMenuEntry("32768", 32768);
	glutAddMenuEntry("65536", 65536);
	glutAddMenuEntry("Galaxy collision (~80 000 particles)", 0);

	int groupSizeMenu = glutCreateMenu(processWorkSizeMenu);
	glutAddMenuEntry("4", 2);
	glutAddMenuEntry("8", 3);
	glutAddMenuEntry("16", 4);
	glutAddMenuEntry("32", 5);
	glutAddMenuEntry("64", 6);
	glutAddMenuEntry("128", 7);
	glutAddMenuEntry("256", 8);
	glutAddMenuEntry("512", 9);
	glutAddMenuEntry("1024", 10);

	int dtMenu = glutCreateMenu(processDtMenu);
	glutAddMenuEntry("1.0", 0);
	glutAddMenuEntry("0.1", 1);
	glutAddMenuEntry("0.01", 2);
	glutAddMenuEntry("0.001", 3);
	glutAddMenuEntry("0.0001", 4);
	glutAddMenuEntry("0.00001", 5);

	int gMenu = glutCreateMenu(processGMenu);
	glutAddMenuEntry("1.0", 0);
	glutAddMenuEntry("0.1", 1);
	glutAddMenuEntry("0.01", 2);
	glutAddMenuEntry("0.001", 3);
	glutAddMenuEntry("0.0001", 4);
	glutAddMenuEntry("0.00001", 5);

	int eps2Menu = glutCreateMenu(processEps2Menu);
	glutAddMenuEntry("100.0", 0);
	glutAddMenuEntry("10.0", 1);
	glutAddMenuEntry("1.0", 2);
	glutAddMenuEntry("0.1", 3);
	glutAddMenuEntry("0.01", 4);
	glutAddMenuEntry("0.001", 5);
	glutAddMenuEntry("0.0001", 6);
	glutAddMenuEntry("0.00001", 7);

	int opacityMenu = glutCreateMenu(processOpacityMenu);
	glutAddMenuEntry("1.0", 0);
	glutAddMenuEntry("0.9", 1);
	glutAddMenuEntry("0.8", 2);
	glutAddMenuEntry("0.7", 3);
	glutAddMenuEntry("0.6", 4);
	glutAddMenuEntry("0.5", 5);
	glutAddMenuEntry("0.4", 6);
	glutAddMenuEntry("0.3", 7);
	glutAddMenuEntry("0.2", 8);
	glutAddMenuEntry("0.1", 9);

	int mainMenu = glutCreateMenu(processMainMenu);
	glutAddSubMenu("GPU optimization", optiMenu);
	glutAddSubMenu("Particles", particlesMenu);
	glutAddSubMenu("Work group size", groupSizeMenu);
	glutAddSubMenu("Dt", dtMenu);
	glutAddSubMenu("G", gMenu);
	glutAddSubMenu("EPS2", eps2Menu);
	glutAddSubMenu("Particle opacity", opacityMenu);
	glutAddMenuEntry("play/pause", 0);
	glutAddMenuEntry("reset", 1);
	glutAddMenuEntry("Compute on CPU", 2);
	glutAddMenuEntry("Compute on GPU", 3);
	glutAddMenuEntry("Run benchmark", 4);

	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	P = glm::perspective(glm::radians(60.0f), 4.0f / 3.0f, 0.1f, 2000.0f);
	V = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	simulation = new GravitySimulation();
	simulation->setMVP(&MVP);
	simulation->generateRandomUniform(16384, 10.0f, 14, 14, 14);
	simulation->setDt(0.0002f);
	simulation->setG(1.0f);
	simulation->setEps2(10.0f);

	glutMainLoop();

	return 0;
}