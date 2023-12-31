#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoords;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 lightPosition;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoords;
out vec4 lightVSPosition;

void main() {
	mat4 vm = viewMatrix * modelMatrix;
	vec4 vmp = vm * vec4(vertexPosition + vec3(0.0f, -2.0f, 0.0f),1.f);
	gl_Position = projectionMatrix * vmp;
	fragPosition = vec3(vmp);
	fragNormal = mat3(transpose(inverse(vm))) * vertexNormal;
	lightVSPosition = viewMatrix * lightPosition;
	fragTexCoords = vertexTexCoords;
}
