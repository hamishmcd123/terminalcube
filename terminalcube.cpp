#include <array>
#include <algorithm>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <thread>
#include <conio.h>

std::array<std::array<char, 128>, 64> screen;

struct Camera {
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
	void updateRotation();
};

Camera cam;

std::array<glm::vec2, 12> cubeEdges = {
	glm::vec2(0, 1), glm::vec2(1, 2), glm::vec2(2, 3), glm::vec2(3, 0), // bottom face
	glm::vec2(4, 5), glm::vec2(5, 6), glm::vec2(6, 7), glm::vec2(7, 4), // top face
	glm::vec2(0, 4), glm::vec2(1, 5), glm::vec2(2, 6), glm::vec2(3, 7)  // vertical edges
};

std::array<glm::vec4, 8> cubeVertices = {
	glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), // 0
	glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), // 1
	glm::vec4(0.5f,  0.5f, -0.5f, 1.0f), // 2
	glm::vec4(-0.5f,  0.5f, -0.5f, 1.0f), // 3
	glm::vec4(-0.5f, -0.5f,  0.5f, 1.0f), // 4
	glm::vec4(0.5f, -0.5f,  0.5f, 1.0f), // 5
	glm::vec4(0.5f,  0.5f,  0.5f, 1.0f), // 6
	glm::vec4(-0.5f,  0.5f,  0.5f, 1.0f)  // 7
};

glm::mat4 view = glm::lookAt(cam.cameraPos, cam.cameraPos + cam.cameraFront, cam.cameraUp);
glm::mat4 perspective = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 100.0f);

glm::ivec3 viewportTransform(const glm::vec4& ndc, int screenWidth, int screenHeight) {
	int x = static_cast<int>(((ndc.x + 1.0f) * 0.5f * screenWidth));
	int y = static_cast<int>(((1.0f - ndc.y) * 0.5f * screenHeight));
	int z = static_cast<int>((ndc.z * 255));
	return glm::ivec3(x, y, z);
}

void clear() {
	for (auto& i : screen) {
		std::fill(i.begin(), i.end(), ' ');
	}
}


void draw() {
	for (const auto& i : screen) {
		for (const auto& j : i) {
			std::cout << j;
		}
		std::cout << '\n';
	}
}

void plot(int x, int y) {
	screen[y][x] = '@';
}

void drawLine(int x1, int y1, int x2, int y2) {
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = (x2 > x1) ? 1 : -1;
	int sy = (y2 > y1) ? 1 : -1;

	// Whichever of sx and sy is bigger, that will be the driving axis
	if (dy <= dx) {
		int err = dx / 2;
		while (x1 != x2) {
			plot(x1, y1);
			x1 += sx;
			err -= dy;
			if (err < 0) {
				y1 += sy;
				err += dx;
			}
		}
	}
	else {
		int err = dy / 2;
		while (y1 != y2) {
			plot(x1, y1);
			y1 += sy;
			err -= dx;
			if (err < 0) {
				x1 += sx;
				err += dy;
			}
		}
	}
	plot(x2, y2);
}

int main() {
	//Main Loop
	std::cout << "\x1b[32m";
	float angle = 0.0f;

	while (true) {
		if (_kbhit()) {
			char c = _getch();
			if (c == 'a') cam.cameraPos += 0.5f * glm::normalize(glm::cross(cam.cameraUp, cam.cameraFront));
			if (c == 'd') cam.cameraPos -= 0.5f * glm::normalize(glm::cross(cam.cameraUp, cam.cameraFront));
			if (c == 'w') cam.cameraPos += cam.cameraFront;
			if (c == 's') cam.cameraPos -= cam.cameraFront;
			if (c == 'q') cam.yaw -= 10;
			if (c == 'e') cam.yaw += 10;
			if (c == 'j') cam.pitch -= 10;
			if (c == 'k') cam.pitch += 10;
			if (c == 'x') break;
		}

		clear();
		cam.updateRotation();
		glm::mat4 view = glm::lookAt(cam.cameraPos, cam.cameraPos + cam.cameraFront, cam.cameraUp);
		std::cout  << "\x1b[?25l";
		std::cout << "\x1b[H";
		angle += 5.0f;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 1.0f));
		glm::mat4 mvp = perspective * view * model;
		std::array<glm::vec3, 8> transformed;
		std::array<bool, 8> isInside;

		for (int i = 0; i < 8; i++) {
			glm::vec4 clip = mvp * cubeVertices[i];

			if (clip.w != 0) {
				clip /= clip.w;
			}
			
			glm::vec3 final = viewportTransform(clip, 128, 64);

			transformed[i] = final;

			isInside[i] = !(clip.x < -1 || clip.x > 1 || clip.y < -1 || clip.y > 1 || clip.z < -1 || clip.z > 1);

			if (isInside[i]) {
				plot(final.x, final.y);
			}
		}

		for (const auto& edge : cubeEdges) {
			int start = edge.x;
			int end = edge.y;

			glm::vec3& p1 = transformed[start];
			glm::vec3& p2 = transformed[end];

			if (isInside[start] && isInside[end]) {
				drawLine(p1.x, p1.y, p2.x, p2.y);
			}
		}

		draw();
	}
	return 0;
}

void Camera::updateRotation()
{
	glm::vec3 direction;

	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = direction;
}
