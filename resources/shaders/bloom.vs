#version 330

// Fullscreen quad — no model matrix, just clip-space vertices
in vec3 vertexPosition;
in vec2 vertexTexCoord;

out vec2 fragTexCoord;

void main()
{
    fragTexCoord = vertexTexCoord;
    gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
